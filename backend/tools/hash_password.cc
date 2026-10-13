#include <sodium.h>
#include <iostream>
#include <string>

int main()
{
    if (sodium_init() < 0)
    {
        return 1;
    }

    // Dummy password for local testing only.
    const std::string password = "test-password";
    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str_alg(
            hash,
            password.data(),
            password.size(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_ARGON2ID13) != 0)
    {
        std::cerr << "Password hashing failed\n";
        return 1;
    }

    std::cout << hash << '\n';
    return 0;
}