#pragma once

#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/opaque_data_tuple.h>
#include <kea/dhcp++/option.h>
#include <kea/util/buffer.h>
#include <cstdint>

namespace kea {
namespace dhcp {

class OptionOpaqueDataTuples : public Option {
public:

    enum {MinimalLength=4};
    typedef std::vector<OpaqueDataTuple> TuplesCollection;

    OptionOpaqueDataTuples(const uint16_t type);

    OptionOpaqueDataTuples(const uint16_t type, OptionBufferConstIter begin,
            OptionBufferConstIter end);

    std::unique_ptr<Option> clone() const;

    virtual void pack(kea::util::OutputBuffer& buf) const;

    virtual void unpack(OptionBufferConstIter begin, OptionBufferConstIter end);

    void addTuple(const OpaqueDataTuple& tuple);

    void setTuple(const size_t at, const OpaqueDataTuple& tuple);

    OpaqueDataTuple getTuple(const size_t at) const;

    size_t getTuplesNum() const {
        return (tuples_.size());
    }

    const TuplesCollection& getTuples() const {
        return (tuples_);
    }

    bool hasTuple(const std::string& tuple_str) const;

    virtual uint16_t len() const;

    virtual std::string toText(int indent = 0) const;

private:
    TuplesCollection tuples_;

};

typedef std::shared_ptr<OptionOpaqueDataTuples> OptionOpaqueDataTuplesPtr;

};
};
