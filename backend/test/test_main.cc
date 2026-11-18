#define DROGON_TEST_MAIN
#include <drogon/drogon.h>
#include <drogon/drogon_test.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "../Password.h"
#include "test_support.h"

namespace {

std::string serverUrl;
std::string currentRunId;
bool databaseReady = false;

std::string readSetting(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return std::string(value && value[0] != '\0' ? value : fallback);
}

// Runs one query and reports whether the database answered within five seconds.
bool probeDatabase() {
    auto answered = std::make_shared<std::promise<bool>>();
    auto answer = answered->get_future();

    drogon::app().getDbClient("default")->execSqlAsync(
        "SELECT 1", [answered](const drogon::orm::Result&) { answered->set_value(true); },
        [answered](const drogon::orm::DrogonDbException&) { answered->set_value(false); });

    return answer.wait_for(std::chrono::seconds(5)) == std::future_status::ready && answer.get();
}

// Deleting a user also deletes their recipes (ON DELETE CASCADE).
void deleteTestAccounts() {
    try {
        drogon::app().getDbClient("default")->execSqlSync("DELETE FROM users WHERE email LIKE ?",
                                                          "apitest-" + currentRunId + "-%");
    } catch (const drogon::orm::DrogonDbException&) {
        std::cerr << "Could not delete the test accounts for run " << currentRunId << ".\n";
    }
}

}  // namespace

namespace testsupport {

const std::string& baseUrl() {
    return serverUrl;
}

bool databaseAvailable() {
    return databaseReady;
}

const std::string& runId() {
    return currentRunId;
}

}  // namespace testsupport

DROGON_TEST(PasswordVerification) {
    REQUIRE(sodium_init() >= 0);

    const std::string password = "test-password";
    char hash[crypto_pwhash_STRBYTES];

    const int result = crypto_pwhash_str_alg(
        hash, password.data(), password.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_ARGON2ID13);

    REQUIRE(result == 0);

    CHECK(auth::verifyPassword(password, hash));
    CHECK(!auth::verifyPassword("wrong-password", hash));
    CHECK(!auth::verifyPassword("", hash));
    CHECK(!auth::verifyPassword(password, "not-a-valid-hash"));
}

DROGON_TEST(PasswordHashing) {
    REQUIRE(sodium_init() >= 0);

    const std::string first = auth::hashPassword("test-password");
    const std::string second = auth::hashPassword("test-password");

    CHECK(first.rfind("$argon2id$", 0) == 0);
    CHECK(first != second);
    CHECK(auth::verifyPassword("test-password", first));
    CHECK(auth::verifyPassword("test-password", second));
    CHECK(!auth::verifyPassword("Test-password", first));
}

int main(int argc, char** argv) {
    using namespace drogon;

    if (sodium_init() < 0) {
        std::cerr << "Could not initialize libsodium\n";
        return 1;
    }

    // The API tests talk to the real controllers over HTTP, so this starts the
    // backend the same way main.cc does, on a separate port.
    const std::string port = readSetting("TEST_PORT", "18080");
    serverUrl = "http://127.0.0.1:" + port;
    currentRunId = std::to_string(getpid()) + "-" +
                   std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

    app().createDbClient("mysql", readSetting("DB_HOST", "127.0.0.1"),
                         static_cast<unsigned short>(std::stoi(readSetting("DB_PORT", "3306"))),
                         readSetting("DB_NAME", "cardamom"), readSetting("DB_USER", "cardamom"),
                         readSetting("DB_PASSWORD", ""), 1, "", "default");
    app().enableSession(1800, Cookie::SameSite::kStrict);
    app().addListener("127.0.0.1", static_cast<uint16_t>(std::stoi(port)));

    std::promise<void> p1;
    std::future<void> f1 = p1.get_future();

    // Start the main loop on another thread
    std::thread thr([&]() {
        // Queues the promise to be fulfilled after starting the loop
        app().getLoop()->queueInLoop([&p1]() { p1.set_value(); });
        app().run();
    });

    // The future is only satisfied after the event loop started
    f1.get();

    databaseReady = probeDatabase();
    if (!databaseReady) {
        std::cerr << "\nWARNING: the database did not answer, so the tests that need it are "
                     "skipped.\nStart it with `docker compose up -d db` and load .env first "
                     "(see backend/README.md).\n\n";
    }

    int status = test::run(argc, argv);

    if (databaseReady) {
        deleteTestAccounts();
    }

    // Ask the event loop to shutdown and wait
    app().getLoop()->queueInLoop([]() { app().quit(); });
    thr.join();

    // Leave without running static destructors. Drogon crashes in its session
    // store's destructor when a session whose ID was changed (which login does)
    // is still alive at exit, and that would turn a passing run into a segfault.
    std::cout.flush();
    std::cerr.flush();
    std::fflush(nullptr);
    std::_Exit(status);
}
