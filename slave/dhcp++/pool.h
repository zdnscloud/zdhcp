#pragma once

#include <kea/util/io_address.h>
#include <vector>

namespace kea {
namespace dhcp {

using namespace kea::util;

class Pool {
public:
    Pool(const IOAddress& first, const IOAddress& last);

    Pool(const IOAddress& prefix, uint8_t prefix_len);

    uint32_t getId() const { return (id_); }

    const IOAddress& getFirstAddress() const { return (first_); }

    const IOAddress& getLastAddress() const { return (last_); }

    bool inRange(const IOAddress& addr) const;

    virtual std::string toText() const;

    virtual ~Pool() {}

    uint64_t getCapacity() const { return (capacity_); }

protected:
    static uint32_t getNextID() {
        static uint32_t id = 0;
        return (id++);
    }

    uint32_t id_;
    IOAddress first_;
    IOAddress last_;
    std::string comments_;
    uint64_t capacity_;
};

};
};
