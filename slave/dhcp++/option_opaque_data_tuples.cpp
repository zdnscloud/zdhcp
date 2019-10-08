#include <kea/exceptions/exceptions.h>
#include <kea/dhcp++/opaque_data_tuple.h>
#include <kea/dhcp++/option_opaque_data_tuples.h>
#include <sstream>

namespace kea {
namespace dhcp {

OptionOpaqueDataTuples::OptionOpaqueDataTuples(const uint16_t type)
    : Option(type) {
}

OptionOpaqueDataTuples::OptionOpaqueDataTuples(const uint16_t type,
        OptionBufferConstIter begin, OptionBufferConstIter end)
    : Option(type) {
    unpack(begin, end);
}

std::unique_ptr<Option> OptionOpaqueDataTuples::clone() const {
    return (cloneInternal<OptionOpaqueDataTuples>());
}

void OptionOpaqueDataTuples::pack(kea::util::OutputBuffer& buf) const {
    packHeader(buf);
    for (auto &tuple : tuples_) {
        tuple.pack(buf);
    }
}

void OptionOpaqueDataTuples::unpack(OptionBufferConstIter begin,
        OptionBufferConstIter end) {
    if (std::distance(begin, end) < MinimalLength - getHeaderLen()) {
        kea_throw(OutOfRange, "parsed data tuples option data truncated to"
                  " size " << std::distance(begin, end));
    }

    size_t offset = 0;
    while (offset < std::distance(begin, end)) {
        //todo move
        OpaqueDataTuple tuple(begin + offset, end);
        addTuple(tuple);
        offset += tuple.getTotalLength();
    }
}

void OptionOpaqueDataTuples::addTuple(const OpaqueDataTuple& tuple) {
    tuples_.push_back(tuple);
}


void OptionOpaqueDataTuples::setTuple(const size_t at, const OpaqueDataTuple& tuple) {
    if (at >= getTuplesNum()) {
        kea_throw(kea::OutOfRange, "attempted to set an opaque data for the"
                " opaque data tuple option at position " << at << " which"
                " is out of range");
    }

    tuples_[at] = tuple;
}

OpaqueDataTuple OptionOpaqueDataTuples::getTuple(const size_t at) const {
    if (at >= getTuplesNum()) {
        kea_throw(kea::OutOfRange, "attempted to get an opaque data for the"
                  " opaque data tuple option at position " << at << " which is"
                  " out of range. There are only " << getTuplesNum() << " tuples");
    }
    return (tuples_[at]);
}

bool OptionOpaqueDataTuples::hasTuple(const std::string& tuple_str) const {
    for (auto& tuple : tuples_) {
        if (tuple == tuple_str) {
            return (true);
        }
    }
    return (false);
}

uint16_t OptionOpaqueDataTuples::len() const {
    uint16_t length = getHeaderLen();
    for (auto& tuple : tuples_) {
        length += tuple.getTotalLength();
    }

    return (length);
}

std::string OptionOpaqueDataTuples::toText(int indent) const {
    std::ostringstream s;
    s << std::string(indent, ' ');
    s << "type=" << getType() << ", len=" << len() - getHeaderLen() << std::dec;
    for (unsigned i = 0; i < getTuplesNum(); ++i) {
        auto tuple = getTuple(i);
        s << ", data-len" << i << "=" << tuple.getLength();
        s << ", data" << i << "='" << tuple << "'";
    }

    return (s.str());
}

}; 
};
