#include "FavoritesController.h"

#include <drogon/drogon.h>

#include <charconv>
#include <cstdint>

#include "JsonResponder.h"

// Lista as receitas favoritas do usuario
void FavoritesController::listFavorites(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    auto respond = api::jsonResponder(callback);
    auto reject = api::errorResponder(respond);

    const auto userId = api::sessionUserId(req);

    if (userId.empty()) {
        reject(drogon::k401Unauthorized, "You must log in to access favorites.");
        return;
    }

    drogon::app().getDbClient("default")->execSqlAsync(
        "SELECT f.recipe_id AS id, r.id AS available_id, "
        "r.name, r.ingredient_list, r.image_url, "
        "u.display_name AS author "
        "FROM recipe_favorites f "
        "LEFT JOIN recipes r ON r.id = f.recipe_id "
        "AND r.visibility = 'public' "
        "LEFT JOIN users u ON u.id = r.user_id "
        "WHERE f.user_id = ? "
        "ORDER BY r.name ASC, f.recipe_id ASC",

        [respond](const drogon::orm::Result& result) {
            Json::Value body;
            body["recipes"] = Json::Value(Json::arrayValue);

            for (const auto& row : result) {
                Json::Value recipe;

                recipe["id"] = row["id"].as<std::string>();

                const bool available = !row["available_id"].isNull();
                recipe["available"] = available;

                if (available) {
                    recipe["name"] = row["name"].as<std::string>();
                    recipe["ingredient_list"] = row["ingredient_list"].as<std::string>();
                    recipe["author"] = row["author"].as<std::string>();

                    if (row["image_url"].isNull()) {
                        recipe["image_url"] = Json::Value();
                    } else {
                        recipe["image_url"] = row["image_url"].as<std::string>();
                    }
                } else {
                    recipe["name"] = "Recipe no longer available";
                }

                body["recipes"].append(recipe);
            }

            respond(drogon::k200OK, body);
        },

        [reject](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Favorites query failed";
            reject(drogon::k503ServiceUnavailable, "Unable to load your favorites.");
        },

        userId);
}

// Adiciona ou remove uma receita dos favoritos
void FavoritesController::changeFavorite(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string id) const {
    auto respond = api::jsonResponder(callback);
    auto reject = api::errorResponder(respond);

    const auto userId = api::sessionUserId(req);

    if (userId.empty()) {
        reject(drogon::k401Unauthorized, "You must log in to manage favorites.");
        return;
    }

    std::uint32_t recipeId = 0;
    const auto* end = id.data() + id.size();
    const auto parsed = std::from_chars(id.data(), end, recipeId);

    if (id.empty() || id.front() == '0' || parsed.ec != std::errc{} || parsed.ptr != end ||
        recipeId == 0) {
        reject(drogon::k400BadRequest, "Invalid recipe ID.");
        return;
    }

    auto database = drogon::app().getDbClient("default");

    auto databaseError = [reject](const drogon::orm::DrogonDbException&) {
        LOG_ERROR << "Favorite operation failed";
        reject(drogon::k503ServiceUnavailable, "Unable to update your favorites.");
    };

    // Remove apenas o favorito, sem deletar a receita original
    if (req->method() == drogon::Delete) {
        database->execSqlAsync(
            "DELETE FROM recipe_favorites "
            "WHERE user_id = ? AND recipe_id = ?",

            [respond](const drogon::orm::Result&) {
                Json::Value body;
                body["message"] = "Recipe removed from favorites.";
                body["favorited"] = false;
                respond(drogon::k200OK, body);
            },

            databaseError, userId, recipeId);

        return;
    }

    // Adiciona uma receita publica respeitando o limite do plano
    database->execSqlAsync(
        "INSERT INTO recipe_favorites (user_id, recipe_id) "
        "SELECT u.id, r.id "
        "FROM users u "
        "JOIN user_tiers t ON t.id = u.tier_id "
        "JOIN recipes r ON r.id = ? "
        "WHERE u.id = ? "
        "AND r.visibility = 'public' "
        "AND r.user_id <> u.id "
        "AND (SELECT COUNT(*) FROM recipes own "
        "WHERE own.user_id = u.id) + "
        "(SELECT COUNT(*) FROM recipe_favorites f "
        "WHERE f.user_id = u.id) < t.recipe_limit "
        "ON DUPLICATE KEY UPDATE recipe_id = VALUES(recipe_id)",

        [respond, reject, database, databaseError, userId,
         recipeId](const drogon::orm::Result& result) {
            if (result.affectedRows() > 0) {
                Json::Value body;
                body["message"] = "Recipe added to favorites.";
                body["favorited"] = true;
                respond(drogon::k200OK, body);
                return;
            }

            // Verifica por que nenhuma receita foi adicionada
            database->execSqlAsync(
                "SELECT t.tier_name, t.recipe_limit, "
                "r.id AS public_id, f.recipe_id AS saved_id "
                "FROM users u "
                "JOIN user_tiers t ON t.id = u.tier_id "
                "LEFT JOIN recipes r ON r.id = ? "
                "AND r.visibility = 'public' "
                "AND r.user_id <> u.id "
                "LEFT JOIN recipe_favorites f "
                "ON f.user_id = u.id AND f.recipe_id = r.id "
                "WHERE u.id = ?",

                [respond, reject](const drogon::orm::Result& rows) {
                    if (rows.empty()) {
                        reject(drogon::k401Unauthorized, "You must log in to manage favorites.");
                        return;
                    }

                    const auto row = rows[0];

                    if (row["public_id"].isNull()) {
                        reject(drogon::k404NotFound, "Recipe is not available to save.");
                        return;
                    }

                    if (!row["saved_id"].isNull()) {
                        Json::Value body;
                        body["message"] = "Recipe already in favorites.";
                        body["favorited"] = true;
                        respond(drogon::k200OK, body);
                        return;
                    }

                    Json::Value body;
                    body["error"] = "You have reached your plan's recipe limit.";
                    body["limit_reached"] = true;
                    body["tier_name"] = row["tier_name"].as<std::string>();
                    body["recipe_limit"] = row["recipe_limit"].as<unsigned int>();

                    respond(drogon::k403Forbidden, body);
                },

                databaseError, recipeId, userId);
        },

        databaseError, recipeId, userId);
}
