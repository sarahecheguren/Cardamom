// HTTP tests for the register, login, and recipe controllers. They send real
// requests to the backend instance that test_main.cc starts.

#include <drogon/drogon.h>
#include <drogon/drogon_test.h>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "test_support.h"

namespace {

using drogon::HttpClientPtr;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

const char* const kPassword = "Secret123!";

// Each client keeps its own cookies, so each client is a separate browser.
HttpClientPtr newClient() {
    auto client = drogon::HttpClient::newHttpClient(testsupport::baseUrl());
    client->enableCookies();
    return client;
}

HttpResponsePtr send(const HttpClientPtr& client, const drogon::HttpRequestPtr& request) {
    const auto result = client->sendRequest(request, 10);
    return result.first == drogon::ReqResult::Ok ? result.second : nullptr;
}

HttpResponsePtr get(const HttpClientPtr& client, const std::string& path) {
    auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Get);
    request->setPath(path);
    return send(client, request);
}

HttpResponsePtr post(const HttpClientPtr& client, const std::string& path,
                     const Json::Value& body) {
    auto request = drogon::HttpRequest::newHttpJsonRequest(body);
    request->setMethod(drogon::Post);
    request->setPath(path);
    return send(client, request);
}

HttpResponsePtr put(const HttpClientPtr& client, const std::string& path, const Json::Value& body) {
    auto request = drogon::HttpRequest::newHttpJsonRequest(body);
    request->setMethod(drogon::Put);
    request->setPath(path);
    return send(client, request);
}

// Sends the text as-is with a JSON content type, for bodies that a Json::Value cannot produce.
HttpResponsePtr postRaw(const HttpClientPtr& client, const std::string& path,
                        const std::string& body) {
    auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Post);
    request->setPath(path);
    request->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    request->setBody(body);
    return send(client, request);
}

// Prints the label when the status is wrong, so a failed CHECK inside a loop names its case.
bool hasStatus(const HttpResponsePtr& response, HttpStatusCode expected, const std::string& label) {
    if (response && response->statusCode() == expected) {
        return true;
    }

    std::cerr << "  [" << label << "] expected status " << expected << ", got "
              << (response ? std::to_string(response->statusCode()) : "no response") << "\n";
    return false;
}

// An error response has a JSON body with a non-empty "error" and must not be cached.
bool isError(const HttpResponsePtr& response, HttpStatusCode expected, const std::string& label) {
    if (!hasStatus(response, expected, label)) {
        return false;
    }

    const auto body = response->getJsonObject();
    if (!body || !(*body)["error"].isString() || (*body)["error"].asString().empty() ||
        response->getHeader("Cache-Control") != "no-store") {
        std::cerr << "  [" << label << "] missing error message or Cache-Control: no-store\n";
        return false;
    }

    return true;
}

Json::Value registration(const std::string& displayName, const std::string& email,
                         const std::string& password) {
    Json::Value body;
    body["display_name"] = displayName;
    body["email"] = email;
    body["password"] = password;
    return body;
}

Json::Value credentials(const std::string& email, const std::string& password) {
    Json::Value body;
    body["email"] = email;
    body["password"] = password;
    return body;
}

Json::Value recipe(const std::string& name, const std::string& ingredients) {
    Json::Value body;
    body["name"] = name;
    body["ingredient_list"] = ingredients;
    return body;
}

std::string testEmail(const std::string& label) {
    return "apitest-" + testsupport::runId() + "-" + label + "@cardamom.test";
}

// Registers a new account and logs the client in. Returns false if either step fails.
bool signIn(const HttpClientPtr& client, const std::string& label) {
    const std::string email = testEmail(label);

    return hasStatus(post(client, "/api/auth/register", registration(label, email, kPassword)),
                     drogon::k201Created, label + " register") &&
           hasStatus(post(client, "/api/auth/login", credentials(email, kPassword)), drogon::k200OK,
                     label + " login");
}

// Saves a recipe and returns its ID, or an empty string if the save failed.
std::string saveRecipe(const HttpClientPtr& client, const Json::Value& body) {
    const auto response = post(client, "/api/recipes", body);
    if (!hasStatus(response, drogon::k201Created, "save " + body["name"].asString())) {
        return "";
    }

    const auto json = response->getJsonObject();
    return json ? (*json)["recipe"]["id"].asString() : "";
}

}  // namespace

// ---------------------------------------------------------------------------
// Tests that do not need the database: these requests are rejected before any
// query runs.
// ---------------------------------------------------------------------------

