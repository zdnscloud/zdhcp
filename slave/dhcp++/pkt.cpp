#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/libdhcp++.h>
#include <kea/dhcp++/option_int.h>
#include <kea/dhcp++/pkt.h>
#include <kea/exceptions/exceptions.h>

#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;
using namespace kea::dhcp;

namespace kea {
namespace dhcp {

const IOAddress IPV4_ZERO_ADDRESS("0.0.0.0");

Pkt::Pkt(uint8_t msg_type, uint32_t transid)
    :op_(DHCPTypeToBootpType(msg_type)),
    transid_(transid),
    hops_(0),
    secs_(0),
    flags_(0),
    iface_(""),
    ifindex_(-1),
    buffer_out_(0),
    local_addr_(IPV4_ZERO_ADDRESS),
    remote_addr_(IPV4_ZERO_ADDRESS),
    local_port_(DHCP4_SERVER_PORT),
    remote_port_(DHCP4_CLIENT_PORT),
    copy_retrieved_options_(false),
    ciaddr_(IPV4_ZERO_ADDRESS),
    yiaddr_(IPV4_ZERO_ADDRESS),
    siaddr_(IPV4_ZERO_ADDRESS),
    giaddr_(IPV4_ZERO_ADDRESS)
{
    memset(sname_, 0, MAX_SNAME_LEN);
    memset(file_, 0, MAX_FILE_LEN);
    setType(msg_type);
}

Pkt::Pkt(const uint8_t* data, size_t len)
    :transid_(0),
    hops_(0),
    secs_(0),
    flags_(0),
    iface_(""),
    ifindex_(-1),
    buffer_out_(0),
    op_(BOOTREQUEST),
    local_addr_(IPV4_ZERO_ADDRESS),
    remote_addr_(IPV4_ZERO_ADDRESS),
    local_port_(DHCP4_SERVER_PORT),
    remote_port_(DHCP4_CLIENT_PORT),
    copy_retrieved_options_(false),
    ciaddr_(IPV4_ZERO_ADDRESS),
    yiaddr_(IPV4_ZERO_ADDRESS),
    siaddr_(IPV4_ZERO_ADDRESS),
    giaddr_(IPV4_ZERO_ADDRESS) 
{
    if (len < DHCPV4_PKT_HDR_LEN) {
        kea_throw(OutOfRange, "Truncated DHCPv4 packet (len=" << len
                << ") received, at least " << DHCPV4_PKT_HDR_LEN
                << " is expected.");
    }

    if (data == NULL) {
        kea_throw(InvalidParameter, "data buffer passed to Pkt is NULL");
    }

    data_.resize(len);
    memcpy(&data_[0], data, len);

    memset(sname_, 0, MAX_SNAME_LEN);
    memset(file_, 0, MAX_FILE_LEN);
}

size_t Pkt::len() const {
    size_t length = DHCPV4_PKT_HDR_LEN;
    for (auto& it : options_) {
        length += it.second->len();
    }
    return (length);
}

void Pkt::pack() {
    buffer_out_.clear();
    try {
        size_t hw_len = hwaddr_.hwaddr_.size();
        buffer_out_.writeUint8(op_);
        buffer_out_.writeUint8(hwaddr_.htype_);
        buffer_out_.writeUint8(hw_len < MAX_CHADDR_LEN ? hw_len : MAX_CHADDR_LEN);
        buffer_out_.writeUint8(hops_);
        buffer_out_.writeUint32(transid_);
        buffer_out_.writeUint16(secs_);
        buffer_out_.writeUint16(flags_);
        buffer_out_.writeUint32(ciaddr_);
        buffer_out_.writeUint32(yiaddr_);
        buffer_out_.writeUint32(siaddr_);
        buffer_out_.writeUint32(giaddr_);

        if ((hw_len > 0) && (hw_len <= MAX_CHADDR_LEN)) {
            // write up to 16 bytes of the hardware address (CHADDR field is 16
            // bytes long in DHCPv4 message).
            buffer_out_.writeData(&hwaddr_.hwaddr_[0],
                    (hw_len < MAX_CHADDR_LEN ? hw_len : MAX_CHADDR_LEN) );
            hw_len = MAX_CHADDR_LEN - hw_len;
        } else {
            hw_len = MAX_CHADDR_LEN;
        }

        if (hw_len > 0) {
            vector<uint8_t> zeros(hw_len, 0);
            buffer_out_.writeData(&zeros[0], hw_len);
        }

        buffer_out_.writeData(sname_, MAX_SNAME_LEN);
        buffer_out_.writeData(file_, MAX_FILE_LEN);
        buffer_out_.writeUint32(DHCP_OPTIONS_COOKIE);

        LibDHCP::packOptions4(buffer_out_, options_);
        buffer_out_.writeUint8(DHO_END);
     } catch(const Exception& e) {
         kea_throw(InvalidOperation, e.what());
    }
}

void Pkt::unpack() {
    util::InputBuffer buffer_in(&data_[0], data_.size());
    if (buffer_in.getLength() < DHCPV4_PKT_HDR_LEN) {
        kea_throw(OutOfRange, "Received truncated DHCPv4 packet (len="
                << buffer_in.getLength() << " received, at least "
                << DHCPV4_PKT_HDR_LEN << "is expected");
    }

    op_ = buffer_in.readUint8();
    uint8_t htype = buffer_in.readUint8();
    uint8_t hlen = buffer_in.readUint8();
    hops_ = buffer_in.readUint8();
    transid_ = buffer_in.readUint32();
    secs_ = buffer_in.readUint16();
    flags_ = buffer_in.readUint16();
    ciaddr_ = IOAddress(buffer_in.readUint32());
    yiaddr_ = IOAddress(buffer_in.readUint32());
    siaddr_ = IOAddress(buffer_in.readUint32());
    giaddr_ = IOAddress(buffer_in.readUint32());

    vector<uint8_t> hw_addr(MAX_CHADDR_LEN, 0);
    buffer_in.readVector(hw_addr, MAX_CHADDR_LEN);
    buffer_in.readData(sname_, MAX_SNAME_LEN);
    buffer_in.readData(file_, MAX_FILE_LEN);

    hw_addr.resize(hlen);
    hwaddr_ = HWAddr(hw_addr, htype);

    if (buffer_in.getLength() == buffer_in.getPosition()) {
        // this is *NOT* DHCP packet. It does not have any DHCPv4 options. In
        // particular, it does not have magic cookie, a 4 byte sequence that
        // differentiates between DHCP and BOOTP packets.
        kea_throw(InvalidOperation, "Received BOOTP packet. BOOTP is not supported.");
    }

    if (buffer_in.getLength() - buffer_in.getPosition() < 4) {
      // there is not enough data to hold magic DHCP cookie
      kea_throw(Unexpected, "Truncated or no DHCP packet.");
    }

    uint32_t magic = buffer_in.readUint32();
    if (magic != DHCP_OPTIONS_COOKIE) {
      kea_throw(Unexpected, "Invalid or missing DHCP magic cookie");
    }

    size_t opts_len = buffer_in.getLength() - buffer_in.getPosition();
    vector<uint8_t> opts_buffer;

    // Use readVector because a function which parses option requires
    // a vector as an input.
    buffer_in.readVector(opts_buffer, opts_len);
    size_t offset = LibDHCP::unpackOptions4(opts_buffer, "dhcp4", options_);

    // If offset is not equal to the size and there is no DHO_END,
    // then something is wrong here. We either parsed past input
    // buffer (bug in our code) or we haven't parsed everything
    // (received trailing garbage or truncated option).
    //
    // Invoking Jon Postel's law here: be conservative in what you send, and be
    // liberal in what you accept. There's no easy way to log something from
    // libdhcp++ library, so we just choose to be silent about remaining
    // bytes. We also need to quell compiler warning about unused offset
    // variable.
    //
    // if ((offset != size) && (opts_buffer[offset] != DHO_END)) {
    //        kea_throw(BadValue, "Received DHCPv6 buffer of size " << size
    //                  << ", were able to parse " << offset << " bytes.");
    // }
    (void)offset;

    // No need to call check() here. There are thorough tests for this
    // later (see Dhcp4Srv::accept()). We want to drop the packet later,
    // so we'll be able to log more detailed drop reason.
}

bool Pkt::delOption(uint16_t type) {
    kea::dhcp::OptionCollection::iterator x = options_.find(type);
    if (x!=options_.end()) {
        options_.erase(x);
        return (true); // delete successful
    } else {
        return (false); // can't find option to be deleted
    }
}

const Option* Pkt::getOption(uint16_t type) const {
    auto x = options_.find(type);
    if (x != options_.end()) {
        return (x->second.get());
    } else {
        return nullptr;
    }
}

void Pkt::addClass(const std::string& client_class) {
    if (classes_.find(client_class) == classes_.end()) {
        classes_.insert(client_class);
    }
}

uint8_t Pkt::getType() const {
    const Option* generic = getOption(DHO_DHCP_MESSAGE_TYPE);
    if (generic == nullptr) {
        return (DHCP_NOTYPE);
    }

    const OptionInt<uint8_t>* type_opt = dynamic_cast<const OptionInt<uint8_t>*>(generic);
    if (type_opt != nullptr) {
        return (type_opt->getValue());
    } else {
        return (generic->getUint8());
    }
}

void Pkt::setType(uint8_t dhcp_type) {
    const Option* opt = getOption(DHO_DHCP_MESSAGE_TYPE);
    if (opt != nullptr) {
        const OptionInt<uint8_t> * type_opt = dynamic_cast<const OptionInt<uint8_t>*>(opt);
        if (type_opt != nullptr) {
            (const_cast<OptionInt<uint8_t> *>(type_opt))->setValue(dhcp_type);
        } else {
            (const_cast<Option*>(opt))->setUint8(dhcp_type);
        }
    } else {
        addOption(std::unique_ptr<Option>(new OptionInt<uint8_t>(DHO_DHCP_MESSAGE_TYPE,
                        dhcp_type)));
    }
}

const char* Pkt::getName(const uint8_t type) {
    static const char* DHCPDISCOVER_NAME = "DHCPDISCOVER";
    static const char* DHCPOFFER_NAME = "DHCPOFFER";
    static const char* DHCPREQUEST_NAME = "DHCPREQUEST";
    static const char* DHCPDECLINE_NAME = "DHCPDECLINE";
    static const char* DHCPACK_NAME = "DHCPACK";
    static const char* DHCPNAK_NAME = "DHCPNAK";
    static const char* DHCPRELEASE_NAME = "DHCPRELEASE";
    static const char* DHCPINFORM_NAME = "DHCPINFORM";
    static const char* UNKNOWN_NAME = "UNKNOWN";

    switch (type) {
        case DHCPDISCOVER:
            return (DHCPDISCOVER_NAME);
        case DHCPOFFER:
            return (DHCPOFFER_NAME);
        case DHCPREQUEST:
            return (DHCPREQUEST_NAME);
        case DHCPDECLINE:
            return (DHCPDECLINE_NAME);
        case DHCPACK:
            return (DHCPACK_NAME);
        case DHCPNAK:
            return (DHCPNAK_NAME);
        case DHCPRELEASE:
            return (DHCPRELEASE_NAME);
        case DHCPINFORM:
            return (DHCPINFORM_NAME);
        default:
            ;
    }
    return (UNKNOWN_NAME);
}

const char* Pkt::getName() const {
    // getType() is now exception safe. Even if there's no option 53 (message
    // type), it now returns 0 rather than throw. getName() is able to handle
    // 0 and unknown message types.
    return (Pkt::getName(getType()));
}

std::string Pkt::getLabel() const {
    /// @todo If and when client id is extracted into Pkt, this method should
    /// use the instance member rather than fetch it every time.
    std::string suffix;
    const Option* client_opt = getOption(DHO_DHCP_CLIENT_IDENTIFIER);
    if (client_opt != nullptr) {
        unique_ptr<ClientId> client_id(new ClientId(client_opt->getData()));
        return makeLabel(&hwaddr_, client_id.get(), transid_);
    } else {
        return makeLabel(&hwaddr_, nullptr, transid_);
    }
}

std::string Pkt::makeLabel(const HWAddr* hwaddr, const ClientId* client_id,
        uint32_t transid) {
    stringstream label;
    label << makeLabel(hwaddr, client_id);
    label << ", tid=0x" << hex << transid << dec;
    return label.str();
}

std::string Pkt::makeLabel(const HWAddr* hwaddr, const ClientId* client_id) {
    stringstream label;
    label << "[" << (hwaddr ? hwaddr->toText() : "no hwaddr info")
          << "], cid=[" << (client_id ? client_id->toText() : "no info")
          << "]";
    return label.str();
}

std::string Pkt::toText() const {
    stringstream output;
    output << "local_address=" << local_addr_.toText() << ":" << local_port_
        << ", remote_adress=" << remote_addr_.toText()
        << ", client_adress=" << ciaddr_.toText()
        << ":" << remote_port_ << ", msg_type=";

    uint8_t msg_type = getType();
    if (msg_type != DHCP_NOTYPE) {
        output << getName(msg_type) << " (" << static_cast<int>(msg_type) << ")";
    } else {
        output << "(missing)";
    }

    output << ", " << hwaddr_.toText();
    if (!yiaddr_.isV4Zero()) {
        output << ", yiaddr=" << yiaddr_.toText();
    }

    output << ", transid=0x" << hex << transid_ << dec;

    if (!options_.empty()) {
        output << "," << std::endl << "options:";
        for (auto& i : options_) {
            try {
                output << std::endl << i.second->toText(2);
            } catch (...) {
                output << "(unknown)" << std::endl;
            }
        }
    } else {
        output << ", message contains no options";
    }

    return (output.str());
}

void Pkt::setHWAddr(uint8_t htype, uint8_t hlen,
        const std::vector<uint8_t>& mac_addr) {
    if (hlen > MAX_CHADDR_LEN) {
        kea_throw(OutOfRange, "Hardware address (len=" << hlen
                  << " too long. Max " << MAX_CHADDR_LEN << " supported.");

    } else if (mac_addr.empty() && (hlen > 0) ) {
        kea_throw(OutOfRange, "Invalid HW Address specified");
    }
    hwaddr_ = HWAddr(mac_addr, htype);
}

void Pkt::setLocalHWAddr(const uint8_t htype, const uint8_t hlen,
        const std::vector<uint8_t>& mac_addr) {
    if (hlen > MAX_CHADDR_LEN) {
        kea_throw(OutOfRange, "Hardware address (len=" << hlen
                << " too long. Max " << MAX_CHADDR_LEN << " supported.");

    } else if (mac_addr.empty() && (hlen > 0) ) {
        kea_throw(OutOfRange, "Invalid HW Address specified");
    }
    local_hwaddr_ = HWAddr(mac_addr, htype);
}

void Pkt::setSname(const uint8_t* sname, size_t snameLen /*= MAX_SNAME_LEN*/) {
    if (snameLen > MAX_SNAME_LEN) {
        kea_throw(OutOfRange, "sname field (len=" << snameLen
                << ") too long, Max " << MAX_SNAME_LEN << " supported.");

    } else if (sname == NULL) {
        kea_throw(InvalidParameter, "Invalid sname specified");
    }

    std::copy(sname, (sname + snameLen), sname_);
    if (snameLen < MAX_SNAME_LEN) {
        std::fill((sname_ + snameLen), (sname_ + MAX_SNAME_LEN), 0);
    }
}

void Pkt::setFile(const uint8_t* file, size_t fileLen /*= MAX_FILE_LEN*/) {
    if (fileLen > MAX_FILE_LEN) {
        kea_throw(OutOfRange, "file field (len=" << fileLen
                << ") too long, Max " << MAX_FILE_LEN << " supported.");

    } else if (file == NULL) {
        kea_throw(InvalidParameter, "Invalid file name specified");
    }

    std::copy(file, (file + fileLen), file_);
    if (fileLen < MAX_FILE_LEN) {
        std::fill((file_ + fileLen), (file_ + MAX_FILE_LEN), 0);
    }
}

uint8_t Pkt::DHCPTypeToBootpType(uint8_t dhcpType) {
    switch (dhcpType) {
        case DHCPDISCOVER:
        case DHCPREQUEST:
        case DHCPDECLINE:
        case DHCPRELEASE:
        case DHCPINFORM:
        case DHCPLEASEQUERY:
        case DHCPBULKLEASEQUERY:
        case DHCPCONFLICTIP:
            return (BOOTREQUEST);

        case DHCPACK:
        case DHCPNAK:
        case DHCPOFFER:
        case DHCPLEASEUNASSIGNED:
        case DHCPLEASEUNKNOWN:
        case DHCPLEASEACTIVE:
        case DHCPLEASEQUERYDONE:
            return (BOOTREPLY);

        default:
            kea_throw(OutOfRange, "Invalid message type: "
                    << static_cast<int>(dhcpType) );
    }
}

void Pkt::addOption(std::unique_ptr<Option> opt) {
    if (getOption(opt->getType()) != nullptr) {
        kea_throw(BadValue, "Option " << opt->getType()
                  << " already present in this message.");
    }
    options_.insert(std::make_pair(opt->getType(), std::move(opt)));
}

bool Pkt::isRelayed() const {
    return (!giaddr_.isV4Zero() && !giaddr_.isV4Bcast());
}

}; 
};
