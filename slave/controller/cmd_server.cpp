#include <kea/controller/cmd_server.h>

namespace kea {
namespace controller {

using namespace kea::configure;

CmdServer::CmdServer(int port) : tcp_server_(new TcpServer(port,this)){
    stop_.store(false);
}

CmdServer::~CmdServer() {
    stop();
}

void
CmdServer::run() {
    tcp_server_->run();
}

void
CmdServer::stop() {
    if (stop_.load()) {return;}

    stop_.store(true);
    tcp_server_->stop();
}

void 
CmdServer::registerHandler(const std::string& cmd_name, 
                           CmdHandler* handler) {
    auto i = cmd_handlers_.find(cmd_name);
    if (i != cmd_handlers_.end()) {
        kea_throw(DuplicateCmdHandler, "duplicate handler for" << cmd_name);
    }
    cmd_handlers_.insert(std::make_pair(cmd_name, handler));
}

void 
CmdServer::unRegisterHandler(const std::string& cmd_name) {
    auto i = cmd_handlers_.find(cmd_name);
    if (i == cmd_handlers_.end()) {
        kea_throw(UnknownCmd, cmd_name);
    }
    cmd_handlers_.erase(i);
}

CmdResult 
CmdServer::runCmd(const std::string& cmd_name, kea::configure::JsonObject params) {
    auto i = cmd_handlers_.find(cmd_name);
    std::lock_guard<std::mutex> guard(cmd_execute_mutex_);
    if (i != cmd_handlers_.end()) {
        return i->second->handleCmd(cmd_name, params);
    } else {
        return std::make_pair("unknown cmd " + cmd_name, false);
    }
}

std::string 
CmdServer::processCmdResult(CmdResult cmdResult) {
    Json resultJson;
    if (cmdResult.second == false) {
        logDebug("CmdSrv    ", "Process cmd failed and error_info: $0", cmdResult.first);
        resultJson = Json::object{{"succeed", Json(cmdResult.second)}, {"error_info", Json(cmdResult.first)}};
    } else {
        resultJson = Json::object{{"succeed", Json(cmdResult.second)}, {"result", Json(cmdResult.first)}};
    }
    return resultJson.dump();
}

std::string 
CmdServer::processCmd(const char* cmd_msg, uint32_t cmd_len, CmdSrvHandler callback) {
    std::string cmd_str(cmd_msg, cmd_len);
    std::unique_ptr<kea::configure::JsonConf> cmd_wrapper;
    JsonObject cmd(nullptr);
    try {
        cmd_wrapper = JsonConf::parseString(cmd_str);
    } catch(std::exception& e) {
        logError("CmdSrv    ", "Json parse string with unexpected error: $0", e.what());
        return std::string("");
    }

    cmd = cmd_wrapper->root();
    if (cmd.hasKey("name") == false) {
        logError("CmdSrv    ", "Json parse cmd key with error: no key for name");
        return std::string("");
    }

    std::string cmd_name = cmd.getString("name");

    if (cmd_name == "stop") {
        callback(true);
    }

    return processCmdResult(runCmd(cmd_name, cmd));
}

};
};
