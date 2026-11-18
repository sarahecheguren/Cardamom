#include "UpgradeController.h"

#include <drogon/drogon.h>

#include <string>

void UpgradeController::asyncHandleHttpRequest(
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
        reject(drogon::k401Unauthorized, "You must log in to upgrade your plan.");
        return;
    }

    auto database = drogon::app().getDbClient("default");

    auto databaseError = [reject](const drogon::orm::DrogonDbException&) {
        LOG_ERROR << "Upgrade query failed";
        reject(drogon::k503ServiceUnavailable, "Unable to upgrade your plan. Please try again.");
    };

    // The tier check is part of the UPDATE, so a user who is already on Premium
    // changes no rows.
    database->execSqlAsync(
        "UPDATE users u JOIN user_tiers t ON t.tier_name = 'Premium' "
        "SET u.tier_id = t.id "
        "WHERE u.id = ? AND u.tier_id <> t.id",
        [respond, reject, req, database, databaseError, userId](const drogon::orm::Result& update) {
            const bool upgraded = update.affectedRows() > 0;

            database->execSqlAsync(
                "SELECT t.tier_name, t.recipe_limit "
                "FROM users u JOIN user_tiers t ON t.id = u.tier_id "
                "WHERE u.id = ? LIMIT 1",
                [respond, reject, req, upgraded](const drogon::orm::Result& result) {
                    if (result.empty()) {
                        // The account was deleted while its session was still alive.
                        req->session()->clear();
                        reject(drogon::k401Unauthorized, "You are not logged in.");
                        return;
                    }

                    if (!upgraded) {
                        reject(drogon::k409Conflict, "You are already on the Premium plan.");
                        return;
                    }

                    const auto& row = result[0];

                    Json::Value body;
                    body["message"] = "You are now on the Premium plan.";
                    body["user"]["tier"] = row["tier_name"].as<std::string>();
                    body["user"]["recipe_limit"] = row["recipe_limit"].as<unsigned int>();

                    respond(drogon::k200OK, body);
                },
                databaseError, userId);
        },
        databaseError, userId);
}
