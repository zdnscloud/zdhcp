#include <kea/hooks/hooks.h>

using namespace kea::hooks;

extern "C" {

int
context_create(CalloutHandle& handle) {
    handle.setArgument("result", static_cast<int>(5));
    return (0);
}

int
hookpt_one(CalloutHandle& handle) {
    int data;
    handle.getArgument("data_1", data);

    int result = 5;
    result *= data;
    handle.setArgument("result", result);

    return (0);
}

int
hookpt_two(CalloutHandle& handle) {
    int data;
    handle.getArgument("data_2", data);

    int result;
    handle.getArgument("result", result);

    result += data;
    handle.setArgument("result", result);

    return (0);
}

int
hookpt_three(CalloutHandle& handle) {
    int data;
    handle.getArgument("data_3", data);

    int result;
    handle.getArgument("result", result);

    result *= data;
    handle.setArgument("result", result);

    return (0);
}

int
version() {
    return (KEA_HOOKS_VERSION);
}

int load(LibraryHandle& handle) {
    return (0);
}

};
