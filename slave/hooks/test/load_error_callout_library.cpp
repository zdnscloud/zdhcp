#include <kea/hooks/hooks.h>

using namespace kea::hooks;

extern "C" {

int
version() {
    return (KEA_HOOKS_VERSION);
}

int
load(LibraryHandle&) {
    return (1);
}

int
unload() {
    return (1);
}
};

