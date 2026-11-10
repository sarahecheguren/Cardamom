#include "RecipeDetailsController.h"

#include <drogon/drogon.h>

#include <charconv>
#include <cstdint>

void RecipeDetailsController::getRecipe(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string id) const {
    auto respond = [callback](drogon::HttpStatusCode status, const Json::Value& body) {
        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(status);
        response->addHeader("Cache-Control", "no-store");
        callback(response);
    };

    auto reject = [respond](drogon::HttpStatusCode status, const char* message) {
        Json::Value body;
        body["error"] = message;
        respond(status, body);
    };

    const auto userId = req->session()->get<std::string>("user_id");

    if (userId.empty()) {
        reject(drogon::k401Unauthorized, "You must log in to access recipes.");
        return;
    }

    std::uint32_t recipeId = 0;
    const auto* end = id.data() + id.size();
    const auto parsed = std::from_chars(id.data(), end, recipeId);

    if (parsed.ec != std::errc{} || parsed.ptr != end || id.front() == '0') {
        reject(drogon::k400BadRequest, "Invalid recipe ID.");
        return;
    }

    drogon::app().getDbClient("default")->execSqlAsync(
        "SELECT id, name, ingredient_list, steps, notes, servings, prep_minutes, cook_minutes "
        "FROM recipes WHERE id = ? AND user_id = ?",
        [respond, reject](const drogon::orm::Result& result) {
            if (result.empty()) {
                reject(drogon::k404NotFound, "Recipe not found.");
                return;
            }

            const auto row = result[0];
            Json::Value body;
            auto& recipe = body["recipe"];

            recipe["id"] = row["id"].as<std::string>();
            recipe["name"] = row["name"].as<std::string>();
            recipe["ingredient_list"] = row["ingredient_list"].as<std::string>();

            for (const char* field : {"steps", "notes"}) {
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

            respond(drogon::k200OK, body);
        },
        [reject](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Recipe details query failed";
            reject(drogon::k503ServiceUnavailable, "Unable to load this recipe. Please try again.");
        },
        recipeId, userId);
}
