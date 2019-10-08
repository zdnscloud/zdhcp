#include <kea/dhcp++/opaque_data_tuple.h>

namespace kea {
namespace dhcp {

OpaqueDataTuple::OpaqueDataTuple() {
}

void OpaqueDataTuple::append(const std::string& text) {
    if (!text.empty()) {
        append(&text[0], text.size());
    }
}

void OpaqueDataTuple::assign(const std::string& text) {
    if (text.empty()) {
        clear();
    } else {
        assign(&text[0], text.size());
    }
}

void OpaqueDataTuple::clear() {
    data_.clear();
}

bool OpaqueDataTuple::equals(const std::string& other) const {
    return (getText() == other);
}

std::string OpaqueDataTuple::getText() const {
    return (std::string(data_.begin(), data_.end()));
}

void OpaqueDataTuple::pack(kea::util::OutputBuffer& buf) const {
    if (getLength() == 0) {
        kea_throw(OpaqueDataTupleError, "failed to create on-wire format of the"
                  " opaque data field, because the field appears to be empty");
    } else if ((1 << (getDataFieldSize() * 8)) <= getLength()) {
        kea_throw(OpaqueDataTupleError, "failed to create on-wire format of the"
                  " opaque data field, because current data length "
                  << getLength() << " exceeds the maximum size for the length"
                  << " field size " << getDataFieldSize());
    }

    if (getDataFieldSize() == 1) {
        buf.writeUint8(static_cast<uint8_t>(getLength()));
    } else {
        buf.writeUint16(getLength());
    }

    buf.writeData(&getData()[0], getLength());
}

int OpaqueDataTuple::getDataFieldSize() const {
    return (1);
}

OpaqueDataTuple&
OpaqueDataTuple::operator=(const std::string& other) {
    assign(&other[0], other.length());
    return (*this);
}

bool OpaqueDataTuple::operator==(const std::string& other) const {
    return (equals(other));
}

bool OpaqueDataTuple::operator!=(const std::string& other) {
    return (!equals(other));
}

std::ostream& operator<<(std::ostream& os, const OpaqueDataTuple& tuple) {
    os << tuple.getText();
    return (os);
}

std::istream& operator>>(std::istream& is, OpaqueDataTuple& tuple) {
    tuple.clear();
    char buf[256];
    while (!is.eof()) {
        is.read(buf, sizeof(buf));
        tuple.append(buf, is.gcount());
    }
    is.clear();
    return (is);
}
};
};
