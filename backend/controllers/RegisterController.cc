#include "RegisterController.h"

#include <drogon/drogon.h>

#include <regex>
#include <string>

#include "Password.h"

void RegisterController::asyncHandleHttpRequest(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto respond = [callback](drogon::HttpStatusCode status, const std::string& text) {
        Json::Value body;
        body[status == drogon::k201Created ? "message" : "error"] = text;

        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(status);
        response->addHeader("Cache-Control", "no-store");
        callback(response);
    };

    const auto input = req->getJsonObject();

    if (!input || !input->isObject() || !(*input)["display_name"].isString() ||
        !(*input)["email"].isString() || !(*input)["password"].isString()) {
        respond(drogon::k400BadRequest, "Display name, email, and password are required.");
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

    const std::string displayName = trim((*input)["display_name"].asString());
    const std::string email = trim((*input)["email"].asString());
    const std::string password = (*input)["password"].asString();

    if (displayName.empty() || displayName.size() > 100 ||
        displayName.find('\0') != std::string::npos) {
        respond(drogon::k400BadRequest, "Display name is required and must be at most 100 bytes.");
        return;
    }

    static const std::regex emailPattern(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)");

    if (email.size() > 254 || email.find('\0') != std::string::npos ||
        !std::regex_match(email, emailPattern)) {
        respond(drogon::k400BadRequest, "Please provide a valid email address.");
        return;
    }

    if (password.size() < 8 || password.size() > 1024 || trim(password).empty()) {
        respond(drogon::k400BadRequest,
                "Password must be between 8 and 1024 bytes and cannot be blank.");
        return;
    }

    auto database = drogon::app().getDbClient("default");

    database->execSqlAsync(
        "SELECT id FROM users WHERE email = ? LIMIT 1",
        [database, respond, displayName, email, password](const drogon::orm::Result& result) {
            if (!result.empty()) {
                respond(drogon::k409Conflict, "An account with this email already exists.");
                return;
            }

            std::string passwordHash;
            try {
                passwordHash = auth::hashPassword(password);
            } catch (const std::exception&) {
                LOG_ERROR << "Registration password hashing failed";
                respond(drogon::k503ServiceUnavailable, "Registration is temporarily unavailable.");
                return;
            }

            database->execSqlAsync(
                "INSERT INTO users (display_name, email, password_hash) "
                "VALUES (?, ?, ?)",
                [respond](const drogon::orm::Result&) {
                    respond(drogon::k201Created, "Account created. You can now log in.");
                },
                [respond](const drogon::orm::DrogonDbException&) {
                    LOG_ERROR << "Registration insert failed";
                    respond(drogon::k503ServiceUnavailable,
                            "Unable to create the account. Please try again.");
                },
                displayName, email, passwordHash);
        },
        [respond](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Registration lookup failed";
            respond(drogon::k503ServiceUnavailable, "Registration is temporarily unavailable.");
        },
        email);
}