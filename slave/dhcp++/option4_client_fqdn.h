#pragma once

#include <kea/dhcp++/option.h>
#include <kea/dns/name.h>

#include <string>
#include <utility>

namespace kea {
namespace dhcp {

class InvalidOption4FqdnFlags : public Exception {
public:
    InvalidOption4FqdnFlags(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class InvalidOption4FqdnDomainName : public Exception {
public:
    InvalidOption4FqdnDomainName(const char* file, size_t line,
                                 const char* what) :
        kea::Exception(file, line, what) {}
};


class Option4ClientFqdn : public Option {
public:

    static const uint8_t FLAG_S = 0x01; ///< Bit S
    static const uint8_t FLAG_O = 0x02; ///< Bit O
    static const uint8_t FLAG_E = 0x04; ///< Bit E
    static const uint8_t FLAG_N = 0x08; ///< Bit N

    static const uint8_t FLAG_MASK = 0xF;

    class Rcode {
    public:
        Rcode(const uint8_t rcode)
            : rcode_(rcode) { }

        uint8_t getCode() const {
            return (rcode_);
        }

    private:
        uint8_t rcode_;
    };


    enum DomainNameType {
        PARTIAL,
        FULL
    };

    static const uint16_t FIXED_FIELDS_LEN = 3;

    Option4ClientFqdn(const uint8_t flags,
                      const Rcode& rcode,
                      const std::string& domain_name,
                      const DomainNameType domain_name_type = FULL);

    Option4ClientFqdn(const uint8_t flags, const Rcode& rcode);

    Option4ClientFqdn(OptionBufferConstIter first,
                      OptionBufferConstIter last);

    Option4ClientFqdn(const Option4ClientFqdn& source);

    virtual std::unique_ptr<Option> clone() const;

    virtual ~Option4ClientFqdn();

    Option4ClientFqdn& operator=(const Option4ClientFqdn& source);

    bool getFlag(const uint8_t flag) const;
    void setFlag(const uint8_t flag, const bool set);
    void resetFlags();

    std::pair<Rcode, Rcode> getRcode() const;
    void setRcode(const Rcode& rcode);
    std::string getDomainName() const;
    void packDomainName(kea::util::OutputBuffer& buf) const;
    void setDomainName(const std::string& domain_name,
                       const DomainNameType domain_name_type);

    void resetDomainName();

    DomainNameType getDomainNameType() const;

    virtual void pack(kea::util::OutputBuffer& buf) const;

    virtual void unpack(OptionBufferConstIter first,
                        OptionBufferConstIter last);

    virtual std::string toText(int indent = 0) const;

    virtual uint16_t len() const;

    inline static const Rcode& RCODE_SERVER() {
        static Rcode rcode(255);
        return (rcode);
    }

    inline static const Rcode& RCODE_CLIENT() {
        static Rcode rcode(0);
        return (rcode);
    }

private:
    void checkFlags(const uint8_t, const bool);
    void parseWireData(OptionBufferConstIter, OptionBufferConstIter);
    void parseCanonicalDomainName(OptionBufferConstIter, OptionBufferConstIter);
    void parseASCIIDomainName(OptionBufferConstIter, OptionBufferConstIter);

    uint8_t flags_;
    Rcode rcode1_;
    Rcode rcode2_;
    std::shared_ptr<kea::dns::Name> domain_name_;
    DomainNameType domain_name_type_;
};

typedef std::shared_ptr<Option4ClientFqdn> Option4ClientFqdnPtr;

};
};