DROGON_TEST(RegisterRejectsInvalidInput) {
    const auto client = newClient();
    const std::string path = "/api/auth/register";
    const std::string email = "new-user@example.com";

    CHECK(isError(postRaw(client, path, ""), drogon::k400BadRequest, "empty body"));
    CHECK(isError(postRaw(client, path, "not json"), drogon::k400BadRequest, "body is not JSON"));
    CHECK(isError(postRaw(client, path, "[]"), drogon::k400BadRequest, "body is an array"));

    Json::Value missingName = registration("Alice", email, kPassword);
    missingName.removeMember("display_name");
    CHECK(isError(post(client, path, missingName), drogon::k400BadRequest, "no display name"));

    Json::Value missingEmail = registration("Alice", email, kPassword);
    missingEmail.removeMember("email");
    CHECK(isError(post(client, path, missingEmail), drogon::k400BadRequest, "no email"));

    Json::Value missingPassword = registration("Alice", email, kPassword);
    missingPassword.removeMember("password");
    CHECK(isError(post(client, path, missingPassword), drogon::k400BadRequest, "no password"));

    Json::Value numericPassword = registration("Alice", email, kPassword);
    numericPassword["password"] = 12345678;
    CHECK(isError(post(client, path, numericPassword), drogon::k400BadRequest, "numeric password"));

    const std::vector<std::pair<std::string, std::string>> displayNames = {
        {"empty display name", ""},
        {"blank display name", " \t\r\n"},
        {"101-byte display name", std::string(101, 'a')},
    };
    for (const auto& [label, displayName] : displayNames) {
        CHECK(isError(post(client, path, registration(displayName, email, kPassword)),
                      drogon::k400BadRequest, label));
    }

    const std::vector<std::pair<std::string, std::string>> emails = {
        {"empty email", ""},
        {"email without @", "alice.example.com"},
        {"email without a dot in the domain", "alice@example"},
        {"email with two @", "alice@home@example.com"},
        {"email with a space", "alice smith@example.com"},
        {"email without a local part", "@example.com"},
        {"255-byte email", std::string(243, 'a') + "@example.com"},
    };
    for (const auto& [label, badEmail] : emails) {
        CHECK(isError(post(client, path, registration("Alice", badEmail, kPassword)),
                      drogon::k400BadRequest, label));
    }

    const std::vector<std::pair<std::string, std::string>> passwords = {
        {"empty password", ""},
        {"7-byte password", "Secret1"},
        {"blank 8-byte password", std::string(8, ' ')},
        {"1025-byte password", std::string(1025, 'p')},
    };
    for (const auto& [label, password] : passwords) {
        CHECK(isError(post(client, path, registration("Alice", email, password)),
                      drogon::k400BadRequest, label));
    }
}

DROGON_TEST(LoginRejectsInvalidInput) {
    const auto client = newClient();
    const std::string path = "/api/auth/login";

    CHECK(isError(postRaw(client, path, ""), drogon::k400BadRequest, "empty body"));
    CHECK(isError(postRaw(client, path, "not json"), drogon::k400BadRequest, "body is not JSON"));
    CHECK(isError(postRaw(client, path, "[]"), drogon::k400BadRequest, "body is an array"));

    Json::Value noPassword;
    noPassword["email"] = "alice@example.com";
    CHECK(isError(post(client, path, noPassword), drogon::k400BadRequest, "no password"));

    Json::Value noEmail;
    noEmail["password"] = kPassword;
    CHECK(isError(post(client, path, noEmail), drogon::k400BadRequest, "no email"));

    Json::Value numericPassword = credentials("alice@example.com", kPassword);
    numericPassword["password"] = 12345678;
    CHECK(isError(post(client, path, numericPassword), drogon::k400BadRequest, "numeric password"));

    CHECK(isError(post(client, path, credentials("", kPassword)), drogon::k400BadRequest,
                  "empty email"));
    CHECK(isError(post(client, path, credentials("alice@example.com", "")), drogon::k400BadRequest,
                  "empty password"));
    CHECK(isError(post(client, path, credentials(std::string(255, 'a'), kPassword)),
                  drogon::k400BadRequest, "255-byte email"));
    CHECK(isError(post(client, path, credentials("alice@example.com", std::string(1025, 'p'))),
                  drogon::k400BadRequest, "1025-byte password"));
}

DROGON_TEST(RecipesRequireLogin) {
    const auto client = newClient();

    CHECK(isError(get(client, "/api/recipes"), drogon::k401Unauthorized, "list recipes"));
    CHECK(isError(post(client, "/api/recipes", recipe("Soup", "Water")), drogon::k401Unauthorized,
                  "save recipe"));
    CHECK(isError(get(client, "/api/recipes/1"), drogon::k401Unauthorized, "view recipe"));
    // The login check comes first, so an invalid ID does not reveal anything either.
    CHECK(isError(get(client, "/api/recipes/abc"), drogon::k401Unauthorized, "view invalid ID"));
    CHECK(isError(put(client, "/api/recipes/1", recipe("Soup", "Water")), drogon::k401Unauthorized,
                  "edit recipe"));
}

// ---------------------------------------------------------------------------
// Tests that need the database.
// ---------------------------------------------------------------------------

