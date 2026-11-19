#pragma once

#include <string>

namespace testsupport {

// Address of the backend instance that test_main.cc starts for the API tests.
const std::string& baseUrl();

// True when the MariaDB database answered a query at startup. Tests that need
// stored data return early when it is false.
bool databaseAvailable();

// Unique per test run. Test accounts use it in their email address so runs
// never collide and test_main.cc can delete them afterwards.
const std::string& runId();

}  // namespace testsupport
