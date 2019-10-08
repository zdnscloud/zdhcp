#pragma once

#include <kea/util/buffer.h>
#include <kea/util/io_utilities.h>

#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace kea {
namespace dhcp {

class OpaqueDataTupleError : public Exception {
public:
    OpaqueDataTupleError(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};


class OpaqueDataTuple {
public:
    typedef std::vector<uint8_t> Buffer;

    OpaqueDataTuple();

    template<typename InputIterator>
    OpaqueDataTuple(InputIterator begin, InputIterator end) {
        unpack(begin, end);
    }

    template<typename InputIterator>
    void append(InputIterator data, const size_t len) {
        data_.insert(data_.end(), data, data + len);
    }

    void append(const std::string& text);

    template<typename InputIterator>
    void assign(InputIterator data, const size_t len) {
        data_.assign(data, data + len);
    }

    void assign(const std::string& text);

    void clear();

    bool equals(const std::string& other) const;

    size_t getLength() const {
        return (data_.size());
    }

    size_t getTotalLength() const {
        return (getDataFieldSize() + getLength());
    }

    const Buffer& getData() const {
        return (data_);
    }

    std::string getText() const;

    void pack(kea::util::OutputBuffer& buf) const;

    template<typename InputIterator>
    void unpack(InputIterator begin, InputIterator end) {
        Buffer buf(begin, end);
        if (std::distance(begin, end) < getDataFieldSize()) {
            kea_throw(OpaqueDataTupleError,
                      "unable to parse the opaque data tuple, the buffer"
                      " length is " << std::distance(begin, end)
                      << ", expected at least " << getDataFieldSize());
        }
        size_t len = getDataFieldSize() == 1 ? *begin :
            kea::util::readUint16(&(*begin), std::distance(begin, end));

        begin += getDataFieldSize();
        if (std::distance(begin, end) < len) {
            kea_throw(OpaqueDataTupleError,
                      "unable to parse the opaque data tuple, the buffer"
                      " length is " << std::distance(begin, end)
                      << ", but the length of the tuple in the length field"
                      " is " << len);
        }
        assign(begin, len);
    }

    OpaqueDataTuple& operator=(const std::string& other);
    bool operator==(const std::string& other) const;
    bool operator!=(const std::string& other);
    int getDataFieldSize() const;

private:
    Buffer data_;
};

typedef std::shared_ptr<OpaqueDataTuple> OpaqueDataTuplePtr;
std::ostream& operator<<(std::ostream& os, const OpaqueDataTuple& tuple);
std::istream& operator>>(std::istream& is, OpaqueDataTuple& tuple);

}; 
};