DROGON_TEST(RegisterAndLogin) {
    if (!testsupport::databaseAvailable()) {
        return;
    }

    const auto client = newClient();
    const std::string email = testEmail("register");

    // The display name is exactly at the 100-byte limit, and the email is trimmed.
    const auto created = post(client, "/api/auth/register",
                              registration(std::string(100, 'a'), "  " + email + "\n", kPassword));
    REQUIRE(hasStatus(created, drogon::k201Created, "register"));
    REQUIRE(created->getJsonObject());
    CHECK((*created->getJsonObject())["message"].isString());
    CHECK(created->getHeader("Cache-Control") == "no-store");

    // Registration does not log the user in.
    CHECK(isError(get(client, "/api/recipes"), drogon::k401Unauthorized, "not logged in yet"));

    CHECK(isError(post(client, "/api/auth/register", registration("Again", email, kPassword)),
                  drogon::k409Conflict, "duplicate email"));

    CHECK(isError(post(client, "/api/auth/login", credentials(email, "Wrong-password1")),
                  drogon::k401Unauthorized, "wrong password"));
    CHECK(isError(post(client, "/api/auth/login", credentials(testEmail("nobody"), kPassword)),
                  drogon::k401Unauthorized, "unknown email"));
    CHECK(isError(get(client, "/api/recipes"), drogon::k401Unauthorized, "failed logins"));

    const auto loggedIn = post(client, "/api/auth/login", credentials(email, kPassword));
    REQUIRE(hasStatus(loggedIn, drogon::k200OK, "login"));
    REQUIRE(loggedIn->getJsonObject());

    const auto& user = (*loggedIn->getJsonObject())["user"];
    CHECK(user["email"].asString() == email);
    CHECK(!user["id"].asString().empty());
    CHECK(!loggedIn->getJsonObject()->isMember("password_hash"));
    CHECK(!user.isMember("password_hash"));
    CHECK(loggedIn->getHeader("Cache-Control") == "no-store");

    // The session cookie now gives access, but only to this client.
    CHECK(hasStatus(get(client, "/api/recipes"), drogon::k200OK, "logged in"));
    CHECK(isError(get(newClient(), "/api/recipes"), drogon::k401Unauthorized, "other browser"));
}

DROGON_TEST(UpgradeRequiresLogin) {
    CHECK(isError(post(newClient(), "/api/account/upgrade", Json::Value(Json::objectValue)),
                  drogon::k401Unauthorized, "upgrade"));
}

DROGON_TEST(ProfileRequiresLogin) {
    CHECK(isError(get(newClient(), "/api/auth/me"), drogon::k401Unauthorized, "profile"));
}

DROGON_TEST(ProfileShowsAccountTierAndRecipeCount) {
    if (!testsupport::databaseAvailable()) {
        return;
    }

    const auto client = newClient();
    const auto other = newClient();
    REQUIRE(signIn(client, "profile"));
    REQUIRE(signIn(other, "profile-other"));

    const auto fresh = get(client, "/api/auth/me");
    REQUIRE(hasStatus(fresh, drogon::k200OK, "new profile"));
    REQUIRE(fresh->getJsonObject());
    CHECK(fresh->getHeader("Cache-Control") == "no-store");

    const auto& user = (*fresh->getJsonObject())["user"];
    CHECK(!user["id"].asString().empty());
    CHECK(user["display_name"].asString() == "profile");
    CHECK(user["email"].asString() == testEmail("profile"));
    // New accounts start on the Free tier.
    CHECK(user["tier"].asString() == "Free");
    CHECK(user["recipe_limit"].asUInt() == 25);
    CHECK(user["recipe_count"].asUInt() == 0);
    CHECK(!user.isMember("password_hash"));

    REQUIRE(!saveRecipe(client, recipe("Soup", "Water")).empty());
    REQUIRE(!saveRecipe(client, recipe("Toast", "Bread")).empty());
    // Another user's recipes are not counted.
    REQUIRE(!saveRecipe(other, recipe("Salad", "Lettuce")).empty());

    const auto updated = get(client, "/api/auth/me");
    REQUIRE(hasStatus(updated, drogon::k200OK, "profile with recipes"));
    REQUIRE(updated->getJsonObject());
    CHECK((*updated->getJsonObject())["user"]["recipe_count"].asUInt() == 2);
}

DROGON_TEST(UpgradeMovesFreeUserToPremium) {
    if (!testsupport::databaseAvailable()) {
        return;
    }

    const Json::Value empty(Json::objectValue);
    const auto client = newClient();
    const auto other = newClient();
    REQUIRE(signIn(client, "upgrade"));
    REQUIRE(signIn(other, "upgrade-other"));

    const auto upgrade = post(client, "/api/account/upgrade", empty);
    REQUIRE(hasStatus(upgrade, drogon::k200OK, "upgrade"));
    REQUIRE(upgrade->getJsonObject());
    CHECK(upgrade->getHeader("Cache-Control") == "no-store");
    CHECK((*upgrade->getJsonObject())["user"]["tier"].asString() == "Premium");
    CHECK((*upgrade->getJsonObject())["user"]["recipe_limit"].asUInt() == 500);

    // The profile shows the new plan straight away.
    const auto profile = get(client, "/api/auth/me");
    REQUIRE(hasStatus(profile, drogon::k200OK, "profile after upgrade"));
    REQUIRE(profile->getJsonObject());
    CHECK((*profile->getJsonObject())["user"]["tier"].asString() == "Premium");
    CHECK((*profile->getJsonObject())["user"]["recipe_limit"].asUInt() == 500);

    // A Premium user cannot upgrade again.
    CHECK(isError(post(client, "/api/account/upgrade", empty), drogon::k409Conflict,
                  "second upgrade"));

    // The change is stored in the database, not the session, so a fresh login sees it.
    const auto returning = newClient();
    REQUIRE(
        hasStatus(post(returning, "/api/auth/login", credentials(testEmail("upgrade"), kPassword)),
                  drogon::k200OK, "login again"));
    const auto later = get(returning, "/api/auth/me");
    REQUIRE(hasStatus(later, drogon::k200OK, "profile after login"));
    REQUIRE(later->getJsonObject());
    CHECK((*later->getJsonObject())["user"]["tier"].asString() == "Premium");

    // Only the user who upgraded changes tier.
    const auto untouched = get(other, "/api/auth/me");
    REQUIRE(hasStatus(untouched, drogon::k200OK, "other profile"));
    REQUIRE(untouched->getJsonObject());
    CHECK((*untouched->getJsonObject())["user"]["tier"].asString() == "Free");
}

