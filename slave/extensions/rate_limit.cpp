#include <kea/hooks/hooks.h>
#include <kea/configure/json_conf.h>
#include <kea/dhcp++/pkt.h>
#include <kea/dhcp++/dhcp4.h>
#include <kea/util/lru_cache.h>

#include <cstdint>
#include <chrono>
#include <cassert>
#include <mutex>

using namespace kea::hooks;
using namespace kea::dhcp;
using namespace kea::util;
using namespace std;

namespace {

typedef std::chrono::system_clock::time_point time_t; 
static const int MAX_REQUEST_COUNT_PER_SEC = 100;
static const std::string MAX_MAC_COUNT_FIELD_NAME{"max-mac-count"};
static const std::string MAX_REQUEST_PER_SEC_FIELD_NAME{"max-request-per-sec"};

struct RequestRecord {
    int queryCount_;
    time_t lastRequestTime_;
};

typedef kea::util::LruCache<uint64_t, RequestRecord> request_record_cache_t;

class RequestThrottle {
public:
    explicit RequestThrottle(int max_mac_count, int max_request_count_per_sec)
        :max_request_count_per_sec_(max_request_count_per_sec) {
        if (max_request_count_per_sec_ <= 0) {
            max_request_count_per_sec_ = MAX_REQUEST_COUNT_PER_SEC;
        }
        cache_.reset(new request_record_cache_t(max_mac_count));
    }

    bool isRequestAllowed(Pkt* query) {
        const HWAddr& addr = query->getHWAddr();
        if (addr.htype_ != HTYPE_ETHER) {
            return (true);
        }

        if (addr.hwaddr_.size() != 6) {
            return (false);
        }

        std::uint64_t mac = (std::uint64_t(addr.hwaddr_[0]) << 40) | 
                            (std::uint64_t(addr.hwaddr_[1]) << 32) | 
                            (std::uint64_t(addr.hwaddr_[2]) << 24) | 
                            (std::uint64_t(addr.hwaddr_[3]) << 16) | 
                            (std::uint64_t(addr.hwaddr_[4]) << 8)  | 
                            std::uint64_t(addr.hwaddr_[5]);
        
        bool isAllowed = false;
        time_t now =  std::chrono::system_clock::now();
        std::lock_guard<std::mutex> lock(cache_lock_);
        RequestRecord* record = nullptr;
        if (cache_->find(mac, &record)) {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - record->lastRequestTime_).count() > 1) {
                record->queryCount_ = 1;
                record->lastRequestTime_ = now;
                isAllowed = true;
            } else {
                if ((record->queryCount_ + 1) < max_request_count_per_sec_) {
                    record->queryCount_ += 1;
                    isAllowed = true;
                } 
            }
        } else {
            isAllowed = true;
            cache_->put(mac, RequestRecord({1, now}));
        }
        return (isAllowed);
    }

private:
    std::unique_ptr<request_record_cache_t> cache_;
    std::mutex cache_lock_;
    int max_request_count_per_sec_;

};

static RequestThrottle* throttle_instance_;
};

extern "C" {

int context_create(CalloutHandle& handle) {
}

int pkt4_receive(CalloutHandle& handle) {
    Pkt* query = nullptr;
    handle.getArgument("query4", query);
    if (query != nullptr) {
        if ((throttle_instance_->isRequestAllowed(query)) == false) {
            handle.setStatus(CalloutHandle::NEXT_STEP_SKIP);
        }
    }

    return (0);
}

int pkt4_send(CalloutHandle& handle) {
    return (0);
}

int version() {
    return (KEA_HOOKS_VERSION);
}

int load(kea::configure::JsonObject& conf) {
    int max_mac_count = 0;
    if (conf.hasKey(MAX_MAC_COUNT_FIELD_NAME)) {
        max_mac_count = conf.getInt(MAX_MAC_COUNT_FIELD_NAME);
    }

    int max_request_count_per_sec = 0;
    if (conf.hasKey(MAX_REQUEST_PER_SEC_FIELD_NAME)) {
        max_request_count_per_sec = conf.getInt(MAX_REQUEST_PER_SEC_FIELD_NAME);
    }

    throttle_instance_ = new RequestThrottle(max_mac_count, max_request_count_per_sec);
    return (0);
}

int unload() {
    if (throttle_instance_ != nullptr) {
        delete throttle_instance_;
        throttle_instance_ = nullptr;
    }
    return (0);
}

};
