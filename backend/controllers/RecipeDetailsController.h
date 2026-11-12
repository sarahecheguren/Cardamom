#pragma once

#include <drogon/HttpController.h>

#include <functional>
#include <string>

class RecipeDetailsController : public drogon::HttpController<RecipeDetailsController> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RecipeDetailsController::getRecipe, "/api/recipes/{1}", drogon::Get);
    METHOD_LIST_END

    void getRecipe(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   std::string id) const;
};
