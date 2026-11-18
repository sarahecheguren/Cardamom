#pragma once

#include <drogon/HttpSimpleController.h>

class RecipeController : public drogon::HttpSimpleController<RecipeController> {
  public:
    void asyncHandleHttpRequest(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback) override;

    PATH_LIST_BEGIN
    PATH_ADD("/api/recipes", drogon::Post, drogon::Get);
    PATH_LIST_END
};