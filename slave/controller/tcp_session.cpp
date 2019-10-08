#include <kea/controller/tcp_session.h>
#include <kea/controller/tcp_session_manager.h>
#include <kea/logging/logging.h>

using namespace kea::logging;

namespace kea{
namespace controller{

TcpSession::TcpSession(tcp::socket socket, TcpSessionManager& session_manager, CmdServerBase* cmd_srv_base, asio::ip::tcp::endpoint endpoint) : socket_(std::move(socket)), session_manager_(session_manager), cmd_srv_base_(cmd_srv_base), endpoint_(endpoint) {
    stop_.store(false);
    server_stop_.store(false);
}

TcpSession::~TcpSession() {
    stop();
}

void 
TcpSession::run() {
    session_manager_.addSession(shared_from_this());
    readMsgHeader();
}

void 
TcpSession::stop() {
    if (stop_.load()){ return; }

    stop_.store(true);
    socket_.close();
}

void 
TcpSession::readMsgHeader() {
    asio::async_read(socket_, asio::buffer(msg_len_, 2), [=](std::error_code ec, std::size_t ) {
           if (!ec) {
               readMsgBody();
           } else {
               logDebug("TcpSrv    ", "Read msg header failed: $0", ec.message().c_str());
               session_manager_.removeSession(shared_from_this());
           } 
    });
}

void 
TcpSession::readMsgBody(){
    int msg_len = (int(static_cast<uint8_t>(msg_len_[0])) << 8) + static_cast<uint8_t>(msg_len_[1]);
    if (msg_len > 0 && msg_len < MAX_MSG_BODY_LEN) {
        memset(msg_body_, 0, msg_len+2);
        asio::async_read(socket_, asio::buffer(msg_body_, msg_len), [=](std::error_code ec, std::size_t) {
            if (!ec) {
                logDebug("TcpSrv    ", "Got cmd: $0 from client $1", std::string(msg_body_, msg_len), endpoint_.address().to_string());
                auto result = cmd_srv_base_->processCmd(msg_body_, msg_len, [=](bool stop) {
                    if (stop) {
                        logDebug("TcpSrv    ", "Got stop cmd from client: $0", endpoint_.address().to_string());
                        server_stop_.store(true);
                    }
                });
                writeMsg(result);
            } else {
                logError("TcpSrv    ", "Read msg body failed: $0", ec.message().c_str());
                session_manager_.removeSession(shared_from_this());
            }
        });
    } else {
        logError("TcpSrv    ", "Read Msg got invalid msg len $0 not between 0 and 2048", msg_len);
        session_manager_.removeSession(shared_from_this());
    }
}

void 
TcpSession::writeMsg(std::string msg) {
    auto msg_len = msg.length();
    if (msg_len > 0 && msg_len < MAX_MSG_BODY_LEN) {
        memset(msg_body_, 0, msg_len+2);
        msg_body_[0] = (msg_len & 0xff00) >> 8;
        msg_body_[1] = (msg_len & 0xff);
        memcpy(msg_body_ + 2, msg.c_str(), msg_len);
        asio::async_write(socket_, asio::buffer(msg_body_, msg_len + 2), [=](std::error_code ec, std::size_t) {
            if(!ec){
                logDebug("TcpSrv    ", "send result: $0 to client $1", msg.c_str(), endpoint_.address().to_string());
                if (server_stop_.load()) {
                    cmd_srv_base_->stop();
                } else {
                    readMsgHeader();
                }
            } else {
                logError("TcpSrv    ", "Write msg failed: $0", ec.message());
                session_manager_.removeSession(shared_from_this());
            }
        });
    } else {
        logError("TcpSrv    ", "Write Msg got invalid msg len $0 not between 0 and 2048", msg_len);
        session_manager_.removeSession(shared_from_this());
    }
}

}
}
