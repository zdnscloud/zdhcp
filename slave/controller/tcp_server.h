#pragma once

#include <kea/controller/tcp_session_manager.h>
#include <kea/controller/cmd_server_base.h>
#include <asio.hpp>
#include <atomic>

namespace kea{
namespace controller{

class TcpServer {
    public:
        explicit TcpServer(uint32_t port, CmdServerBase* cmd_srv_base);
        ~TcpServer();
        TcpServer(const TcpServer&) = delete;
        TcpServer& operator=(const TcpServer&) = delete;
        
        void run();
        void stop();

    private:
        void doAccept();

        std::atomic<bool> stop_;
        asio::io_service io_service_;
        asio::ip::tcp::socket socket_;
        asio::ip::tcp::acceptor acceptor_;
        asio::ip::tcp::endpoint endpoint_;
        TcpSessionManager session_manager_;
        CmdServerBase* cmd_srv_base_;
};

}
}
