#include <kea/dhcp++/option_space.h>
#include <kea/dhcp++/subnet.h>
#include <kea/util/ipaddress_extend.h>
#include <sstream>


namespace kea {
namespace dhcp {

Subnet::Subnet(const IOAddress& prefix, uint8_t len,
        const Triplet<uint32_t>& t1, const Triplet<uint32_t>& t2,
        const Triplet<uint32_t>& valid_lifetime, const RelayInfo& relay,
        const SubnetID id)
    :id_(id), 
    prefix_(prefix), prefix_len_(len), t1_(t1), t2_(t2), 
    valid_(valid_lifetime), last_allocated_ia_(lastAddrInNetwork(prefix, len)), 
    relay_(relay), host_reservation_mode_(HR_ALL), siaddr_(IOAddress("0.0.0.0")) {
        assert(prefix.isV4() && len <= 32);
}

Subnet::RelayInfo::RelayInfo(const IOAddress& addr) :addr_(addr) {}

bool Subnet::inRange(const IOAddress& addr) const {
    IOAddress first = firstAddrInNetwork(prefix_, prefix_len_);
    IOAddress last = lastAddrInNetwork(prefix_, prefix_len_);

    return ((first <= addr) && (addr <= last));
}

void Subnet::setRelayInfo(const RelayInfo& relay) {
    relay_ = relay;
}

bool Subnet::clientSupported(const ClientClasses& classes) const {
    if (!white_list_.empty()) {
        for (ClientClasses::const_iterator it = white_list_.begin(); 
                it != white_list_.end(); ++it) {
            if (classes.contains(*it)) {
                return (true);
            }
        }
        return (false);
    }

    if (!black_list_.empty()) {
        for (ClientClasses::const_iterator it = black_list_.begin(); 
                it != black_list_.end(); ++it) {
            if (classes.contains(*it)) {
                return (false);
            }
        }
        return (true);
    }
    return (true);
}

void Subnet::allowClientClass(const ClientClass& class_name) {
    white_list_.insert(class_name);
}

void Subnet::denyClientClass(const ClientClass& class_name) {
    black_list_.insert(class_name);
}

IOAddress Subnet::getLastAllocated() const {
    return last_allocated_ia_;
}

void Subnet::setLastAllocated(const IOAddress& addr) {
    last_allocated_ia_ = addr;
}

std::string Subnet::toText() const {
    std::stringstream tmp;
    tmp << prefix_.toText() << "/" << static_cast<unsigned int>(prefix_len_);
    return (tmp.str());
}

uint64_t Subnet::getPoolCapacity() const {
    return sumPoolCapacity(pools_);
}

uint64_t Subnet::sumPoolCapacity(const PoolCollection& pools) const {
    uint64_t sum = 0;
    for (auto& p : pools) {
        uint64_t x = p->getCapacity();
        if (x > std::numeric_limits<uint64_t>::max() - sum) {
            return (std::numeric_limits<uint64_t>::max());
        }
        sum += x;
    }
    return (sum);
}

Subnet::Subnet(const IOAddress& prefix, uint8_t length,
        const Triplet<uint32_t>& t1, const Triplet<uint32_t>& t2,
        const Triplet<uint32_t>& valid_lifetime, const SubnetID id)
    : Subnet(prefix, length, t1, t2, valid_lifetime, RelayInfo(IOAddress("0.0.0.0")), id) {
    siaddr_ = IOAddress("0.0.0.0");
    match_client_id_ = true;
    if (!prefix.isV4()) {
        kea_throw(BadValue, "Non IPv4 prefix " << prefix.toText()
                << " specified in subnet4");
    }
}

void Subnet::setSiaddr(const IOAddress& siaddr) {
    if (!siaddr.isV4()) {
        kea_throw(BadValue, "Can't set siaddr to non-IPv4 address "
                << siaddr);
    }
    siaddr_ = siaddr;
}

IOAddress Subnet::getSiaddr() const {
    return (siaddr_);
}

const Subnet::PoolCollection& Subnet::getPools() const {
    return (pools_);
}

Subnet::PoolCollection& Subnet::getPoolsWritable() {
    return (pools_);
}

const Pool* Subnet::getPool(const IOAddress& hint, bool anypool /* true */) const {
    const Pool* candidate = nullptr;
    for (auto& pool : pools_) {
        // if we won't find anything better, then let's just use the first pool
        if (anypool && candidate == nullptr) {
            candidate = pool.get();
        }

        if (pool->inRange(hint)) {
            return (pool.get());
        }
    }
    return (candidate);
}

void Subnet::addPool(std::unique_ptr<Pool> pool) {
    if (!inRange(pool->getFirstAddress()) || !inRange(pool->getLastAddress())) {
        kea_throw(BadValue, "a pool " ", with the following address range: "
                << pool->getFirstAddress() << "-"
                << pool->getLastAddress() << " does not match"
                << " the prefix of a subnet: "
                << prefix_ << "/" << static_cast<int>(prefix_len_)
                << " to which it is being added");
    }
    getPoolsWritable().push_back(std::move(pool));
}

void Subnet::delPools() {
    pools_.clear();
}

void Subnet::setIface(const std::string& iface_name) {
    iface_ = iface_name;
}

std::string Subnet::getIface() const {
    return (iface_);
}

bool Subnet::inPool(const IOAddress& addr) const {
    if (!inRange(addr)) {
        return (false);
    }

    for (auto& pool : pools_) {
        if (pool->inRange(addr)) {
            return (true);
        }
    }
    return (false);
}

};
};
