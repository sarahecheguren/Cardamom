#include "RecipeController.h"

#include <drogon/drogon.h>

#include <string>

#include "RecipeInput.h"

void RecipeController::asyncHandleHttpRequest(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto respond = [callback](drogon::HttpStatusCode status, const Json::Value& body) {
        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(status);
        response->addHeader("Cache-Control", "no-store");
        callback(response);
    };

    auto reject = [respond](drogon::HttpStatusCode status, const std::string& message) {
        Json::Value body;
        body["error"] = message;
        respond(status, body);
    };

    const auto userId = req->session()->get<std::string>("user_id");

    if (userId.empty()) {
        reject(drogon::k401Unauthorized, "You must log in to access recipes.");
        return;
    }

    if (req->method() == drogon::Get) {
        auto database = drogon::app().getDbClient("default");

        database->execSqlAsync(
            "SELECT id, name, ingredient_list, servings, prep_minutes, cook_minutes "
            "FROM recipes WHERE user_id = ? ORDER BY name ASC, id ASC",
            [respond](const drogon::orm::Result& result) {
                Json::Value body;
                body["recipes"] = Json::Value(Json::arrayValue);

                for (const auto& row : result) {
                    Json::Value recipe;
                    recipe["id"] = row["id"].as<std::string>();
                    recipe["name"] = row["name"].as<std::string>();
                    recipe["ingredient_list"] = row["ingredient_list"].as<std::string>();

                    for (const char* field : {"servings", "prep_minutes", "cook_minutes"}) {
                        if (row[field].isNull()) {
                            recipe[field] = Json::Value();
                        } else {
                            recipe[field] = row[field].as<unsigned int>();
                        }
                    }

                    body["recipes"].append(recipe);
                }

                respond(drogon::k200OK, body);
            },
            [reject](const drogon::orm::DrogonDbException&) {
                LOG_ERROR << "Recipe collection query failed";
                reject(drogon::k503ServiceUnavailable,
                       "Unable to load your recipes. Please try again.");
            },
            userId);

        return;
    }

    api::RecipeInput recipe;
    std::string inputError;

    if (!api::readRecipeInput(req->getJsonObject(), recipe, inputError)) {
        reject(drogon::k400BadRequest, inputError);
        return;
    }

    auto database = drogon::app().getDbClient("default");

    // The plan limit is checked in the same statement as the insert, so the row
    // is only written while the user is below it. Recipes the user owns and
    // recipes they have favorited both count toward the limit.
    database->execSqlAsync(
        "INSERT INTO recipes "
        "(user_id, name, ingredient_list, steps, notes, "
        "servings, prep_minutes, cook_minutes) "
        "SELECT u.id, ?, ?, NULLIF(?, ''), NULLIF(?, ''), "
        "NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, '') "
        "FROM users u "
        "JOIN user_tiers t ON t.id = u.tier_id "
        "WHERE u.id = ? "
        "AND (SELECT COUNT(*) FROM recipes r WHERE r.user_id = u.id) + "
        "(SELECT COUNT(*) FROM recipe_favorites f WHERE f.user_id = u.id) < t.recipe_limit",
        [respond, reject, database, userId](const drogon::orm::Result& result) {
            if (result.affectedRows() > 0) {
                Json::Value body;
                body["message"] = "Recipe saved.";
                body["recipe"]["id"] = std::to_string(result.insertId());
                respond(drogon::k201Created, body);
                return;
            }

            // Nothing was inserted. Look up the plan to explain why.
            database->execSqlAsync(
                "SELECT t.tier_name, t.recipe_limit, "
                "(SELECT b.tier_name FROM user_tiers b "
                "WHERE b.recipe_limit > t.recipe_limit "
                "ORDER BY b.recipe_limit ASC LIMIT 1) AS bigger_tier "
                "FROM users u "
                "JOIN user_tiers t ON t.id = u.tier_id "
                "WHERE u.id = ?",
                [respond, reject](const drogon::orm::Result& plan) {
                    if (plan.empty()) {
                        // The account was deleted while its session was still open.
                        reject(drogon::k401Unauthorized, "You must log in to access recipes.");
                        return;
                    }

                    const auto tierName = plan[0]["tier_name"].as<std::string>();
                    const auto recipeLimit = plan[0]["recipe_limit"].as<unsigned int>();

                    std::string message = "You have reached the " + tierName + " plan limit of " +
                                          std::to_string(recipeLimit) + " recipes. ";

                    if (plan[0]["bigger_tier"].isNull()) {
                        message += "Delete a recipe to make room for a new one.";
                    } else {
                        message += plan[0]["bigger_tier"].as<std::string>() +
                                   " offers more room, or you can delete a recipe to make room "
                                   "for a new one.";
                    }

                    Json::Value body;
                    body["error"] = message;
                    body["limit_reached"] = true;
                    body["tier_name"] = tierName;
                    body["recipe_limit"] = recipeLimit;
                    respond(drogon::k403Forbidden, body);
                },
                [reject](const drogon::orm::DrogonDbException&) {
                    LOG_ERROR << "Recipe limit lookup failed";
                    reject(drogon::k503ServiceUnavailable,
                           "Unable to save the recipe. Please try again.");
                },
                userId);
        },
        [reject](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Recipe insert failed";
            reject(drogon::k503ServiceUnavailable, "Unable to save the recipe. Please try again.");
        },
        recipe.name, recipe.ingredients, recipe.steps, recipe.notes, recipe.servings,
        recipe.prepMinutes, recipe.cookMinutes, userId);
}