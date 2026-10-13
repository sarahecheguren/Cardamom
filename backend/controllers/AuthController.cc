#include "AuthController.h"
#include "Password.h"
#include <drogon/drogon.h>

void AuthController::asyncHandleHttpRequest(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto input = req->getJsonObject();

    auto respond = [callback](drogon::HttpStatusCode status,
                              const Json::Value& body) {
        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(status);
        response->addHeader("Cache-Control", "no-store");
        callback(response);
    };

    if (!input || !input->isObject() ||
        !(*input)["email"].isString() ||
        !(*input)["password"].isString() ||
        (*input)["email"].asString().empty() ||
        (*input)["password"].asString().empty())
    {
        Json::Value body;
        body["error"] = "Email and password are required.";
        respond(drogon::k400BadRequest, body);
        return;
    }

    const std::string email = (*input)["email"].asString();
    const std::string password = (*input)["password"].asString();

    if (email.size() > 254 || password.size() > 1024)
    {
        Json::Value body;
        body["error"] = "Email or password is too long.";
        respond(drogon::k400BadRequest, body);
        return;
    }

    auto database = drogon::app().getDbClient("default");

    database->execSqlAsync(
        "SELECT id, email, password_hash FROM users WHERE email = ? LIMIT 1",
        [respond, password, req](const drogon::orm::Result& result) {
            Json::Value body;

            if (result.empty() ||
                !auth::verifyPassword(
                    password,
                    result[0]["password_hash"].as<std::string>()))
            {
                body["error"] = "Invalid email or password.";
                respond(drogon::k401Unauthorized, body);
                return;
            }

            auto session = req->session();
            session->clear();
            session->insert(
                "user_id", result[0]["id"].as<std::string>());
            session->changeSessionIdToClient();

            body["message"] = "Login successful.";
            body["user"]["id"] =
                result[0]["id"].as<std::string>();
            body["user"]["email"] =
                result[0]["email"].as<std::string>();

            respond(drogon::k200OK, body);
        },
        [respond](const drogon::orm::DrogonDbException&) {
            LOG_ERROR << "Login database query failed";

            Json::Value body;
            body["error"] = "Login is temporarily unavailable.";
            respond(drogon::k503ServiceUnavailable, body);
        },
        email);
}