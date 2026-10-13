#define DROGON_TEST_MAIN
#include <drogon/drogon.h>
#include <drogon/drogon_test.h>

#include "../Password.h"

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

int main(int argc, char** argv) {
    using namespace drogon;

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
    int status = test::run(argc, argv);

    // Ask the event loop to shutdown and wait
    app().getLoop()->queueInLoop([]() { app().quit(); });
    thr.join();
    return status;
}
