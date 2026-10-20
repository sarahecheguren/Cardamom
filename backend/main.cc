#include <drogon/drogon.h>
#include <sodium.h>
#include <filesystem> 

#include <cstdlib>

#include "Password.h"

int main() {
    if (sodium_init() < 0) {
        LOG_ERROR << "Could not initialize libsodium";
        return 1;
    }

    const char* databasePassword = std::getenv("CARDAMOM_DB_PASSWORD");

    if (databasePassword == nullptr || databasePassword[0] == '\0') {
        LOG_ERROR << "CARDAMOM_DB_PASSWORD is not set";
        return 1;
    }

    drogon::app().createDbClient("mysql", "127.0.0.1", 3306, "cardamom_auth_dev", "cardamom_login",
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
