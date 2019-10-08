#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/option4_client_fqdn.h>
#include <kea/dns/labelsequence.h>
#include <kea/util/buffer.h>
#include <kea/util/io_utilities.h>
#include <kea/util/strutil.h>
#include <sstream>

namespace kea {
namespace dhcp {

Option4ClientFqdn::Option4ClientFqdn(const uint8_t flag, 
                                     const Rcode& rcode, 
                                     const std::string& domain_name,
                                     const DomainNameType domain_name_type)
    : Option(DHO_FQDN),
      flags_(flag),
      rcode1_(rcode),
      rcode2_(rcode),
      domain_name_type_(domain_name_type) {
    checkFlags(flags_, true);
    setDomainName(domain_name, domain_name_type_);
}

Option4ClientFqdn::Option4ClientFqdn(const uint8_t flag, const Rcode& rcode)
    : Option(DHO_FQDN),
      flags_(flag),
      rcode1_(rcode),
      rcode2_(rcode),
      domain_name_type_(PARTIAL) {
    checkFlags(flags_, true);
    setDomainName("", domain_name_type_);
}

Option4ClientFqdn::Option4ClientFqdn(OptionBufferConstIter first,
                                     OptionBufferConstIter last)
    : Option(DHO_FQDN, first, last),
      rcode1_(Option4ClientFqdn::RCODE_CLIENT()),
      rcode2_(Option4ClientFqdn::RCODE_CLIENT()) {
    parseWireData(first, last);
    checkFlags(flags_, false);
}

Option4ClientFqdn::~Option4ClientFqdn() {
}

Option4ClientFqdn::Option4ClientFqdn(const Option4ClientFqdn& source)
    : Option(source),
      flags_(source.flags_),
      rcode1_(source.rcode1_),
      rcode2_(source.rcode2_),
      domain_name_(),
      domain_name_type_(source.domain_name_type_) {
    if (source.domain_name_) {
        domain_name_.reset(new kea::dns::Name(*source.domain_name_));
    }
}

Option4ClientFqdn& Option4ClientFqdn::operator=(const Option4ClientFqdn& source) {
    Option::operator=(source);
    if (source.domain_name_) {
        domain_name_.reset(new kea::dns::Name(*source.domain_name_));
    } else {
        domain_name_.reset();
    }

    flags_ = source.flags_;
    rcode1_ = source.rcode1_;
    rcode2_ = source.rcode2_;
    domain_name_type_ = source.domain_name_type_;

    return (*this);
}

void Option4ClientFqdn:: setDomainName(const std::string& domain_name, 
                                  const DomainNameType name_type) {
    std::string name = kea::util::str::trim(domain_name);
    if (name.empty()) {
        if (name_type == FULL) {
            kea_throw(InvalidOption4FqdnDomainName,
                      "fully qualified domain-name must not be empty"
                      << " when setting new domain-name for DHCPv4 Client"
                      << " FQDN Option");
        }
        domain_name_.reset();
    } else {
        try {
            domain_name_.reset(new kea::dns::Name(name));
        } catch (const Exception&) {
            kea_throw(InvalidOption4FqdnDomainName,
                      "invalid domain-name value '"
                      << domain_name << "' when setting new domain-name for"
                      << " DHCPv4 Client FQDN Option");

        }
    }

    domain_name_type_ = name_type;
}

void Option4ClientFqdn::checkFlags(const uint8_t flags, const bool check_mbz) {
    if (check_mbz && ((flags & ~FLAG_MASK) != 0)) {
        kea_throw(InvalidOption4FqdnFlags,
                "invalid DHCPv4 Client FQDN Option flags: 0x"
                << std::hex << static_cast<int>(flags) << std::dec);
    }

    // According to RFC 4702, section 2.1. if the N bit is 1, the S bit
    // MUST be 0. Checking it here.
    if ((flags & (FLAG_N | FLAG_S)) == (FLAG_N | FLAG_S)) {
        kea_throw(InvalidOption4FqdnFlags,
                "both N and S flag of the DHCPv4 Client FQDN Option are set."
                << " According to RFC 4702, if the N bit is 1 the S bit"
                << " MUST be 0");
    }
}

void Option4ClientFqdn::parseWireData(OptionBufferConstIter first,
        OptionBufferConstIter last) {

    if (std::distance(first, last) < FIXED_FIELDS_LEN) {
        kea_throw(OutOfRange, "DHCPv4 Client FQDN Option ("
                  << DHO_FQDN << ") is truncated");
    }

    flags_ = *(first++);
    rcode1_ = Rcode(*(first++));
    rcode2_ = Rcode(*(first++));

    try {
        if ((flags_ & FLAG_E) != 0) {
            parseCanonicalDomainName(first, last);
        } else {
            parseASCIIDomainName(first, last);
        }
    } catch (const Exception& ex) {
        kea_throw(InvalidOption4FqdnDomainName,
                "failed to parse the domain-name in DHCPv4 Client FQDN"
                << " Option: " << ex.what());
    }
}

void Option4ClientFqdn::parseCanonicalDomainName(OptionBufferConstIter first,
        OptionBufferConstIter last) {
    if (std::distance(first, last) > 0) {
        if (*(last - 1) != 0) {
            OptionBuffer buf(first, last);
            buf.push_back(0);
            kea::util::InputBuffer name_buf(&buf[0], buf.size());
            domain_name_.reset(new kea::dns::Name(name_buf));
            domain_name_type_ = PARTIAL;
        } else {
            kea::util::InputBuffer name_buf(&(*first),
                                            std::distance(first, last));
            domain_name_.reset(new kea::dns::Name(name_buf));
            domain_name_type_ = Option4ClientFqdn::FULL;
        }
    }
}

void Option4ClientFqdn::parseASCIIDomainName(OptionBufferConstIter first,
        OptionBufferConstIter last) {
    if (std::distance(first, last) > 0) {
        std::string domain_name(first, last);
        domain_name_.reset(new kea::dns::Name(domain_name));
        domain_name_type_ = domain_name[domain_name.length() - 1] == '.' ?
            FULL : PARTIAL;
    }
}

std::unique_ptr<Option> Option4ClientFqdn::clone() const {
    return (cloneInternal<Option4ClientFqdn>());
}

bool Option4ClientFqdn::getFlag(const uint8_t flag) const {
    // Caller should query for one of the: E, N, S or O flags. Any other value
    /// is invalid and results in the exception.
    if (flag != FLAG_S && flag != FLAG_O && flag != FLAG_N && flag != FLAG_E) {
        kea_throw(InvalidOption4FqdnFlags, "invalid DHCPv4 Client FQDN"
                << " Option flag specified, expected E, N, S or O");
    }

    return ((flags_ & flag) != 0);
}

void Option4ClientFqdn::setFlag(const uint8_t flag, const bool set_flag) {
    if (((flag & ~FLAG_MASK) != 0) || (flag == 0)) {
        kea_throw(InvalidOption4FqdnFlags, "invalid DHCPv4 Client FQDN"
                << " Option flag 0x" << std::hex
                << static_cast<int>(flag) << std::dec
                << " is being set. Expected combination of E, N, S and O");
    }

    uint8_t new_flag = flags_;
    if (set_flag) {
        new_flag |= flag;
    } else {
        new_flag &= ~flag;
    }

    checkFlags(new_flag, true);
    flags_ = new_flag;
}

std::pair<Option4ClientFqdn::Rcode, Option4ClientFqdn::Rcode> Option4ClientFqdn::getRcode() const {
    return (std::make_pair(rcode1_, rcode2_));
}

void Option4ClientFqdn::setRcode(const Rcode& rcode) {
    rcode1_ = rcode;
    rcode2_ = rcode;
}

void Option4ClientFqdn::resetFlags() {
    flags_ = 0;
}

std::string Option4ClientFqdn::getDomainName() const {
    if (domain_name_) {
        return (domain_name_->toText(domain_name_type_ == PARTIAL));
    }
    return ("");
}

void Option4ClientFqdn::packDomainName(kea::util::OutputBuffer& buf) const {
    if (domain_name_) {
        if (getFlag(FLAG_E)) {
            // Domain name, encoded as a set of labels.
            kea::dns::LabelSequence labels(*domain_name_);
            if (labels.getDataLength() > 0) {
                size_t read_len = 0;
                const uint8_t* data = labels.getData(&read_len);
                if (domain_name_type_ == PARTIAL) {
                    --read_len;
                }
                buf.writeData(data, read_len);
            }
        } else {
            std::string domain_name = getDomainName();
            if (!domain_name.empty()) {
                buf.writeData(&domain_name[0], domain_name.size());
            }

        }
    }
}

void Option4ClientFqdn::resetDomainName() {
    setDomainName("", PARTIAL);
}

Option4ClientFqdn::DomainNameType Option4ClientFqdn::getDomainNameType() const {
    return (domain_name_type_);
}

void Option4ClientFqdn::pack(kea::util::OutputBuffer& buf) const {
    // Header = option code and length.
    packHeader(buf);
    // Flags field.
    buf.writeUint8(flags_);
    // RCODE1 and RCODE2
    buf.writeUint8(rcode1_.getCode());
    buf.writeUint8(rcode2_.getCode());
    // Domain name.
    packDomainName(buf);
}

void Option4ClientFqdn::unpack(OptionBufferConstIter first,
        OptionBufferConstIter last) {
    setData(first, last);
    parseWireData(first, last);
    // Check that the flags in the received option are valid. Ignore MBZ bits,
    // because we don't want to dkeaard the whole option because of MBZ bits
    // being set.
    checkFlags(flags_, false);
}

std::string Option4ClientFqdn::toText(int indent) const {
    std::ostringstream stream;
    std::string in(indent, ' '); // base indentation
    stream << in  << "type=" << type_ << " (CLIENT_FQDN), "
           <<  "flags: ("
           << "N=" << (getFlag(FLAG_N) ? "1" : "0") << ", "
           << "E=" << (getFlag(FLAG_E) ? "1" : "0") << ", "
           << "O=" << (getFlag(FLAG_O) ? "1" : "0") << ", "
           << "S=" << (getFlag(FLAG_S) ? "1" : "0") << "), "
           << "domain-name='" << getDomainName() << "' ("
           << (getDomainNameType() == PARTIAL ? "partial" : "full")
           << ")";

    return (stream.str());
}

uint16_t Option4ClientFqdn::len() const {
    uint16_t domain_name_length = 0;
    if (domain_name_) {
        if (getFlag(FLAG_E)) {
            domain_name_length = domain_name_type_ == FULL ?
                domain_name_->getLength() :
                domain_name_->getLength() - 1;
        } else {
            domain_name_length = getDomainName().length();
        }
    }

    return (getHeaderLen() + FIXED_FIELDS_LEN + domain_name_length);
}

}; 
};