DROGON_TEST(RecipeCreateListAndView) {
    if (!testsupport::databaseAvailable()) {
        return;
    }

    const auto client = newClient();
    REQUIRE(signIn(client, "recipes"));

    const auto empty = get(client, "/api/recipes");
    REQUIRE(hasStatus(empty, drogon::k200OK, "empty list"));
    REQUIRE(empty->getJsonObject());
    CHECK((*empty->getJsonObject())["recipes"].isArray());
    CHECK((*empty->getJsonObject())["recipes"].empty());

    // Only the required fields. Text is trimmed before it is stored.
    const std::string toastId = saveRecipe(client, recipe("  Toast \n", "\tBread\n"));
    REQUIRE(!toastId.empty());

    Json::Value full = recipe("Apple pie", "Apples\nFlour\nButter");
    full["steps"] = "  Make the dough.\nBake.  ";
    full["notes"] = "Serve warm.";
    full["servings"] = 8;
    full["prep_minutes"] = 0;
    full["cook_minutes"] = 65535;
    const std::string pieId = saveRecipe(client, full);
    REQUIRE(!pieId.empty());
    CHECK(pieId != toastId);

    // Blank optional text and explicit nulls are stored as "no value".
    Json::Value blanks = recipe("Zucchini bread", "Zucchini");
    blanks["steps"] = "   ";
    blanks["notes"] = "";
    blanks["servings"] = Json::Value();
    const std::string breadId = saveRecipe(client, blanks);
    REQUIRE(!breadId.empty());

    const auto listed = get(client, "/api/recipes");
    REQUIRE(hasStatus(listed, drogon::k200OK, "list"));
    REQUIRE(listed->getJsonObject());
    CHECK(listed->getHeader("Cache-Control") == "no-store");

    const auto& recipes = (*listed->getJsonObject())["recipes"];
    REQUIRE(recipes.size() == 3);

    // Sorted by name.
    CHECK(recipes[0]["id"].asString() == pieId);
    CHECK(recipes[0]["name"].asString() == "Apple pie");
    CHECK(recipes[0]["ingredient_list"].asString() == "Apples\nFlour\nButter");
    CHECK(recipes[0]["servings"].asUInt() == 8);
    CHECK(recipes[0]["prep_minutes"].isUInt());
    CHECK(recipes[0]["prep_minutes"].asUInt() == 0);
    CHECK(recipes[0]["cook_minutes"].asUInt() == 65535);

    CHECK(recipes[1]["id"].asString() == toastId);
    CHECK(recipes[1]["name"].asString() == "Toast");
    CHECK(recipes[1]["ingredient_list"].asString() == "Bread");
    CHECK(recipes[1]["servings"].isNull());
    CHECK(recipes[1]["prep_minutes"].isNull());
    CHECK(recipes[1]["cook_minutes"].isNull());

    CHECK(recipes[2]["id"].asString() == breadId);

    const auto pie = get(client, "/api/recipes/" + pieId);
    REQUIRE(hasStatus(pie, drogon::k200OK, "view full recipe"));
    REQUIRE(pie->getJsonObject());
    CHECK(pie->getHeader("Cache-Control") == "no-store");

    const auto& pieBody = (*pie->getJsonObject())["recipe"];
    CHECK(pieBody["id"].asString() == pieId);
    CHECK(pieBody["name"].asString() == "Apple pie");
    CHECK(pieBody["ingredient_list"].asString() == "Apples\nFlour\nButter");
    CHECK(pieBody["steps"].asString() == "Make the dough.\nBake.");
    CHECK(pieBody["notes"].asString() == "Serve warm.");
    CHECK(pieBody["servings"].asUInt() == 8);
    CHECK(pieBody["prep_minutes"].isUInt());
    CHECK(pieBody["prep_minutes"].asUInt() == 0);
    CHECK(pieBody["cook_minutes"].asUInt() == 65535);

    const auto bread = get(client, "/api/recipes/" + breadId);
    REQUIRE(hasStatus(bread, drogon::k200OK, "view minimal recipe"));
    REQUIRE(bread->getJsonObject());

    const auto& breadBody = (*bread->getJsonObject())["recipe"];
    CHECK(breadBody["name"].asString() == "Zucchini bread");
    CHECK(breadBody["steps"].isNull());
    CHECK(breadBody["notes"].isNull());
    CHECK(breadBody["servings"].isNull());
    CHECK(breadBody["prep_minutes"].isNull());
    CHECK(breadBody["cook_minutes"].isNull());
}

