#include "RecipeController.h"

#include <drogon/drogon.h>

#include <string>

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

    const auto input = req->getJsonObject();

    if (!input || !input->isObject()) {
        reject(drogon::k400BadRequest, "Please provide a JSON object.");
        return;
    }

    auto trim = [](const std::string& value) {
        const auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return std::string();
        }

        const auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    };

    if (!(*input)["name"].isString() || !(*input)["ingredient_list"].isString()) {
        reject(drogon::k400BadRequest, "Recipe name and ingredients are required.");
        return;
    }

    const std::string name = trim((*input)["name"].asString());
    const std::string ingredients = trim((*input)["ingredient_list"].asString());

    if (name.empty() || name.size() > 200 || name.find('\0') != std::string::npos) {
        reject(drogon::k400BadRequest, "Recipe name is required and must be at most 200 bytes.");
        return;
    }

    if (ingredients.empty() || ingredients.size() > 65535 ||
        ingredients.find('\0') != std::string::npos) {
        reject(drogon::k400BadRequest, "Ingredients are required and must be at most 65535 bytes.");
        return;
    }

    std::string steps;
    std::string notes;

    auto readOptionalText = [&](const char* field, std::string& value) {
        const auto& item = (*input)[field];

        if (item.isNull()) {
            return true;
        }

        if (!item.isString()) {
            return false;
        }

        value = trim(item.asString());
        return value.size() <= 65535 && value.find('\0') == std::string::npos;
    };

    if (!readOptionalText("steps", steps) || !readOptionalText("notes", notes)) {
        reject(drogon::k400BadRequest, "Steps and notes must be text of at most 65535 bytes each.");
        return;
    }

    std::string servings;
    std::string prepMinutes;
    std::string cookMinutes;

    auto readOptionalNumber = [&](const char* field, unsigned int minimum, std::string& value) {
        const auto& item = (*input)[field];

        if (item.isNull()) {
            return true;
        }

        if (!item.isUInt()) {
            return false;
        }

        const auto number = item.asUInt();
        if (number < minimum || number > 65535) {
            return false;
        }

        value = std::to_string(number);
        return true;
    };

    if (!readOptionalNumber("servings", 1, servings) ||
        !readOptionalNumber("prep_minutes", 0, prepMinutes) ||
        !readOptionalNumber("cook_minutes", 0, cookMinutes)) {
        reject(drogon::k400BadRequest,
               "Servings must be an integer from 1 to 65535. "
               "Preparation and cooking times must be integers from 0 to 65535.");
        return;
    }

    auto database = drogon::app().getDbClient("default");

    database->execSqlAsync(
        "INSERT INTO recipes "
        "(user_id, name, ingredient_list, steps, notes, "
        "servings, prep_minutes, cook_minutes) "
        "VALUES (?, ?, ?, NULLIF(?, ''), NULLIF(?, ''), "
        "NULLIF(?, ''), NULLIF(?, ''), NULLIF(?, ''))",
        [respond](const drogon::orm::Result& result) {
            Json::Value body;
            body["message"] = "Recipe saved.";
            body["recipe"]["id"] = std::to_string(result.insertId());
            respond(drogon::k201Created, body);
        },
        [reject](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Recipe insert failed";
            reject(drogon::k503ServiceUnavailable, "Unable to save the recipe. Please try again.");
        },
        userId, name, ingredients, steps, notes, servings, prepMinutes, cookMinutes);
}