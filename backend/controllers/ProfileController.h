#pragma once

#include <drogon/HttpSimpleController.h>

// Returns the logged-in user's profile: name, email, tier, and recipe count.
class ProfileController : public drogon::HttpSimpleController<ProfileController> {
  public:
    void asyncHandleHttpRequest(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback) override;

    PATH_LIST_BEGIN
    PATH_ADD("/api/auth/me", drogon::Get);
    PATH_LIST_END
};
