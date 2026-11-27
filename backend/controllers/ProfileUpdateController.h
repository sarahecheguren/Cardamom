#pragma once

#include <drogon/HttpSimpleController.h>

class ProfileUpdateController : public drogon::HttpSimpleController<ProfileUpdateController> {
  public:
    void asyncHandleHttpRequest(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback) override;

    PATH_LIST_BEGIN
    PATH_ADD("/api/auth/me", drogon::Put);
    PATH_LIST_END
};