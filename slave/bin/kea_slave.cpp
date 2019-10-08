#include <kea/server/ctrl_server.h>
#include <kea/controller/cmd_server.h>
#include <kea/statistics/pkt_statistic.h>
#include <kea/version.h>

#include <gflags/gflags.h>

using namespace kea;
using namespace kea::server;
using namespace kea::controller;
using namespace kea::util;
using namespace kea::statis;
using namespace std;

DECLARE_string(conf);
DECLARE_int32(port);

DEFINE_string(conf, "kea.conf", "configure file to read");
DEFINE_int32(port, 6000, "cmd server port");

int main(int argc, char** argv) {
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    std::unique_ptr<ControlledDhcpv4Srv> dhcp_server(new ControlledDhcpv4Srv(FLAGS_conf));
    dhcp_server->run();

    logInfo("KeaSlave  ", "====== The Version ID of kea_slave: $0 ======", KEA_SLAVE_VERSION_ID);

    std::unique_ptr<CmdServer> cmd_server(new CmdServer(FLAGS_port));
    cmd_server->registerHandler("stop", dhcp_server.get());
    cmd_server->registerHandler("reconfig", dhcp_server.get());
    cmd_server->registerHandler("statis_lps", &Statistics::instance());

    cmd_server->run();

    return 0;
}
