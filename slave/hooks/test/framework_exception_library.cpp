#include <kea/hooks/hooks.h>
#include <kea/configure/json_conf.h>

#include <exception>

extern "C" {

int
version() {
    throw std::exception();
}

int
load(const kea::configure::JsonObject&) {
    throw std::exception();
}

int
unload() {
    throw std::exception();
}

};

