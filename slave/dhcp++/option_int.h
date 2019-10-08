#pragma once

#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_data_types.h>
#include <kea/util/io_utilities.h>

#include <cstdint>
#include <sstream>

namespace kea {
namespace dhcp {


template<typename T>
class OptionInt: public Option {
private:

    typedef std::shared_ptr<OptionInt<T> > OptionIntTypePtr;

public:
    OptionInt(uint16_t type, T value)
        : Option(type), value_(value) {
        if (OptionDataTypeTraits<T>::type == OPT_UNKNOWN_TYPE) {
            kea_throw(dhcp::InvalidDataType, "unsupported or non-integer type");
        }
        setEncapsulatedSpace("dhcp4");
    }

    OptionInt(uint16_t type, OptionBufferConstIter begin,
               OptionBufferConstIter end)
        : Option(type) {
        if (OptionDataTypeTraits<T>::type == OPT_UNKNOWN_TYPE) {
            kea_throw(dhcp::InvalidDataType, "non-integer type");
        }
        setEncapsulatedSpace("dhcp4");
        unpack(begin, end);
    }

    virtual std::unique_ptr<Option> clone() const {
        return (cloneInternal<OptionInt<T>>());
    }

    void pack(kea::util::OutputBuffer& buf) const {
        packHeader(buf);
        int data_len = sizeof(T);
        switch (data_len) {
        case 1:
            buf.writeUint8(value_);
            break;
        case 2:
            buf.writeUint16(value_);
            break;
        case 4:
            buf.writeUint32(value_);
            break;
        default:
            kea_throw(dhcp::InvalidDataType, "non-integer type");
        }
        packOptions(buf);
    }

    virtual void unpack(OptionBufferConstIter begin, OptionBufferConstIter end) {
        if (distance(begin, end) < sizeof(T)) {
            kea_throw(OutOfRange, "Option " << getType() << " truncated");
        }
        int data_size_len = sizeof(T);
        switch (data_size_len) {
        case 1:
            value_ = *begin;
            break;
        case 2:
            value_ = kea::util::readUint16(&(*begin),
                                           std::distance(begin, end));
            break;
        case 4:
            value_ = kea::util::readUint32(&(*begin),
                                           std::distance(begin, end));
            break;
        default:
            kea_throw(dhcp::InvalidDataType, "non-integer type");
        }

        begin += data_size_len;
        unpackOptions(OptionBuffer(begin, end));
    }

    void setValue(T value) { value_ = value; }

    T getValue() const { return value_; }

    virtual uint16_t len() const {
        uint16_t length =  OPTION4_HDR_LEN;
        length += sizeof(T);;
        for (auto &pair : options_) {
            length += pair.second->len();
        }
        return (length);
    }

    virtual std::string toText(int indent = 0) const {
        std::stringstream output;
        output << headerToText(indent) << ": ";

        if (sizeof(T) == 1) {
            output << static_cast<int>(getValue());
        } else {
            output << getValue();
        }

        output << " ("
               << OptionDataTypeUtil::getDataTypeName(OptionDataTypeTraits<T>::type)
               << ")";

        output << suboptionsToText(indent + 2);

        return (output.str());
    }

private:

    T value_;  
};

typedef OptionInt<std::uint8_t> OptionUint8;
typedef OptionInt<std::uint16_t> OptionUint16;
typedef OptionInt<std::uint32_t> OptionUint32;

};
};
