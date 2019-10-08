#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <fstream>
#include <iostream>
#include <kea/hooks/hooks.h>
#include <kea/dhcp++/pkt.h>
#include <kea/dhcp++/hwaddr.h>
#include <kea/dhcp++/option_string.h>
#include <kea/dhcp++/option_int_array.h>
#include <kea/controller/cmd_server.h>

using namespace std;
using namespace kea::dhcp;
using namespace kea::hooks;

namespace kea{
namespace statis{

class Statistics : public kea::controller::CmdHandler{
    public:
        Statistics();
        ~Statistics();

        void stop(); 
        int count_recv(Pkt* query);
        int count_send(Pkt* query, Pkt* rsp);
        static Statistics& instance();
        static void init();
        virtual kea::controller::CmdResult handleCmd(const std::string& cmd_name, kea::configure::JsonObject params);

    private:
        void count_proc();

        mutex mx_;
        mutex lps_mx_;
        ofstream fp_;
        std::string lps_;
        atomic_bool started_;
        atomic_uint discover_;
        atomic_uint offer_;
        atomic_uint request_;
        atomic_uint ack_;
        thread* output_thr_;
};

}
}
