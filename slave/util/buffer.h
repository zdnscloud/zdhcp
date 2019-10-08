#pragma once

#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cassert>
#include <memory>

#include <kea/exceptions/exceptions.h>

using namespace std;

namespace kea {
namespace util {

class InvalidBufferPosition : public kea::Exception {
public:
    InvalidBufferPosition(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class InputBuffer {
public:
    InputBuffer(const void* data, size_t len) :
        position_(0), data_(static_cast<const uint8_t*>(data)), len_(len) {}

    size_t getLength() const { return (len_); }

    size_t getPosition() const { return (position_); }

    void setPosition(size_t position) {
        if (position > len_) {
            throwError("position is too large");
        }
        position_ = position;
    }

    uint8_t readUint8() {
        if (position_ + sizeof(uint8_t) > len_) {
            throwError("read beyond end of buffer");
        }

        return (data_[position_++]);
    }

    uint16_t readUint16() {
        uint16_t data;
        const uint8_t* cp;

        if (position_ + sizeof(data) > len_) {
            throwError("read beyond end of buffer");
        }

        cp = &data_[position_];
        data = ((unsigned int)(cp[0])) << 8;
        data |= ((unsigned int)(cp[1]));
        position_ += sizeof(data);

        return (data);
    }

    uint32_t readUint32() {
        uint32_t data;
        const uint8_t* cp;

        if (position_ + sizeof(data) > len_) {
            throwError("read beyond end of buffer");
        }

        cp = &data_[position_];
        data = ((unsigned int)(cp[0])) << 24;
        data |= ((unsigned int)(cp[1])) << 16;
        data |= ((unsigned int)(cp[2])) << 8;
        data |= ((unsigned int)(cp[3]));
        position_ += sizeof(data);

        return (data);
    }

    void readData(void* data, size_t len) {
        if (position_ + len > len_) {
            throwError("read beyond end of buffer");
        }

        static_cast<void>(std::memmove(data, &data_[position_], len));
        position_ += len;
    }

    void readVector(std::vector<uint8_t>& data, size_t len) {
        if (position_ + len > len_) {
            throwError("read beyond end of buffer");
        }

        data.resize(len);
        readData(&data[0], len);
    }

private:
    static void throwError(const char* msg) {
        kea_throw(InvalidBufferPosition, msg);
    }

    size_t position_;
    const uint8_t* data_;
    size_t len_;
};

class OutputBuffer {
public:
    OutputBuffer(size_t len) :
        buffer_(NULL),
        size_(0),
        allocated_(len)
    {
        if (allocated_ != 0) {
            buffer_ = static_cast<uint8_t*>(malloc(allocated_));
            if (buffer_ == NULL) {
                throw std::bad_alloc();
            }
        }
    }

    OutputBuffer(const OutputBuffer& other) :
        buffer_(NULL),
        size_(other.size_),
        allocated_(other.allocated_)
    {
        if (allocated_ != 0) {
            buffer_ = static_cast<uint8_t*>(malloc(allocated_));
            if (buffer_ == NULL) {
                throw std::bad_alloc();
            }
            static_cast<void>(std::memmove(buffer_, other.buffer_, other.size_));
        }
    }

    ~ OutputBuffer() { free(buffer_); }

    OutputBuffer& operator =(const OutputBuffer& other) {
        if (this != &other) {
            if (other.allocated_ != 0) {

                uint8_t* newbuff = static_cast<uint8_t*>(malloc(other.allocated_));
                if (newbuff == NULL) {
                    throw std::bad_alloc();
                }

                free(buffer_);
                buffer_ = newbuff;
                static_cast<void>(std::memmove(buffer_, other.buffer_, other.size_));

            } else {
                free(buffer_);
                buffer_ = NULL;
            }

            size_ = other.size_;
            allocated_ = other.allocated_;
        }
        return (*this);
    }

    size_t getCapacity() const { return (allocated_); }

    const void* getData() const { return (buffer_); }

    size_t getLength() const { return (size_); }

    uint8_t operator[](size_t pos) const {
        assert (pos < size_);
        return (buffer_[pos]);
    }

    void skip(size_t len) {
        ensureAllocated(size_ + len);
        size_ += len;
    }

    void trim(size_t len) {
        if (len > size_) {
            kea_throw(OutOfRange, "trimming too large from output buffer");
        }
        size_ -= len;
    }

    void clear() { size_ = 0; }

    void wipe() {
        if (buffer_ != NULL) {
            static_cast<void>(std::memset(buffer_, 0, allocated_));
        }
        size_ = 0;
    }

    void writeUint8(uint8_t data) {
        ensureAllocated(size_ + 1);
        buffer_[size_ ++] = data;
    }

    void writeUint8At(uint8_t data, size_t pos) {
        if (pos + sizeof(data) > size_) {
            kea_throw(InvalidBufferPosition, "write at invalid position");
        }
        buffer_[pos] = data;
    }

    void writeUint16(uint16_t data) {
        ensureAllocated(size_ + sizeof(data));
        buffer_[size_ ++] = static_cast<uint8_t>((data & 0xff00U) >> 8);
        buffer_[size_ ++] = static_cast<uint8_t>(data & 0x00ffU);
    }

    void writeUint16At(uint16_t data, size_t pos) {
        if (pos + sizeof(data) > size_) {
            kea_throw(InvalidBufferPosition, "write at invalid position");
        }

        buffer_[pos] = static_cast<uint8_t>((data & 0xff00U) >> 8);
        buffer_[pos + 1] = static_cast<uint8_t>(data & 0x00ffU);
    }

    void writeUint32(uint32_t data) {
        ensureAllocated(size_ + sizeof(data));
        buffer_[size_ ++] = static_cast<uint8_t>((data & 0xff000000) >> 24);
        buffer_[size_ ++] = static_cast<uint8_t>((data & 0x00ff0000) >> 16);
        buffer_[size_ ++] = static_cast<uint8_t>((data & 0x0000ff00) >> 8);
        buffer_[size_ ++] = static_cast<uint8_t>(data & 0x000000ff);
    }

    void writeData(const void *data, size_t len) {
        ensureAllocated(size_ + len);
        static_cast<void>(std::memmove(buffer_ + size_, data, len));
        size_ += len;
    }

private:
    uint8_t* buffer_;
    size_t size_;
    size_t allocated_;

    void ensureAllocated(size_t needed_size) {
        if (allocated_ < needed_size) {
            size_t new_size = (allocated_ == 0) ? 1024 : allocated_;
            while (new_size < needed_size) {
                new_size *= 2;
            }
            uint8_t* new_buffer_(static_cast<uint8_t*>(realloc(buffer_,
                new_size)));
            if (new_buffer_ == NULL) {
                throw std::bad_alloc();
            }
            buffer_ = new_buffer_;
            allocated_ = new_size;
        }
    }
};

typedef std::shared_ptr<InputBuffer> InputBufferPtr;
typedef std::shared_ptr<OutputBuffer> OutputBufferPtr;

}; 
};
