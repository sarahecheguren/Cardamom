#include "ProfileController.h"

#include <drogon/drogon.h>

#include <cstdint>
#include <string>

#include "JsonResponder.h"

void ProfileController::asyncHandleHttpRequest(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto respond = api::jsonResponder(callback);
    auto reject = api::errorResponder(respond);

    const auto userId = api::sessionUserId(req);

    if (userId.empty()) {
        reject(drogon::k401Unauthorized, "You are not logged in.");
        return;
    }

    auto database = drogon::app().getDbClient("default");

    database->execSqlAsync(
        "SELECT u.id, u.display_name, u.email, t.tier_name, t.recipe_limit, "
        "(SELECT COUNT(*) FROM recipes r WHERE r.user_id = u.id) + "
        "(SELECT COUNT(*) FROM recipe_favorites f WHERE f.user_id = u.id) AS recipe_count "
        "FROM users u JOIN user_tiers t ON t.id = u.tier_id "
        "WHERE u.id = ? LIMIT 1",
        [respond, reject, req](const drogon::orm::Result& result) {
            if (result.empty()) {
                // The account was deleted while its session was still alive.
                req->session()->clear();
                reject(drogon::k401Unauthorized, "You are not logged in.");
                return;
            }

            const auto& row = result[0];

            Json::Value body;
            auto& user = body["user"];
            user["id"] = row["id"].as<std::string>();
            user["display_name"] = row["display_name"].as<std::string>();
            user["email"] = row["email"].as<std::string>();
            user["tier"] = row["tier_name"].as<std::string>();
            user["recipe_limit"] = row["recipe_limit"].as<unsigned int>();
            user["recipe_count"] =
                static_cast<Json::UInt64>(row["recipe_count"].as<std::uint64_t>());

            respond(drogon::k200OK, body);
        },
        [reject](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Profile query failed";
            reject(drogon::k503ServiceUnavailable, "Your profile is temporarily unavailable.");
        },
        userId);
}
