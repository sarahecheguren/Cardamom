#include "RecipeDetailsController.h"

#include <drogon/drogon.h>

#include <charconv>
#include <cstdint>

namespace {

class RecipeResponse {
  public:
    explicit RecipeResponse(const std::function<void(const drogon::HttpResponsePtr&)>& callback)
        : callback_(callback) {}

    void operator()(drogon::HttpStatusCode status, const Json::Value& body) const {
        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(status);
        response->addHeader("Cache-Control", "no-store");
        callback_(response);
    }

    void reject(drogon::HttpStatusCode status, const char* message) const {
        Json::Value body;
        body["error"] = message;
        (*this)(status, body);
    }

  private:
    std::function<void(const drogon::HttpResponsePtr&)> callback_;
};

bool parseRecipeId(const std::string& id, std::uint32_t& recipeId) {
    if (id.empty() || id.front() == '0') {
        return false;
    }

    const auto* end = id.data() + id.size();
    const auto parsed = std::from_chars(id.data(), end, recipeId);
    return parsed.ec == std::errc{} && parsed.ptr == end && recipeId != 0;
}

}  // namespace

void RecipeDetailsController::getRecipe(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string id) const {
    const RecipeResponse respond(callback);

    const auto userId = req->session()->get<std::string>("user_id");

    if (userId.empty()) {
        respond.reject(drogon::k401Unauthorized, "You must log in to access recipes.");
        return;
    }

    std::uint32_t recipeId = 0;
    if (!parseRecipeId(id, recipeId)) {
        respond.reject(drogon::k400BadRequest, "Invalid recipe ID.");
        return;
    }

    drogon::app().getDbClient("default")->execSqlAsync(
        "SELECT id, name, ingredient_list, steps, notes, servings, prep_minutes, cook_minutes "
        "FROM recipes WHERE id = ? AND user_id = ?",
        [respond](const drogon::orm::Result& result) {
            if (result.empty()) {
                respond.reject(drogon::k404NotFound, "Recipe not found.");
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
        [respond](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Recipe details query failed";
            respond.reject(drogon::k503ServiceUnavailable,
                           "Unable to load this recipe. Please try again.");
        },
        recipeId, userId);
}

void RecipeDetailsController::deleteRecipe(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string id) const {
    const RecipeResponse respond(callback);

    const auto userId = req->session()->get<std::string>("user_id");

    if (userId.empty()) {
        respond.reject(drogon::k401Unauthorized, "You must log in to delete recipes.");
        return;
    }

    std::uint32_t recipeId = 0;
    if (!parseRecipeId(id, recipeId)) {
        respond.reject(drogon::k400BadRequest, "Invalid recipe ID.");
        return;
    }

    drogon::app().getDbClient("default")->execSqlAsync(
        "DELETE FROM recipes WHERE id = ? AND user_id = ?",
        [respond](const drogon::orm::Result& result) {
            if (result.affectedRows() == 0) {
                respond.reject(drogon::k404NotFound, "Recipe not found.");
                return;
            }

            Json::Value body;
            body["message"] = "Recipe deleted.";
            respond(drogon::k200OK, body);
        },
        [respond](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Recipe deletion query failed";
            respond.reject(drogon::k503ServiceUnavailable,
                           "Unable to delete this recipe. Please try again.");
        },
        recipeId, userId);
}