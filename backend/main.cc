#include <drogon/drogon.h>
#include <sodium.h>

#include <cstdlib>
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
            const auto userId = req->session()->get<std::string>("user_id");

            Json::Value body;
            auto status = drogon::k200OK;

            if (userId.empty()) {
                body["error"] = "You are not logged in.";
                status = drogon::k401Unauthorized;
            } else {
                body["user"]["id"] = userId;
            }

            auto response = drogon::HttpResponse::newHttpJsonResponse(body);
            response->setStatusCode(status);
            response->addHeader("Cache-Control", "no-store");
            callback(response);
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

    drogon::app().addListener("0.0.0.0", 8080);
    drogon::app().run();
    return 0;
}