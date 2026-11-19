#include <drogon/drogon.h>
#include <sodium.h>

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "Password.h"

int main() {
    if (sodium_init() < 0) {
        LOG_ERROR << "Could not initialize libsodium";
        return 1;
    }

    auto readSetting = [](const char* name, const char* fallback) {
        const char* value = std::getenv(name);
        return std::string(value && value[0] != '\0' ? value : fallback);
    };

    const std::string databaseHost = readSetting("DB_HOST", "127.0.0.1");
    const std::string databaseName = readSetting("DB_NAME", "cardamom");
    const std::string databaseUser = readSetting("DB_USER", "cardamom");
    const std::string databasePassword = readSetting("DB_PASSWORD", "");
    const std::string portText = readSetting("DB_PORT", "3306");

    if (databasePassword.empty()) {
        LOG_ERROR << "DB_PASSWORD is not set";
        return 1;
    }

    int databasePort;
    try {
        std::size_t consumed = 0;
        databasePort = std::stoi(portText, &consumed);
        if (consumed != portText.size() || databasePort < 1 || databasePort > 65535) {
            throw std::invalid_argument("Invalid port");
        }
    } catch (const std::exception&) {
        LOG_ERROR << "DB_PORT must be a number between 1 and 65535";
        return 1;
    }

    drogon::app().createDbClient("mysql", databaseHost, databasePort, databaseName, databaseUser,
                                 databasePassword, 1, "", "default");

    drogon::app().registerHandler(
        "/api/health",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            Json::Value body;
            body["status"] = "ok";
            body["message"] = "Cardamom backend is running";

            auto response = drogon::HttpResponse::newHttpJsonResponse(body);
            callback(response);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/auth/me",
        [](const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto respond = [callback](drogon::HttpStatusCode status, const Json::Value& body) {
                auto response = drogon::HttpResponse::newHttpJsonResponse(body);
                response->setStatusCode(status);
                response->addHeader("Cache-Control", "no-store");
                callback(response);
            };

            const auto userId = req->session()->get<std::string>("user_id");

            if (userId.empty()) {
                Json::Value body;
                body["error"] = "You are not logged in.";
                respond(drogon::k401Unauthorized, body);
                return;
            }

            drogon::app().getDbClient("default")->execSqlAsync(
                "SELECT u.id, u.display_name, u.email, t.tier_name, t.recipe_limit, "
                "(SELECT COUNT(*) FROM recipes r WHERE r.user_id = u.id) + "
                "(SELECT COUNT(*) FROM recipe_favorites f WHERE f.user_id = u.id) "
                "AS recipe_count "
                "FROM users u JOIN user_tiers t ON t.id = u.tier_id "
                "WHERE u.id = ? LIMIT 1",
                [respond, req](const drogon::orm::Result& result) {
                    Json::Value body;

                    if (result.empty()) {
                        req->session()->clear();
                        req->session()->changeSessionIdToClient();
                        body["error"] = "You are not logged in.";
                        respond(drogon::k401Unauthorized, body);
                        return;
                    }

                    const auto row = result[0];
                    auto& user = body["user"];

                    user["id"] = row["id"].as<std::string>();
                    user["display_name"] = row["display_name"].as<std::string>();
                    user["email"] = row["email"].as<std::string>();
                    user["tier_name"] = row["tier_name"].as<std::string>();
                    user["recipe_limit"] = row["recipe_limit"].as<unsigned int>();
                    user["recipe_count"] =
                        Json::UInt64(row["recipe_count"].as<unsigned long long>());

                    respond(drogon::k200OK, body);
                },
                [respond](const drogon::orm::DrogonDbException&) {
                    LOG_ERROR << "Profile database query failed";

                    Json::Value body;
                    body["error"] = "Unable to load your account. Please try again.";
                    respond(drogon::k503ServiceUnavailable, body);
                },
                userId);
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/auth/logout",
        [](const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            req->session()->clear();
            req->session()->changeSessionIdToClient();

            Json::Value body;
            body["message"] = "Logout successful.";

            auto response = drogon::HttpResponse::newHttpJsonResponse(body);
            response->addHeader("Cache-Control", "no-store");
            callback(response);
        },
        {drogon::Post});

    drogon::app().enableSession(1800, drogon::Cookie::SameSite::kStrict);

    const std::string frontendDir = readSetting("FRONTEND_DIR", "./frontend");
    if (!std::filesystem::is_directory(frontendDir)) {
        LOG_WARN << "Frontend directory not found at " << frontendDir
                 << ". Run from the repository root or set FRONTEND_DIR.";
    }
    drogon::app().setDocumentRoot(frontendDir);

    drogon::app().addListener("0.0.0.0", 8080);
    drogon::app().run();
    return 0;
}
