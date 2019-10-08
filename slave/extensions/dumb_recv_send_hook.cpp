#include <kea/hooks/hooks.h>
#include <kea/configure/json_conf.h>
#include <kea/dhcp++/pkt.h>

using namespace kea::hooks;
using namespace kea::dhcp;
using namespace std;

namespace {

extern "C" {

int context_create(CalloutHandle& handle) {
}

int pkt4_receive(CalloutHandle& handle) {
    Pkt* query = nullptr;
    handle.getArgument("query4", query);
    if (query != nullptr) {
        std::cout << "get query:" << query->toText() << std::endl;
    }

    return (0);
}

int pkt4_send(CalloutHandle& handle) {
    Pkt* rsp = nullptr;
    handle.getArgument("response4", rsp);
    if (rsp != nullptr) {
        std::cout << "send response:" << rsp->toText() << std::endl;
    }
    return (0);
}

int version() {
    return (KEA_HOOKS_VERSION);
}

int load(kea::configure::JsonObject&) {
    return (0);
}

}
};