DROGON_TEST(RecipeRejectsInvalidInput) {
    if (!testsupport::databaseAvailable()) {
        return;
    }

    const auto client = newClient();
    REQUIRE(signIn(client, "validation"));

    const std::string path = "/api/recipes";

    CHECK(isError(postRaw(client, path, ""), drogon::k400BadRequest, "empty body"));
    CHECK(isError(postRaw(client, path, "not json"), drogon::k400BadRequest, "body is not JSON"));
    CHECK(isError(postRaw(client, path, "[]"), drogon::k400BadRequest, "body is an array"));
    CHECK(isError(postRaw(client, path, R"({"name":"a\u0000b","ingredient_list":"Water"})"),
                  drogon::k400BadRequest, "NUL byte in name"));

    Json::Value noName;
    noName["ingredient_list"] = "Water";
    CHECK(isError(post(client, path, noName), drogon::k400BadRequest, "no name"));

    Json::Value noIngredients;
    noIngredients["name"] = "Soup";
    CHECK(isError(post(client, path, noIngredients), drogon::k400BadRequest, "no ingredients"));

    Json::Value numericName = recipe("Soup", "Water");
    numericName["name"] = 42;
    CHECK(isError(post(client, path, numericName), drogon::k400BadRequest, "numeric name"));

    CHECK(isError(post(client, path, recipe(" \t\n", "Water")), drogon::k400BadRequest,
                  "blank name"));
    CHECK(isError(post(client, path, recipe(std::string(201, 'n'), "Water")),
                  drogon::k400BadRequest, "201-byte name"));
    CHECK(isError(post(client, path, recipe("Soup", "  \n")), drogon::k400BadRequest,
                  "blank ingredients"));
    CHECK(isError(post(client, path, recipe("Soup", std::string(65536, 'i'))),
                  drogon::k400BadRequest, "65536-byte ingredients"));

    for (const char* field : {"steps", "notes"}) {
        Json::Value notText = recipe("Soup", "Water");
        notText[field] = 5;
        CHECK(isError(post(client, path, notText), drogon::k400BadRequest,
                      std::string("numeric ") + field));

        Json::Value tooLong = recipe("Soup", "Water");
        tooLong[field] = std::string(65536, 's');
        CHECK(isError(post(client, path, tooLong), drogon::k400BadRequest,
                      std::string("65536-byte ") + field));
    }

    for (const char* field : {"servings", "prep_minutes", "cook_minutes"}) {
        const std::vector<std::pair<std::string, Json::Value>> values = {
            {"negative", Json::Value(-1)}, {"fractional", Json::Value(1.5)},
            {"text", Json::Value("4")},    {"true", Json::Value(true)},
            {"65536", Json::Value(65536)},
        };
        for (const auto& [label, value] : values) {
            Json::Value body = recipe("Soup", "Water");
            body[field] = value;
            CHECK(isError(post(client, path, body), drogon::k400BadRequest,
                          label + " " + std::string(field)));
        }
    }

    Json::Value zeroServings = recipe("Soup", "Water");
    zeroServings["servings"] = 0;
    CHECK(isError(post(client, path, zeroServings), drogon::k400BadRequest, "zero servings"));

    // None of the rejected requests saved anything.
    const auto afterRejections = get(client, path);
    REQUIRE(hasStatus(afterRejections, drogon::k200OK, "list after rejections"));
    REQUIRE(afterRejections->getJsonObject());
    CHECK((*afterRejections->getJsonObject())["recipes"].empty());

    // Values exactly at the limits are accepted.
    Json::Value limits = recipe(std::string(200, 'n'), std::string(65535, 'i'));
    limits["steps"] = std::string(65535, 's');
    limits["notes"] = std::string(65535, 'o');
    limits["servings"] = 1;
    limits["prep_minutes"] = 0;
    limits["cook_minutes"] = 65535;
    const std::string limitsId = saveRecipe(client, limits);
    REQUIRE(!limitsId.empty());

    const auto stored = get(client, path + "/" + limitsId);
    REQUIRE(hasStatus(stored, drogon::k200OK, "view recipe at the limits"));
    REQUIRE(stored->getJsonObject());

    const auto& storedBody = (*stored->getJsonObject())["recipe"];
    CHECK(storedBody["name"].asString().size() == 200);
    CHECK(storedBody["ingredient_list"].asString().size() == 65535);
    CHECK(storedBody["steps"].asString().size() == 65535);
    CHECK(storedBody["notes"].asString().size() == 65535);
    CHECK(storedBody["servings"].asUInt() == 1);
}

DROGON_TEST(RecipeDetailsRejectsBadIds) {
    if (!testsupport::databaseAvailable()) {
        return;
    }

    const auto client = newClient();
    REQUIRE(signIn(client, "ids"));

    for (const char* id : {"abc", "0", "01", "-1", "1.5", "1a", "4294967296"}) {
        CHECK(isError(get(client, std::string("/api/recipes/") + id), drogon::k400BadRequest,
                      std::string("recipe ID ") + id));
    }

    // The largest valid ID is well-formed, but no recipe has it.
    CHECK(isError(get(client, "/api/recipes/4294967295"), drogon::k404NotFound, "unknown ID"));
}

