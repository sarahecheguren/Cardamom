#pragma once

#include <sodium.h>

#include <stdexcept>
#include <string>

namespace auth {

inline std::string hashPassword(const std::string& password) {
    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str_alg(
            hash, password.data(), password.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_ARGON2ID13) != 0) {
        throw std::runtime_error("Password hashing failed");
    }

    return std::string(hash);
}

inline bool verifyPassword(const std::string& password, const std::string& storedHash) {
    return crypto_pwhash_str_verify(storedHash.c_str(), password.data(), password.size()) == 0;
}

}  // namespace auth