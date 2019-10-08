#pragma once

#include <mutex>

#include <kea/dhcp++/option.h>
#include <kea/dhcp++/classify.h>
#include <kea/dhcp++/pool.h>
#include <kea/util/triplet.h>
#include <kea/util/io_address.h>

namespace kea {
namespace dhcp {

typedef uint32_t SubnetID;
using util::IOAddress;
using util::Triplet;

class Subnet {
public:
    typedef std::vector<std::unique_ptr<Pool>> PoolCollection;

    struct RelayInfo {
        RelayInfo(const IOAddress& addr);
        IOAddress addr_;
    };

    typedef enum  {
        HR_DISABLED,
        HR_OUT_OF_POOL,
        HR_ALL
    } HostResvMode;

    Subnet(const IOAddress& prefix, uint8_t length,
           const Triplet<uint32_t>& t1,
           const Triplet<uint32_t>& t2,
           const Triplet<uint32_t>& valid_lifetime,
           const SubnetID id);

    bool inRange(const IOAddress& addr) const;
    bool inPool(const IOAddress& addr) const;

    Triplet<uint32_t> getValid() const { return (valid_); }

    Triplet<uint32_t> getT1() const { return (t1_); }

    Triplet<uint32_t> getT2() const { return (t2_); }

    IOAddress getLastAllocated() const;

    void setLastAllocated(const IOAddress& addr);

    SubnetID getID() const { return (id_); }

    std::pair<IOAddress, uint8_t> get() const {
        return (std::make_pair(prefix_, prefix_len_));
    }

    void addPool(std::unique_ptr<Pool> pool);
    void delPools();

    const Pool* getPool(const IOAddress& addr, bool anypool = true) const;

    const Pool* getAnyPool() {
        return (getPool(default_pool()));
    }

    const PoolCollection& getPools() const;

    uint64_t getPoolCapacity() const;

    void setIface(const std::string& iface_name);

    std::string getIface() const;

    virtual std::string toText() const;

    void setRelayInfo(const RelayInfo& relay);

    const RelayInfo& getRelayInfo() { return (relay_); }

    bool clientSupported(const ClientClasses& client_classes) const;

    void allowClientClass(const ClientClass& class_name);

    void denyClientClass(const ClientClass& class_name);

    HostResvMode getHostReservationMode() const { return (host_reservation_mode_); }

    void setHostReservationMode(HostResvMode mode) { host_reservation_mode_ = mode; }

    std::mutex last_allocate_addr_lock;

private:
    PoolCollection& getPoolsWritable();
    Subnet(const IOAddress& prefix, uint8_t len,
            const Triplet<uint32_t>& t1, const Triplet<uint32_t>& t2,
            const Triplet<uint32_t>& valid_lifetime, const RelayInfo& relay,
            const SubnetID id);

    uint64_t sumPoolCapacity(const PoolCollection& pools) const;

    SubnetID id_;
    PoolCollection pools_;

    IOAddress prefix_;
    uint8_t prefix_len_;

    Triplet<uint32_t> t1_;
    Triplet<uint32_t> t2_;
    Triplet<uint32_t> valid_;

    IOAddress last_allocated_ia_;

    std::string iface_;
    RelayInfo relay_;
    ClientClasses black_list_;
    ClientClasses white_list_;
    HostResvMode host_reservation_mode_;

public:
    void setSiaddr(const IOAddress& siaddr);
    IOAddress getSiaddr() const;

    void setMatchClientId(const bool match) {
        match_client_id_ = match;
    }
    bool getMatchClientId() const {
        return (match_client_id_);
    }

    OptionCollection& getOptdata() {
        return opt_data_;
    }

    const OptionCollection& getOptdata() const {
        return (const OptionCollection&)opt_data_;
    }
private:
    virtual IOAddress default_pool() const {
        return (IOAddress("0.0.0.0"));
    }

    IOAddress siaddr_;
    bool match_client_id_;
    OptionCollection opt_data_;
};

typedef std::vector<std::unique_ptr<Subnet>> Subnet4Collection;

};
};
