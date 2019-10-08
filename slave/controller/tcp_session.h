#pragma once 

#include <kea/controller/cmd_server_base.h>
#include <asio.hpp>
#include <atomic>
#include <memory>

using namespace asio::ip;

namespace kea{
namespace controller{

class TcpSessionManager;

class TcpSession : public std::enable_shared_from_this<TcpSession> {
    public:
        explicit TcpSession(tcp::socket socket, TcpSessionManager& session_manager, CmdServerBase* cmd_srv_base, asio::ip::tcp::endpoint endpoint);
        ~TcpSession();

        TcpSession(const TcpSession&) = delete;
        TcpSession& operator=(const TcpSession&) = delete;

        void run();
        void stop();

    private:
        void readMsgHeader();
        void readMsgBody();
        void writeMsg(std::string msg);

        std::atomic<bool> stop_;
        std::atomic<bool> server_stop_;
        asio::ip::tcp::socket socket_;
        asio::ip::tcp::endpoint endpoint_;
        char msg_len_[2];
        static const int MAX_MSG_BODY_LEN = 2048;
        char msg_body_[MAX_MSG_BODY_LEN];
        TcpSessionManager& session_manager_;
        CmdServerBase* cmd_srv_base_;
};

typedef std::shared_ptr<TcpSession> SessionSharedPtr;

}
}
