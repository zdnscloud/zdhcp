#include <string>
#include <kea/server/testutil/pgsql_schema.h>

#include <gtest/gtest.h>
#include <libpq-fe.h>

#include <fstream>
#include <sstream>
#include <stdlib.h>

using namespace std;

namespace kea {
namespace server {
namespace testutil {

const char* PGSQL_VALID_TYPE = "type=postgresql";
const char* DATABASE_SCRIPTS_DIR = "/home/vagrant/workspace/code/cpp/zdns-kea/kea/server/testutil/scripts";

string
validPgSQLConnectionString() {
    return (connectionString(PGSQL_VALID_TYPE, VALID_NAME, VALID_HOST,
                             VALID_USER, VALID_PASSWORD));
}

void destroyPgSQLSchema(bool show_err) {
    runPgSQLScript(DATABASE_SCRIPTS_DIR, "dhcpdb_drop.pgsql",
                   show_err);
}

void createPgSQLSchema(bool show_err) {
    runPgSQLScript(DATABASE_SCRIPTS_DIR, "dhcpdb_create.pgsql",
                   show_err);
}

void runPgSQLScript(const std::string& path, const std::string& script_name,
                    bool show_err) {
    std::ostringstream cmd;

    cmd << "export PGPASSWORD=zdns; cat ";
    if (!path.empty()) {
        cmd << " < " << path << "/";
    }

    cmd << script_name
        << " | psql --set ON_ERROR_STOP=1 -A -t -h localhost -q -U zdns -d zdns";

    if (!show_err) {
        cmd << " 2>/dev/null ";
    }

    int retval = ::system(cmd.str().c_str());
    ASSERT_EQ(0, retval) << "runPgSQLSchema failed:" << cmd.str();
}

};
};
};
