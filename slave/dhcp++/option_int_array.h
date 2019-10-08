#pragma once

#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_data_types.h>
#include <kea/util/io_utilities.h>

#include <sstream>
#include <cstdint>

namespace kea {
namespace dhcp {

template<typename T>
class OptionIntArray: public Option {
public:
    OptionIntArray(const uint16_t type)
        : Option(type),
          values_(0) {
        if (OptionDataTypeTraits<T>::type == OPT_UNKNOWN_TYPE) {
            kea_throw(dhcp::InvalidDataType, "unsupported or non-integer type");
        }
    }

    OptionIntArray(const uint16_t type,
                   const OptionBuffer& buf)
        : Option(type) {
        if (OptionDataTypeTraits<T>::type == OPT_UNKNOWN_TYPE) {
            kea_throw(dhcp::InvalidDataType, "non-integer type");
        }
        unpack(buf.begin(), buf.end());
    }

    OptionIntArray(const uint16_t type,
                   OptionBufferConstIter begin, OptionBufferConstIter end)
        : Option(type) {
        if (OptionDataTypeTraits<T>::type == OPT_UNKNOWN_TYPE) {
            kea_throw(dhcp::InvalidDataType, "non-integer type");
        }
        unpack(begin, end);
    }

    virtual std::unique_ptr<Option> clone() const {
        return (cloneInternal<OptionIntArray<T> >());
    }

    void addValue(const T value) {
        values_.push_back(value);
    }

    void pack(kea::util::OutputBuffer& buf) const {
        size_t data_size_len = sizeof(T);
        packHeader(buf);
        for (size_t i = 0; i < values_.size(); ++i) {
            switch (data_size_len) {
            case 1:
                buf.writeUint8(values_[i]);
                break;
            case 2:
                buf.writeUint16(values_[i]);
                break;
            case 4:
                buf.writeUint32(values_[i]);
                break;
            default:
                kea_throw(dhcp::InvalidDataType, "non-integer type");
            }
        }
    }

    virtual void unpack(OptionBufferConstIter begin, OptionBufferConstIter end) {
        if (distance(begin, end) == 0) {
            kea_throw(OutOfRange, "option " << getType() << " empty");
        }
        if (distance(begin, end) % sizeof(T) != 0) {
            kea_throw(OutOfRange, "option " << getType() << " truncated");
        }

        values_.clear();
        size_t data_size_len = sizeof(T);
        while (begin != end) {
            switch (data_size_len) {
            case 1:
                values_.push_back(*begin);
                break;
            case 2:
                values_.push_back(kea::util::readUint16(&(*begin),
                                      std::distance(begin, end)));
                break;
            case 4:
                values_.push_back(kea::util::readUint32(&(*begin),
                                      std::distance(begin, end)));
                break;
            default:
                kea_throw(dhcp::InvalidDataType, "non-integer type");
            }
            begin += data_size_len;
        }
    }

    const std::vector<T>& getValues() const { return (values_); }

    void setValues(const std::vector<T>& values) { values_ = values; }

    virtual uint16_t len() const {
        uint16_t length =  OPTION4_HDR_LEN;
        length += values_.size() * sizeof(T);
        for (auto& pair : options_) {
            length += pair.second->len();
        }
        return (length);
    }
    
    virtual std::string toString() const
    {
        std::stringstream output;
        for (typename std::vector<T>::const_iterator value = values_.begin();
             value != values_.end(); ++value) {
            if (value != values_.begin())
                output << ",";

            if (sizeof(T) == 1) {
                output << static_cast<int>(*value);
            } else {
                output << *value;
            }
        }

        return (output.str());
    }

    virtual std::string toText(int indent = 0) const {
        std::stringstream output;
        output << headerToText(indent) << ":";

        std::string data_type = OptionDataTypeUtil::getDataTypeName(OptionDataTypeTraits<T>::type);
        for (typename std::vector<T>::const_iterator value = values_.begin();
             value != values_.end(); ++value) {
            output << " ";

            if (sizeof(T) == 1) {
                output << static_cast<int>(*value);
            } else {
                output << *value;
            }

            output << "(" << data_type << ")";
        }

        return (output.str());
    }

private:

    std::vector<T> values_;
};

typedef OptionIntArray<uint8_t> OptionUint8Array;
typedef OptionIntArray<uint16_t> OptionUint16Array;
typedef OptionIntArray<uint32_t> OptionUint32Array;

}; 
}; 
