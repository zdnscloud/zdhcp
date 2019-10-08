#include <kea/server/testutil/schema.h>
#include <string>

using namespace std;

namespace kea {
namespace server {
namespace testutil {

const char* INVALID_TYPE = "type=unknown";
const char* VALID_NAME = "name=zdns";
const char* INVALID_NAME = "name=invalidname";
const char* VALID_HOST = "host=localhost";
const char* INVALID_HOST = "host=invalidhost";
const char* VALID_USER = "user=zdns";
const char* INVALID_USER = "user=invaliduser";
const char* VALID_PASSWORD = "password=zdns";
const char* INVALID_PASSWORD = "password=invalid";
const char* VALID_TIMEOUT = "connect-timeout=10";
const char* INVALID_TIMEOUT_1 = "connect-timeout=foo";
const char* INVALID_TIMEOUT_2 = "connect-timeout=-17";

string connectionString(const char* type, const char* name, const char* host,
                        const char* user, const char* password, const char* timeout) {
    const string space = " ";
    string result = "";

    if (type != NULL) {
        result += string(type);
    }
    if (name != NULL) {
        if (! result.empty()) {
            result += space;
        }
        result += string(name);
    }

    if (host != NULL) {
        if (! result.empty()) {
            result += space;
        }
        result += string(host);
    }

    if (user != NULL) {
        if (! result.empty()) {
            result += space;
        }
        result += string(user);
    }

    if (password != NULL) {
        if (! result.empty()) {
            result += space;
        }
        result += string(password);
    }

    if (timeout != NULL) {
        if (! result.empty()) {
            result += space;
        }
        result += string(timeout);
    }

    return (result);
}

};
};
};