DROGON_TEST(RecipesArePrivateToTheirOwner) {
    if (!testsupport::databaseAvailable()) {
        return;
    }

    const auto alice = newClient();
    const auto bob = newClient();
    REQUIRE(signIn(alice, "alice"));
    REQUIRE(signIn(bob, "bob"));

    const std::string aliceRecipe = saveRecipe(alice, recipe("Alice's soup", "Water"));
    const std::string bobRecipe = saveRecipe(bob, recipe("Bob's salad", "Lettuce"));
    REQUIRE(!aliceRecipe.empty());
    REQUIRE(!bobRecipe.empty());

    const auto aliceList = get(alice, "/api/recipes");
    REQUIRE(hasStatus(aliceList, drogon::k200OK, "Alice's list"));
    REQUIRE(aliceList->getJsonObject());

    const auto& recipes = (*aliceList->getJsonObject())["recipes"];
    REQUIRE(recipes.size() == 1);
    CHECK(recipes[0]["id"].asString() == aliceRecipe);

    CHECK(hasStatus(get(alice, "/api/recipes/" + aliceRecipe), drogon::k200OK, "own recipe"));
    // Someone else's recipe looks the same as one that does not exist.
    CHECK(isError(get(alice, "/api/recipes/" + bobRecipe), drogon::k404NotFound, "Bob's recipe"));
    CHECK(isError(get(bob, "/api/recipes/" + aliceRecipe), drogon::k404NotFound, "Alice's recipe"));
}

DROGON_TEST(RecipeEdit) {
    if (!testsupport::databaseAvailable()) {
        return;
    }

    const auto client = newClient();
    const auto other = newClient();
    REQUIRE(signIn(client, "edit"));
    REQUIRE(signIn(other, "edit-other"));

    Json::Value original = recipe("Apple pie", "Apples\nFlour");
    original["steps"] = "Bake.";
    original["notes"] = "Serve warm.";
    original["servings"] = 8;
    original["prep_minutes"] = 30;
    original["cook_minutes"] = 60;
    const std::string id = saveRecipe(client, original);
    REQUIRE(!id.empty());
    const std::string path = "/api/recipes/" + id;

    auto stored = [&]() {
        const auto response = get(client, path);
        return response && response->getJsonObject() ? (*response->getJsonObject())["recipe"]
                                                     : Json::Value();
    };

    // Every field can be changed. Text is trimmed, as it is when adding.
    Json::Value changed = recipe("  Pear tart \n", "Pears\nFlour\nButter");
    changed["steps"] = "Make the dough.\nBake.";
    changed["notes"] = "Better cold.";
    changed["servings"] = 6;
    changed["prep_minutes"] = 0;
    changed["cook_minutes"] = 45;

    const auto update = put(client, path, changed);
    REQUIRE(hasStatus(update, drogon::k200OK, "edit every field"));
    REQUIRE(update->getJsonObject());
    CHECK(update->getHeader("Cache-Control") == "no-store");
    CHECK((*update->getJsonObject())["recipe"]["id"].asString() == id);

    // The recipe view shows the new values straight away.
    const auto tart = stored();
    CHECK(tart["id"].asString() == id);
    CHECK(tart["name"].asString() == "Pear tart");
    CHECK(tart["ingredient_list"].asString() == "Pears\nFlour\nButter");
    CHECK(tart["steps"].asString() == "Make the dough.\nBake.");
    CHECK(tart["notes"].asString() == "Better cold.");
    CHECK(tart["servings"].asUInt() == 6);
    CHECK(tart["prep_minutes"].isUInt());
    CHECK(tart["prep_minutes"].asUInt() == 0);
    CHECK(tart["cook_minutes"].asUInt() == 45);

    // So does the collection, which still holds one recipe.
    const auto listed = get(client, "/api/recipes");
    REQUIRE(hasStatus(listed, drogon::k200OK, "list after edit"));
    REQUIRE(listed->getJsonObject());
    const auto& recipes = (*listed->getJsonObject())["recipes"];
    REQUIRE(recipes.size() == 1);
    CHECK(recipes[0]["id"].asString() == id);
    CHECK(recipes[0]["name"].asString() == "Pear tart");
    CHECK(recipes[0]["cook_minutes"].asUInt() == 45);

    // Saving without changing anything succeeds.
    CHECK(hasStatus(put(client, path, changed), drogon::k200OK, "edit with no changes"));

    // The required-field rules are the same as when adding, and a refused edit changes nothing.
    Json::Value badNumber = recipe("Pear tart", "Pears");
    badNumber["servings"] = 0;
    const std::vector<std::pair<std::string, Json::Value>> invalid = {
        {"missing name",
         [] {
             Json::Value body;
             body["ingredient_list"] = "Pears";
             return body;
         }()},
        {"blank name", recipe("   ", "Pears")},
        {"blank ingredients", recipe("Pear tart", " \n ")},
        {"long name", recipe(std::string(201, 'n'), "Pears")},
        {"zero servings", badNumber},
        {"not an object", Json::Value(Json::arrayValue)},
    };
    for (const auto& [label, body] : invalid) {
        CHECK(isError(put(client, path, body), drogon::k400BadRequest, "edit with " + label));
    }
    CHECK(stored()["name"].asString() == "Pear tart");
    CHECK(stored()["ingredient_list"].asString() == "Pears\nFlour\nButter");

    // Another user cannot edit the recipe, and gets the same answer as for a missing one.
    CHECK(isError(put(other, path, recipe("Stolen", "Nothing")), drogon::k404NotFound,
                  "edit another user's recipe"));
    CHECK(stored()["name"].asString() == "Pear tart");
    CHECK(isError(put(client, "/api/recipes/4294967295", recipe("Ghost", "Nothing")),
                  drogon::k404NotFound, "edit unknown recipe"));
    CHECK(isError(put(client, "/api/recipes/abc", recipe("Ghost", "Nothing")),
                  drogon::k400BadRequest, "edit invalid ID"));

    // Optional fields that are left out or blank are cleared.
    Json::Value minimal = recipe("Pear tart", "Pears");
    minimal["steps"] = "  ";
    minimal["servings"] = Json::Value();
    CHECK(hasStatus(put(client, path, minimal), drogon::k200OK, "edit to required fields only"));

    const auto cleared = stored();
    CHECK(cleared["ingredient_list"].asString() == "Pears");
    CHECK(cleared["steps"].isNull());
    CHECK(cleared["notes"].isNull());
    CHECK(cleared["servings"].isNull());
    CHECK(cleared["prep_minutes"].isNull());
    CHECK(cleared["cook_minutes"].isNull());
}

