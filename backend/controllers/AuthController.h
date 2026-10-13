#pragma once

#include <drogon/HttpSimpleController.h>

class AuthController : public drogon::HttpSimpleController<AuthController>
{
  public:
    void asyncHandleHttpRequest(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        override;

    PATH_LIST_BEGIN
    PATH_ADD("/api/auth/login", drogon::Post);
    PATH_LIST_END
};