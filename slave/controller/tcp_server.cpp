#include <kea/controller/tcp_server.h>
#include <kea/controller/tcp_session.h>
#include <kea/logging/logging.h>

using namespace kea::logging;

namespace kea{
namespace controller{

TcpServer::TcpServer(uint32_t port, CmdServerBase* cmd_srv_base) : acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)), socket_(io_service_), cmd_srv_base_(cmd_srv_base) {
    stop_.store(false);
    doAccept();
}

TcpServer::~TcpServer() {
    stop();
}

void 
TcpServer::run() {
    io_service_.run();
}

void
TcpServer::stop() {
    if (stop_.load()) {return;}

    stop_.store(true);
    acceptor_.close();
    session_manager_.stop();
    io_service_.stop();
}

void 
TcpServer::doAccept() {
    acceptor_.async_accept(socket_, endpoint_, [=](std::error_code ec) {
            if (!ec) {
                logDebug("TcpSrv    ", "Got Session from client: $0", endpoint_.address().to_string());
                std::make_shared<TcpSession>(std::move(socket_), session_manager_, cmd_srv_base_, endpoint_)->run();
            } else {
                logError("TcpSrv    ", "Accept session failed: $0", ec.message().c_str());
            }
            if (this->stop_.load() == false) {
                doAccept();
            }
    });
}

}
}
