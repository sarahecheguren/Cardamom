#pragma once

#include <drogon/HttpController.h>

#include <functional>
#include <string>

class DiscoverController : public drogon::HttpController<DiscoverController> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(DiscoverController::listRecipes, "/api/discover", drogon::Get);
    ADD_METHOD_TO(DiscoverController::getRecipe, "/api/discover/{1}", drogon::Get);
    METHOD_LIST_END

    void listRecipes(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

    void getRecipe(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   std::string id) const;
};
