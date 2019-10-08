#pragma once

#include <kea/util/io_address.h>
#include <kea/dhcp++/option.h>
#include <kea/util/buffer.h>

#include <map>
#include <string>
#include <vector>

using namespace kea::util;

namespace kea {
namespace dhcp {

class Option4AddrLst : public Option {
public:

    typedef std::vector<IOAddress> AddressContainer;
    Option4AddrLst(uint8_t type);
    Option4AddrLst(uint8_t type, const AddressContainer& addrs);
    Option4AddrLst(uint8_t type, const IOAddress& addr);
    Option4AddrLst(uint8_t type, OptionBufferConstIter first, OptionBufferConstIter last);

    virtual std::unique_ptr<Option> clone() const;

    virtual void pack(kea::util::OutputBuffer& buf) const;

    virtual std::string toText(int indent = 0) const;
    virtual std::string toString() const;

    virtual uint16_t len() const;

    AddressContainer getAddresses() const { return addrs_; };
    void setAddresses(const AddressContainer& addrs);
    void setAddress(const IOAddress& addr);
    void addAddress(const IOAddress& addr);

protected:
    AddressContainer addrs_;
};

};
};
