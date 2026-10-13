#pragma once

#include <sodium.h>
#include <string>

namespace auth {

inline bool verifyPassword(const std::string& password,
                           const std::string& storedHash) {
    return crypto_pwhash_str_verify(
               storedHash.c_str(),
               password.data(),
               password.size()) == 0;
}

}