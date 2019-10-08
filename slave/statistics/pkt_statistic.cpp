#include <kea/statistics/pkt_statistic.h>
#include <kea/logging/logging.h>

using namespace kea::logging;

namespace kea{
namespace statis{

static Statistics *SingletonStatis = nullptr;


Statistics::Statistics() : started_(true), discover_(0), offer_(0), request_(0), ack_(0), lps_("0 0 0 0") {
    try {
        fp_.open("/usr/local/zddi/dhcp/db/dhcpd.actives", ofstream::app);
        output_thr_ = new thread(&Statistics::count_proc, this);
    }
    catch (const std::exception& ex) {
        started_ = false;
        logError("Statistics", "Construct unexpected error: $0", ex.what());
    }
}

Statistics::~Statistics() {
    stop();
}

void
Statistics::init() {
    if (SingletonStatis != nullptr) {
        SingletonStatis->stop();
        delete SingletonStatis;
    }

    SingletonStatis = new Statistics();
}

Statistics&
Statistics::instance(){
    return *SingletonStatis;
}
void 
Statistics::stop() {
    try {
        started_ = false;
        if (output_thr_ != nullptr) {
            output_thr_->join();
            delete output_thr_;
            output_thr_ = nullptr;
        }        
        fp_.close();
    }
    catch (const std::exception& ex) {
        logError("Statistics", "Stop unexpected error: $0", ex.what());
    }
}

int 
Statistics::count_recv(Pkt* query) {
    try {
        switch (query->getType()) {
            case DHCPDISCOVER:
                discover_++;
                break;
            case DHCPREQUEST:
                request_++;
                break;
            default:
                break;
        }
    }
    catch (const std::exception& ex) {
        logError("Statistics", "!!!pkt4_receive unexpected error: $0", ex.what());
        return (1);
    }

    return 0;
}

int 
Statistics::count_send(Pkt* query, Pkt* rsp) {
    try {
        switch (rsp->getType()) {
            case DHCPOFFER:
                offer_++;
                break;
            case DHCPACK:
                {
                    ack_++;
                    stringstream ss;
                    ss << query->getHWAddr().toText(false) << "#####";
                    const OptionUint8Array *option_prl = dynamic_cast<const OptionUint8Array*>
                        (query->getOption(DHO_DHCP_PARAMETER_REQUEST_LIST));
                    if (option_prl != nullptr) {
                        const vector<uint8_t>& requested_opts = option_prl->getValues();
                        for (vector<uint8_t>::const_iterator value = requested_opts.begin();
                                value != requested_opts.end(); ++value) {
                            if (value != requested_opts.begin())
                                ss << ",";
                            ss << static_cast<int>(*value);
                        }
                        ss << "#####";
                    }

                    const OptionString *vendor_class = dynamic_cast<const OptionString*>
                        (query->getOption(DHO_VENDOR_CLASS_IDENTIFIER));
                    if (vendor_class != nullptr) {
                        ss << vendor_class->getValue();
                    }
                    ss << "\n";

                    std::unique_lock<std::mutex> lock(mx_);
                    fp_ << ss.str();
                    fp_.flush();
                    lock.unlock();
                }
                break;
            default:
                break;
        }
    }
    catch (const std::exception& ex) {
        logError("Statistics", "!!!pkt4_send unexpected error: $0", ex.what());
        return (1);
    }

    return 0;
}

void 
Statistics::count_proc() {
    while (started_) {
        try {
            sleep(1);
            stringstream buf;
            buf << discover_.load() << " " << offer_.load() << " " << request_.load() << " " 
                << ack_.load();
            discover_ = offer_ = request_ = ack_ = 0;
            std::lock_guard<std::mutex> guard(lps_mx_);
            lps_ = buf.str();
        }
        catch (const std::exception& ex) {
            logError("Statistics", "!!!statistics thread unexpected error: $0", ex.what());
        }
    }
}

kea::controller::CmdResult 
Statistics::handleCmd(const std::string& cmd_name, kea::configure::JsonObject params) {
    if (cmd_name == "statis_lps") {
        std::lock_guard<std::mutex> guard(lps_mx_);
        auto lps = lps_;
        return std::make_pair(lps, true);
    } else {
        return std::make_pair(std::string("unknowned command:") + cmd_name, false);
    }
}

}
}

