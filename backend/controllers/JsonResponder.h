#pragma once

#include <drogon/drogon.h>

#include <functional>
#include <string>

// Helpers shared by the JSON API controllers.
namespace api {

// Returns a function that sends `body` as an uncached JSON response.
inline auto jsonResponder(std::function<void(const drogon::HttpResponsePtr&)> callback) {
    return [callback](drogon::HttpStatusCode status, const Json::Value& body) {
        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(status);
        response->addHeader("Cache-Control", "no-store");
        callback(response);
    };
}

// Returns a function that sends {"error": message} through `respond`.
template <typename Respond>
auto errorResponder(Respond respond) {
    return [respond](drogon::HttpStatusCode status, const std::string& message) {
        Json::Value body;
        body["error"] = message;
        respond(status, body);
    };
}

// The logged-in user's ID, or an empty string when there is no login.
inline std::string sessionUserId(const drogon::HttpRequestPtr& req) {
    return req->session()->get<std::string>("user_id");
}

}  // namespace api
