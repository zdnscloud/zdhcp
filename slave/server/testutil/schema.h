#pragma once

#include <cstdlib>
#include <string>

namespace kea {
namespace server {
namespace testutil {

extern const char* INVALID_TYPE;
extern const char* VALID_NAME;
extern const char* INVALID_NAME;
extern const char* VALID_HOST;
extern const char* INVALID_HOST;
extern const char* VALID_USER;
extern const char* INVALID_USER;
extern const char* VALID_PASSWORD;
extern const char* INVALID_PASSWORD;
extern const char* VALID_TIMEOUT;
extern const char* INVALID_TIMEOUT_1;
extern const char* INVALID_TIMEOUT_2;
std::string connectionString(const char* type, const char* name = NULL,
                             const char* host = NULL, const char* user = NULL,
                             const char* password = NULL, const char* timeout = NULL);
};
};
};