DROGON_TEST(RecipeLimitIsEnforced) {
    if (!testsupport::databaseAvailable()) {
        return;
    }

    const auto client = newClient();
    const auto other = newClient();
    REQUIRE(signIn(client, "limit"));
    REQUIRE(signIn(other, "limit-other"));

    // The delete and favorite steps go straight to the database.
    const auto database = drogon::app().getDbClient("default");
    const std::string email = testEmail("limit");

    const auto tiers = database->execSqlSync(
        "SELECT recipe_limit AS free_limit FROM user_tiers WHERE tier_name = 'Free'");
    const auto freeLimit = tiers[0]["free_limit"].as<unsigned int>();

    auto ownedCount = [&]() {
        const auto listed = get(client, "/api/recipes");
        return listed && listed->getJsonObject() ? (*listed->getJsonObject())["recipes"].size() : 0;
    };

    // A new account is on the Free plan and can save recipes up to its limit.
    std::string lastId;
    for (unsigned int i = 1; i <= freeLimit; ++i) {
        lastId = saveRecipe(client, recipe("Recipe " + std::to_string(i), "Flour"));
        REQUIRE(!lastId.empty());
    }

    // The next one is refused, and the message points to Premium.
    const auto blocked = post(client, "/api/recipes", recipe("One too many", "Flour"));
    REQUIRE(isError(blocked, drogon::k403Forbidden, "at the Free limit"));

    const auto& blockedBody = *blocked->getJsonObject();
    CHECK(blockedBody["limit_reached"].asBool());
    CHECK(blockedBody["tier_name"].asString() == "Free");
    CHECK(blockedBody["recipe_limit"].asUInt() == freeLimit);
    CHECK(blockedBody["error"].asString().find("limit") != std::string::npos);
    CHECK(blockedBody["error"].asString().find("Premium") != std::string::npos);
    CHECK(ownedCount() == freeLimit);

    // Another user's full collection does not block this one.
    CHECK(!saveRecipe(other, recipe("Other's recipe", "Water")).empty());

    // Deleting a recipe frees one place, and only one.
    database->execSqlSync("DELETE FROM recipes WHERE id = ?", lastId);
    CHECK(!saveRecipe(client, recipe("Replacement", "Flour")).empty());
    CHECK(isError(post(client, "/api/recipes", recipe("One too many", "Flour")),
                  drogon::k403Forbidden, "back at the limit"));

    // A favorite takes a place just like an owned recipe.
    const auto replacement = database->execSqlSync(
        "SELECT r.id FROM recipes r JOIN users u ON u.id = r.user_id "
        "WHERE u.email = ? AND r.name = 'Replacement'",
        email);
    REQUIRE(replacement.size() == 1);
    database->execSqlSync("DELETE FROM recipes WHERE id = ?",
                          replacement[0]["id"].as<std::string>());
    database->execSqlSync(
        "INSERT INTO recipe_favorites (user_id, recipe_id) "
        "SELECT u.id, r.id FROM users u, recipes r "
        "JOIN users o ON o.id = r.user_id "
        "WHERE u.email = ? AND o.email = ?",
        email, testEmail("limit-other"));
    CHECK(isError(post(client, "/api/recipes", recipe("One too many", "Flour")),
                  drogon::k403Forbidden, "favorite fills the last place"));
    CHECK(ownedCount() == freeLimit - 1);

    // A user blocked at the Free limit can save again right after upgrading.
    REQUIRE(hasStatus(post(client, "/api/account/upgrade", Json::Value(Json::objectValue)),
                      drogon::k200OK, "upgrade at the limit"));
    CHECK(!saveRecipe(client, recipe("Room on Premium", "Flour")).empty());
    CHECK(ownedCount() == freeLimit);
}

