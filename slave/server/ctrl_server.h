#pragma once
#include <thread>
#include <atomic>

#include <kea/configure/json_conf.h>
#include <kea/server/server.h>
#include <kea/controller/cmd_server.h>
#include <kea/util/encode/hex.h>
#include <folly/MPMCQueue.h>

namespace kea {
namespace server {

typedef folly::MPMCQueue<PktPtr> PktQueue;
typedef std::unique_ptr<PktQueue> PktQueuePtr;

class Dhcpv4SrvContext {
public:
    explicit Dhcpv4SrvContext(kea::configure::JsonConf& conf, SubnetMgr& subnet_mgr, BaseHostDataSource& host_mgr, PktQueue& in_queue, PktQueue& out_queue);
    void run();
    void stop();

    std::unique_ptr<Dhcpv4Srv> server_;
    PktQueue& in_queue_;
    PktQueue& out_queue_;
};

class ControlledDhcpv4Srv : public kea::controller::CmdHandler {
public:
    ControlledDhcpv4Srv(const std::string& config_file_path); 
    ~ControlledDhcpv4Srv();

    void run();
    void stop();
    virtual kea::controller::CmdResult handleCmd(const std::string& cmd_name, kea::configure::JsonObject params);

private:
    void createWorkers();
    void runWorkers();
    void initSubnetMgr();
    kea::controller::CmdResult reconfigCmd();

    std::string config_file_path_;
    int   worker_count_;
    std::vector<std::unique_ptr<Dhcpv4SrvContext>> workers_;
    std::vector<std::thread> worker_threads_;
    std::thread send_pkt_thread_;
    std::thread recv_pkt_thread_;
    std::unique_ptr<kea::configure::JsonConf> conf_; 
    int pipefd_[2];
    std::atomic<bool> stop_flag_;
    std::unique_ptr<SubnetMgr> subnet_mgr_;
    std::unique_ptr<BaseHostDataSource> host_mgr_;
    PktQueuePtr in_queue_;
    PktQueuePtr out_queue_;
};
}; 
};
