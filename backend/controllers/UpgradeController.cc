#include "UpgradeController.h"

#include <drogon/drogon.h>

#include <string>

#include "JsonResponder.h"

void UpgradeController::asyncHandleHttpRequest(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto respond = api::jsonResponder(callback);
    auto reject = api::errorResponder(respond);

    const auto userId = api::sessionUserId(req);

    if (userId.empty()) {
        reject(drogon::k401Unauthorized, "You must log in to upgrade your plan.");
        return;
    }

    auto databaseError = [reject](const drogon::orm::DrogonDbException&) {
        LOG_ERROR << "Upgrade query failed";
        reject(drogon::k503ServiceUnavailable, "Unable to upgrade your plan. Please try again.");
    };

    auto database = drogon::app().getDbClient("default");

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
