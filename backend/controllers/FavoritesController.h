#pragma once

#include <drogon/HttpController.h>

#include <functional>
#include <string>

class FavoritesController : public drogon::HttpController<FavoritesController> {
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(FavoritesController::listFavorites, "/api/favorites", drogon::Get);
    ADD_METHOD_TO(FavoritesController::changeFavorite, "/api/favorites/{1}", drogon::Post,
                  drogon::Delete);
    METHOD_LIST_END

    void listFavorites(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) const;

    void changeFavorite(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                        std::string id) const;
};
