#include <kea/hooks/hooks.h>
#include <kea/configure/json_conf.h>
#include <iostream>

using namespace kea::hooks;
using namespace kea::configure;

extern "C" {

int
version() {
    return (KEA_HOOKS_VERSION);
}

int load(JsonObject& parameters) {
    std::string string_elem  = parameters.getString("svalue");
    int int_elem     = parameters.getInt("ivalue");
    bool bool_elem    = parameters.getBool("bvalue");

    if (string_elem != "string value") {
        return (1);
    }

    if (int_elem!= 42) {
        return (2);
    }

    if (bool_elem != true) {
        return (9);
    }

    return (0);
}
    

};
