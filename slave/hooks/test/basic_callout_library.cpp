#include <kea/hooks/hooks.h>
#include <kea/configure/json_conf.h>
#include <fstream>

using namespace kea::hooks;
using namespace std;

namespace {

extern "C" {

int
context_create(CalloutHandle& handle) {
    handle.setArgument("result", static_cast<int>(10));
    return (0);
}

int
hookpt_one(CalloutHandle& handle) {
    int data;
    handle.getArgument("data_1", data);

    int result;
    handle.getArgument("result", result);

    result += data;
    handle.setArgument("result", result);

    return (0);
}

int
hookpt_two(CalloutHandle& handle) {
    int data;
    handle.getArgument("data_2", data);

    int result;
    handle.getArgument("result", result);

    result *= data;
    handle.setArgument("result", result);

    return (0);
}

int
hookpt_three(CalloutHandle& handle) {
    int data;
    handle.getArgument("data_3", data);

    int result;
    handle.getArgument("result", result);

    result -= data;
    handle.setArgument("result", result);

    return (0);
}

int
version() {
    return (KEA_HOOKS_VERSION);
}

int
load(kea::configure::JsonObject&) {
    return (0);
}

}

};
