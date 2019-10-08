#include <kea/server/ctrl_server_init.h>
#include <kea/server/ctrl_server.h>

namespace kea {
namespace server {

static const int DEFAULT_QUEUE_SIZE = 1000;

Dhcpv4SrvContext::Dhcpv4SrvContext(JsonConf& conf, SubnetMgr& subnet_mgr, 
        BaseHostDataSource& host_mgr, PktQueue& in_queue, PktQueue& out_queue) 
    : in_queue_(in_queue), out_queue_(out_queue) {
    server_.reset(new Dhcpv4Srv(&subnet_mgr, &host_mgr, out_queue));
}

void Dhcpv4SrvContext::run() {
    PktPtr rsp(nullptr);
    PktPtr query(nullptr);
    while(true) {
        in_queue_.blockingRead(query);
        if (!query) { break; }
        server_->processPacket(std::move(query));
    }
}

void Dhcpv4SrvContext::stop() {
    in_queue_.write(nullptr);
}

ControlledDhcpv4Srv::ControlledDhcpv4Srv(const std::string& config_file_path)
    : config_file_path_(config_file_path) {
    conf_ = JsonConf::parseFile(config_file_path_);
    initLog(*conf_);
    initStdOptions();
    initVendorOptions();
    initNic(*conf_);
    Statistics::init();
}

ControlledDhcpv4Srv::~ControlledDhcpv4Srv() {
    stop();
}

void ControlledDhcpv4Srv::run() {
    initCustomOptions(*conf_);
    initHooks(*conf_);
    initClientClasses(*conf_);
    initSubnetMgr();
    initPingCheck(*conf_);
    initRpcAllocateEngine(*conf_);

    createWorkers();
    runWorkers();
}

void
ControlledDhcpv4Srv::initSubnetMgr() {
    host_mgr_.reset(new HostsInMem());
    subnet_mgr_ = createSubnetMgr(*conf_, *host_mgr_);
}

void ControlledDhcpv4Srv::runWorkers() {
    for (auto& context : workers_) {
        std::thread t(std::bind(&Dhcpv4SrvContext::run, context.get()));
        worker_threads_.push_back(std::move(t));
    }

    std::thread send_pkt_thread([](PktQueue* out_queue) {
        PktPtr rsp;
        while(true) {
            out_queue->blockingRead(rsp);
            if (rsp == nullptr) {
                break;
            }
            try {
                logInfo("Dhcpv4Srv ", rsp->toText());
                IfaceMgr::instance().send(*rsp);
            } catch(kea::nic::SocketWriteError& e) {
                logError("Dhcpv4Srv ", "!!socket write exception:$0", e.what());
            }
        } 
    }, out_queue_.get());
    send_pkt_thread_ = std::move(send_pkt_thread);

    int stop_fd = pipefd_[0];
    std::thread recv_pkt_thread([this](PktQueue* in_queue) {
        while(true) {
            PktPtr pkt = IfaceMgr::instance().receive4(this->pipefd_[0], 1000);
            if (this->stop_flag_.load()) {
                break;
            }

            if (pkt) {
                in_queue->blockingWrite(std::move(pkt));
            }
        }
    }, in_queue_.get());
    recv_pkt_thread_ = std::move(recv_pkt_thread);
}

void ControlledDhcpv4Srv::stop() {
    if (stop_flag_.load()) {
        return;
    } 

    write(pipefd_[1], "1", 1);
    stop_flag_.store(true);

    drainQueue(*in_queue_);
    recv_pkt_thread_.join();

    kea::rpc::RpcAllocateEngine::instance().stop();
    Pinger::instance().stop();

    for(auto& context : workers_) {
        context->stop();
    }

    drainQueue(*out_queue_);
    out_queue_->blockingWrite(nullptr);
    send_pkt_thread_.join();

    for(auto& worker_thread : worker_threads_) {
        worker_thread.join();
    }
    close(pipefd_[0]);
    close(pipefd_[1]);
}

void 
ControlledDhcpv4Srv::createWorkers() {
    workers_.clear();
    worker_threads_.clear();

    stop_flag_.store(false);
    pipe(pipefd_);
    if (conf_->root().hasKey("dhcp4.worker-count")) {
        worker_count_ = conf_->root().getInt("dhcp4.worker-count");
    } else {
        worker_count_ = std::thread::hardware_concurrency();
    }
    in_queue_.reset(new PktQueue(worker_count_ * DEFAULT_QUEUE_SIZE));
    out_queue_.reset(new PktQueue(worker_count_ * DEFAULT_QUEUE_SIZE));
    for (int i = 0; i < worker_count_; i++) {
        workers_.push_back(std::unique_ptr<Dhcpv4SrvContext>
                (new Dhcpv4SrvContext(*conf_, *subnet_mgr_, *host_mgr_, *in_queue_, *out_queue_)));
    }
}

kea::controller::CmdResult ControlledDhcpv4Srv::handleCmd(const std::string& cmd_name, JsonObject params) {
    if (cmd_name == "reconfig") {
        return reconfigCmd();
    } else if (cmd_name == "stop") {
        stop();
        return std::make_pair(std::string("stop"), true);
    } else {
        return std::make_pair(std::string("unknowned command:") + cmd_name, false);
    }
}

kea::controller::CmdResult 
ControlledDhcpv4Srv::reconfigCmd() {
    auto conf_backup = std::move(conf_);
    stop();

    try {
        conf_ = JsonConf::parseFile(config_file_path_);
        run();
        return std::make_pair(std::string("reconfig"), true);
    } catch (const std::exception &e){
        logError("Dhcpv4Srv ", "!!!reconfig get exception: $0", e.what());
        conf_ = std::move(conf_backup);
        run();
        logWarning("Dhcpv4Srv ", "reconfig failed and using old configure to run");
        return std::make_pair(e.what(), false);
    }
}

};
};
