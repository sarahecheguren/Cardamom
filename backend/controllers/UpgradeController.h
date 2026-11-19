#pragma once

#include <drogon/HttpSimpleController.h>

// Moves the logged-in user from the Free tier to Premium. There is no payment
// step: the frontend asks the user to confirm, then calls this endpoint.
class UpgradeController : public drogon::HttpSimpleController<UpgradeController> {
  public:
    void asyncHandleHttpRequest(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback) override;

    PATH_LIST_BEGIN
    PATH_ADD("/api/account/upgrade", drogon::Post);
    PATH_LIST_END
};
