#pragma once

#include <drogon/HttpSimpleController.h>

class RegisterController : public drogon::HttpSimpleController<RegisterController> {
  public:
    void asyncHandleHttpRequest(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback) override;

    PATH_LIST_BEGIN
    PATH_ADD("/api/auth/register", drogon::Post);
    PATH_LIST_END
};