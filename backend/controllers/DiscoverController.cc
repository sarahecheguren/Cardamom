#include "DiscoverController.h"

#include <drogon/drogon.h>

#include <charconv>
#include <cstdint>

#include "JsonResponder.h"

namespace {

// Converte uma receita publica em JSON
Json::Value publicRecipe(const drogon::orm::Row& row) {
    Json::Value recipe;

    recipe["id"] = row["id"].as<std::string>();
    recipe["name"] = row["name"].as<std::string>();
    recipe["author"] = row["author"].as<std::string>();
    recipe["visibility"] = "public";

    for (const char* field : {"ingredient_list", "steps", "notes", "image_url"}) {
        if (row[field].isNull()) {
            recipe[field] = Json::Value();
        } else {
            recipe[field] = row[field].as<std::string>();
        }
    }

    for (const char* field : {"servings", "prep_minutes", "cook_minutes"}) {
        if (row[field].isNull()) {
            recipe[field] = Json::Value();
        } else {
            recipe[field] = row[field].as<unsigned int>();
        }
    }

    return recipe;
}

}  // namespace

// Lista as receitas compartilhadas pela comunidade
void DiscoverController::listRecipes(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    auto respond = api::jsonResponder(callback);
    auto reject = api::errorResponder(respond);

    if (api::sessionUserId(req).empty()) {
        reject(drogon::k401Unauthorized, "You must log in to discover recipes.");
        return;
    }

    drogon::app().getDbClient("default")->execSqlAsync(
        "SELECT r.id, r.name, r.ingredient_list, r.steps, r.notes, "
        "r.image_url, r.servings, r.prep_minutes, r.cook_minutes, "
        "u.display_name AS author "
        "FROM recipes r "
        "JOIN users u ON u.id = r.user_id "
        "WHERE r.visibility = 'public' "
        "ORDER BY r.created_at DESC, r.id DESC LIMIT 100",

        [respond](const drogon::orm::Result& result) {
            Json::Value body;
            body["recipes"] = Json::Value(Json::arrayValue);

            for (const auto& row : result) {
                body["recipes"].append(publicRecipe(row));
            }

            respond(drogon::k200OK, body);
        },

        [reject](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Discover query failed";
            reject(drogon::k503ServiceUnavailable, "Unable to load public recipes.");
        });
}

// Abre uma receita publica sem permitir alteracoes
void DiscoverController::getRecipe(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                   std::string id) const {
    auto respond = api::jsonResponder(callback);
    auto reject = api::errorResponder(respond);

    if (api::sessionUserId(req).empty()) {
        reject(drogon::k401Unauthorized, "You must log in to discover recipes.");
        return;
    }

    std::uint32_t recipeId = 0;
    const auto* end = id.data() + id.size();
    const auto parsed = std::from_chars(id.data(), end, recipeId);

    if (id.empty() || parsed.ec != std::errc{} || parsed.ptr != end || recipeId == 0) {
        reject(drogon::k400BadRequest, "Invalid recipe ID.");
        return;
    }

    drogon::app().getDbClient("default")->execSqlAsync(
        "SELECT r.id, r.name, r.ingredient_list, r.steps, r.notes, "
        "r.image_url, r.servings, r.prep_minutes, r.cook_minutes, "
        "u.display_name AS author "
        "FROM recipes r "
        "JOIN users u ON u.id = r.user_id "
        "WHERE r.id = ? AND r.visibility = 'public'",

        [respond, reject](const drogon::orm::Result& result) {
            if (result.empty()) {
                reject(drogon::k404NotFound, "Recipe not found.");
                return;
            }

            Json::Value body;
            body["recipe"] = publicRecipe(result[0]);

            respond(drogon::k200OK, body);
        },

        [reject](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Public recipe query failed";
            reject(drogon::k503ServiceUnavailable, "Unable to load this recipe.");
        },

        recipeId);
}
