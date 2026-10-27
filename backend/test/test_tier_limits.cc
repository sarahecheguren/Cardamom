#include <drogon/drogon.h>
#include <drogon/drogon_test.h>
#include <json/json.h>
#include <future>
#include <string>

std::string extractSessionCookie(const drogon::HttpResponsePtr& resp) {
    auto cookies = resp->getCookies();
    auto it = cookies.find("JSESSIONID");
    if (it != cookies.end()) {
        return "JSESSIONID=" + it->second.getValue() + ";";
    }
    return resp->getHeader("Set-Cookie");
}

DROGON_TEST(CardamomTierBoundariesQA) {

    // Aligned to the main.cc port
    auto client = drogon::HttpClient::newHttpClient("http://127.0.0.1:8080");

    // TEST 1: Free Tier Limit:

    {
        Json::Value loginJson;
        loginJson["email"] = "maya.com";
        loginJson["password"] = "Secret123!";

        auto loginReq = drogon::HttpRequest::newHttpJsonRequest(loginJson);
        loginReq->setPath("/api/auth/login"); // Standardized auth prefix routing
        loginReq->setMethod(drogon::Post);

        std::promise<std::pair<drogon::HttpStatusCode, std::string>> loginPromise;
        auto loginFuture = loginPromise.get_future();

        client->sendRequest(loginReq, [&loginPromise](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                loginPromise.set_value({resp->statusCode(), extractSessionCookie(resp)});
            } else {
                loginPromise.set_value({drogon::k500InternalServerError, ""});
            }
        });

        auto loginRes = loginFuture.get();
        REQUIRE(loginRes.first == drogon::k200OK);
        std::string sessionCookie = loginRes.second;

        // Attempting to inject the 26th recipe block
        Json::Value recipeJson;
        recipeJson["title"] = "QA Limit Breaker Test";
        recipeJson["content"] = "Should be rejected because the customer is already at 25 recipes.";

        auto recipeReq = drogon::HttpRequest::newHttpJsonRequest(recipeJson);
        recipeReq->setPath("/api/recipes");
        recipeReq->setMethod(drogon::Post);
        recipeReq->addHeader("Cookie", sessionCookie);

        std::promise<drogon::HttpStatusCode> recipePromise;
        auto recipeFuture = recipePromise.get_future();

        client->sendRequest(recipeReq, [&recipePromise](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                recipePromise.set_value(resp->statusCode());
            } else {
                recipePromise.set_value(drogon::k500InternalServerError);
            }
        });

        // ASSERTION: Verify the tier rule blocks the save request
        CHECK(recipeFuture.get() == drogon::k403Forbidden);
    }


    // TEST 2: Premium Tier Limit Bypass Verification:

    {
        Json::Value loginJson;
        loginJson["email"] = "Priya@example.com";
        loginJson["password"] = "Secret123!";

        auto loginReq = drogon::HttpRequest::newHttpJsonRequest(loginJson);
        loginReq->setPath("/api/login");
        loginReq->setMethod(drogon::Post);

        std::promise<std::pair<drogon::HttpStatusCode, std::string>> loginPromise;
        auto loginFuture = loginPromise.get_future();

        client->sendRequest(loginReq, [&loginPromise](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                loginPromise.set_value({resp->statusCode(), extractSessionCookie(resp)});
            } else {
                loginPromise.set_value({drogon::k500InternalServerError, ""});
            }
        });

        auto loginRes = loginFuture.get();
        REQUIRE(loginRes.first == drogon::k200OK);
        std::string sessionCookie = loginRes.second;

        // Attempt 31st recipe creation
        Json::Value recipeJson;
        recipeJson["title"] = "Premium QA Verification Entry";
        recipeJson["content"] = "Premium users bypass the 25 recipe wall.";

        auto recipeReq = drogon::HttpRequest::newHttpJsonRequest(recipeJson);
        recipeReq->setPath("/api/recipes");
        recipeReq->setMethod(drogon::Post);
        recipeReq->addHeader("Cookie", sessionCookie);

        std::promise<drogon::HttpStatusCode> recipePromise;
        auto recipeFuture = recipePromise.get_future();

        client->sendRequest(recipeReq, [&recipePromise](drogon::ReqResult res, const drogon::HttpResponsePtr& resp) {
            if (res == drogon::ReqResult::Ok && resp) {
                recipePromise.set_value(resp->statusCode());
            } else {
                recipePromise.set_value(drogon::k500InternalServerError);
            }
        });

        // ASSERTION: Premium tier requests successfully pass the threshold of 25 recipes:
        CHECK(recipeFuture.get() == drogon::k201Created);
    }
}
