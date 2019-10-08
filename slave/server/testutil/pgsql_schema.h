#pragma once

#include <kea/server/testutil/schema.h>
#include <string>

namespace kea {
namespace server {
namespace testutil {

extern const char* PGSQL_VALID_TYPE;

std::string validPgSQLConnectionString();

void destroyPgSQLSchema(bool show_err = false);
void createPgSQLSchema(bool show_err = false);
void runPgSQLScript(const std::string& path, const std::string& script_name, bool show_err);

};
};
};
