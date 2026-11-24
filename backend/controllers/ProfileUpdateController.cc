#include "ProfileUpdateController.h"

#include <drogon/drogon.h>

#include <regex>
#include <string>

#include "JsonResponder.h"
#include "Password.h"

namespace {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

}  // namespace

void ProfileUpdateController::asyncHandleHttpRequest(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    const auto respond = api::jsonResponder(callback);

    const auto reject = [respond](drogon::HttpStatusCode status, const std::string& message,
                                  const std::string& field) {
        Json::Value body;
        body["error"] = message;
        if (!field.empty()) {
            body["field"] = field;
        }
        respond(status, body);
    };

    const auto userId = api::sessionUserId(req);
    if (userId.empty()) {
        reject(drogon::k401Unauthorized, "You are not logged in.", "");
        return;
    }

    const auto input = req->getJsonObject();
    if (!input || !input->isObject()) {
        reject(drogon::k400BadRequest, "Please provide a JSON object.", "");
        return;
    }

    if (!(*input)["display_name"].isString()) {
        reject(drogon::k400BadRequest, "Display name is required.", "display_name");
        return;
    }

    if (!(*input)["email"].isString()) {
        reject(drogon::k400BadRequest, "Email address is required.", "email");
        return;
    }

    const auto displayName = trim((*input)["display_name"].asString());
    const auto email = trim((*input)["email"].asString());

    if (displayName.empty() || displayName.size() > 100 ||
        displayName.find('\0') != std::string::npos) {
        reject(drogon::k400BadRequest, "Display name is required and must be at most 100 bytes.",
               "display_name");
        return;
    }

    static const std::regex emailPattern(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)");
    if (email.size() > 254 || email.find('\0') != std::string::npos ||
        !std::regex_match(email, emailPattern)) {
        reject(drogon::k400BadRequest, "Please provide a valid email address.", "email");
        return;
    }

    const auto& passwordInput = (*input)["current_password"];
    if (!passwordInput.isNull() && !passwordInput.isString()) {
        reject(drogon::k400BadRequest, "Current password must be text.", "current_password");
        return;
    }

    const auto password = passwordInput.isString() ? passwordInput.asString() : "";
    if (password.size() > 1024) {
        reject(drogon::k400BadRequest, "Current password is too long.", "current_password");
        return;
    }

    const auto database = drogon::app().getDbClient("default");
    const auto databaseError = [reject](const drogon::orm::DrogonDbException&) {
        LOG_ERROR << "Profile update database query failed";
        reject(drogon::k503ServiceUnavailable, "Unable to update your account. Please try again.",
               "");
    };

    database->execSqlAsync(
        "SELECT display_name, email, password_hash FROM users WHERE id = ? LIMIT 1",
        [database, respond, reject, databaseError, req, userId, displayName, email,
         password](const drogon::orm::Result& result) {
            if (result.empty()) {
                req->session()->clear();
                reject(drogon::k401Unauthorized, "You are not logged in.", "");
                return;
            }

            const auto row = result[0];
            const auto oldName = row["display_name"].as<std::string>();
            const auto oldEmail = row["email"].as<std::string>();
            const auto passwordHash = row["password_hash"].as<std::string>();
            const bool emailChanged = email != oldEmail;

            if (emailChanged && trim(password).empty()) {
                reject(drogon::k400BadRequest,
                       "Enter your current password to change your email address.",
                       "current_password");
                return;
            }

            if (emailChanged && !auth::verifyPassword(password, passwordHash)) {
                reject(drogon::k403Forbidden, "Current password is incorrect.", "current_password");
                return;
            }

            const auto saved = [respond, displayName, email]() {
                Json::Value body;
                body["message"] = "Account information updated.";
                body["user"]["display_name"] = displayName;
                body["user"]["email"] = email;
                respond(drogon::k200OK, body);
            };

            if (displayName == oldName && !emailChanged) {
                saved();
                return;
            }

            // Check for a duplicate both before updating and after any update
            // failure. The database's unique key also protects simultaneous saves.
            const auto updateError = [database, reject, databaseError, email,
                                      userId](const drogon::orm::DrogonDbException&) {
                database->execSqlAsync(
                    "SELECT id FROM users WHERE email = ? AND id <> ? LIMIT 1",
                    [reject](const drogon::orm::Result& duplicate) {
                        if (!duplicate.empty()) {
                            reject(drogon::k409Conflict,
                                   "An account with this email already exists.", "email");
                            return;
                        }

                        LOG_ERROR << "Profile update failed";
                        reject(drogon::k503ServiceUnavailable,
                               "Unable to update your account. Please try again.", "");
                    },
                    databaseError, email, userId);
            };

            database->execSqlAsync(
                "SELECT id FROM users WHERE email = ? AND id <> ? LIMIT 1",
                [database, saved, reject, updateError, displayName, email, userId, oldName,
                 oldEmail, passwordHash, emailChanged](const drogon::orm::Result& duplicate) {
                    if (!duplicate.empty()) {
                        reject(drogon::k409Conflict, "An account with this email already exists.",
                               "email");
                        return;
                    }

                    // Only update the account if it has not changed since the
                    // password check. Subscription and recipe data are untouched.
                    database->execSqlAsync(
                        "UPDATE users SET display_name = ?, email = ?, "
                        "email_verified = CASE WHEN ? THEN FALSE ELSE email_verified END "
                        "WHERE id = ? AND BINARY display_name = BINARY ? "
                        "AND BINARY email = BINARY ? "
                        "AND BINARY password_hash = BINARY ?",
                        [saved, reject](const drogon::orm::Result& updated) {
                            if (updated.affectedRows() == 0) {
                                reject(drogon::k409Conflict,
                                       "Your account changed. Reload the page and try again.", "");
                                return;
                            }
                            saved();
                        },
                        updateError, displayName, email, emailChanged, userId, oldName, oldEmail,
                        passwordHash);
                },
                databaseError, email, userId);
        },
        databaseError, userId);
}