DROGON_TEST(ProfileUpdate) {
    const std::string path = "/api/auth/me";
    const auto client = newClient();
    const auto other = newClient();

    Json::Value input;
    input["display_name"] = "Updated Cook";
    input["email"] = testEmail("profile-edit");

    CHECK(isError(put(client, path, input), drogon::k401Unauthorized,
                  "profile update requires login"));

    if (!testsupport::databaseAvailable()) {
        return;
    }

    REQUIRE(signIn(client, "profile-edit"));
    REQUIRE(signIn(other, "profile-edit-other"));

    const auto originalEmail = testEmail("profile-edit");
    const auto changedEmail = testEmail("profile-edit-new");

    auto checkProfile = [&](const HttpClientPtr& account, const std::string& expectedName,
                            const std::string& expectedEmail) {
        const auto response = get(account, path);
        REQUIRE(hasStatus(response, drogon::k200OK, "read profile"));
        REQUIRE(response->getJsonObject());

        const auto user = (*response->getJsonObject())["user"];
        CHECK(user["display_name"].asString() == expectedName);
        CHECK(user["email"].asString() == expectedEmail);
        CHECK(user["tier"].asString() == "Free");
        CHECK(user["recipe_limit"].asUInt() == 25);
        CHECK(!user.isMember("password_hash"));
    };

    auto checkFieldError = [&](const Json::Value& body, HttpStatusCode status,
                               const std::string& field) {
        const auto response = put(client, path, body);
        REQUIRE(isError(response, status, "profile field " + field));
        REQUIRE(response->getJsonObject());
        CHECK((*response->getJsonObject())["field"].asString() == field);
    };

    // Name changes do not require a password, and surrounding spaces are trimmed.
    input["display_name"] = "  Updated Cook  ";
    input["email"] = originalEmail;

    const auto renamed = put(client, path, input);
    REQUIRE(hasStatus(renamed, drogon::k200OK, "change display name"));
    REQUIRE(renamed->getJsonObject());
    CHECK(renamed->getHeader("Cache-Control") == "no-store");
    CHECK((*renamed->getJsonObject())["user"]["display_name"].asString() == "Updated Cook");
    checkProfile(client, "Updated Cook", originalEmail);

    // Saving unchanged values must also succeed.
    input["display_name"] = "Updated Cook";
    CHECK(hasStatus(put(client, path, input), drogon::k200OK, "save unchanged account"));

    // Display names may match another account's name.
    input["display_name"] = "profile-edit-other";
    REQUIRE(hasStatus(put(client, path, input), drogon::k200OK,
                      "display names do not need to be unique"));

    // Invalid fields are identified, and rejected requests change nothing.
    const auto valid = input;
    input["display_name"] = "   ";
    checkFieldError(input, drogon::k400BadRequest, "display_name");

    input = valid;
    input.removeMember("display_name");
    checkFieldError(input, drogon::k400BadRequest, "display_name");

    input = valid;
    input["display_name"] = std::string(101, 'n');
    checkFieldError(input, drogon::k400BadRequest, "display_name");

    input = valid;
    input["email"] = " ";
    checkFieldError(input, drogon::k400BadRequest, "email");

    input = valid;
    input.removeMember("email");
    checkFieldError(input, drogon::k400BadRequest, "email");

    input = valid;
    input["email"] = "not-an-email";
    checkFieldError(input, drogon::k400BadRequest, "email");

    checkProfile(client, "profile-edit-other", originalEmail);

    // Email changes require the current password.
    input = valid;
    input["display_name"] = "Final Cook";
    input["email"] = changedEmail;
    checkFieldError(input, drogon::k400BadRequest, "current_password");

    input["current_password"] = "wrong-password";
    checkFieldError(input, drogon::k403Forbidden, "current_password");
    checkProfile(client, "profile-edit-other", originalEmail);

    // Another account's email is refused without partially saving the name.
    input["current_password"] = kPassword;
    input["email"] = testEmail("profile-edit-other");
    checkFieldError(input, drogon::k409Conflict, "email");
    checkProfile(client, "profile-edit-other", originalEmail);
    checkProfile(other, "profile-edit-other", testEmail("profile-edit-other"));

    // A valid email change updates only the signed-in account.
    input["email"] = changedEmail;
    REQUIRE(
        hasStatus(put(client, path, input), drogon::k200OK, "change email with correct password"));
    checkProfile(client, "Final Cook", changedEmail);
    checkProfile(other, "profile-edit-other", testEmail("profile-edit-other"));

    // A fresh login proves the new values are stored in the database.
    // The test server does not register main.cc's logout handler, so use
    // a separate client with no session cookies.
    const auto returning = newClient();
    CHECK(isError(post(returning, "/api/auth/login", credentials(originalEmail, kPassword)),
                  drogon::k401Unauthorized, "old email no longer logs in"));

    REQUIRE(hasStatus(post(returning, "/api/auth/login", credentials(changedEmail, kPassword)),
                      drogon::k200OK, "login with updated email"));
    checkProfile(returning, "Final Cook", changedEmail);
}
