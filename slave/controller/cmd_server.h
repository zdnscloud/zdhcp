#pragma once

#include <kea/configure/json_conf.h>
#include <kea/exceptions/exceptions.h>
#include <kea/controller/cmd_server_base.h>
#include <kea/controller/tcp_server.h>

#include <unordered_map>
#include <mutex>

namespace kea {
namespace controller {

class DuplicateCmdHandler : public Exception {
public:
    DuplicateCmdHandler(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

class UnknownCmd: public Exception {
public:
    UnknownCmd(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};


typedef std::pair<std::string, bool> CmdResult;
class CmdHandler {
public:
    virtual ~CmdHandler() = default;
    virtual CmdResult handleCmd(const std::string& cmd_name, kea::configure::JsonObject params) = 0;
};

typedef std::function<void(bool)> CmdSrvHandler;
class CmdServer : public CmdServerBase{
public:
    explicit CmdServer(int port);
    ~CmdServer();

    void registerHandler(const std::string& cmd_name, CmdHandler* handler);
    void unRegisterHandler(const std::string& cmd_name);
    void run();
    virtual void stop();
    virtual std::string processCmd(const char* cmd_msg, uint32_t cmd_len, CmdSrvHandler callback);

private:
    CmdResult runCmd(const std::string& cmd_name, kea::configure::JsonObject params);
    std::string processCmdResult(CmdResult cmdResult);
    
    std::unique_ptr<TcpServer> tcp_server_;
    std::unordered_map<std::string, CmdHandler*> cmd_handlers_;
    std::mutex cmd_execute_mutex_;
    std::atomic<bool> stop_;
};

};
};
