#pragma once 

#include <cstdint>
#include <string>
#include <vector>
#include <kea/dns/exceptions.h>

namespace kea {
namespace dns {
namespace name {
namespace internal {
    extern const uint8_t maptolower[];
}; 
};
};
namespace util {
class InputBuffer;
class OutputBuffer;
};

namespace dns {

class EmptyLabel : public NameParserException {
public:
    EmptyLabel(const char* file, size_t line, const char* what) :
        NameParserException(file, line, what) {}
};

class TooLongName : public NameParserException {
public:
    TooLongName(const char* file, size_t line, const char* what) :
        NameParserException(file, line, what) {}
};

class TooLongLabel : public NameParserException {
public:
    TooLongLabel(const char* file, size_t line, const char* what) :
        NameParserException(file, line, what) {}
};

class BadLabelType : public NameParserException {
public:
    BadLabelType(const char* file, size_t line, const char* what) :
        NameParserException(file, line, what) {}
};

class BadEscape : public NameParserException {
public:
    BadEscape(const char* file, size_t line, const char* what) :
        NameParserException(file, line, what) {}
};

class IncompleteName : public NameParserException {
public:
    IncompleteName(const char* file, size_t line, const char* what) :
        NameParserException(file, line, what) {}
};

class MissingNameOrigin : public NameParserException {
public:
    MissingNameOrigin(const char* file, size_t line, const char* what) :
        NameParserException(file, line, what) {}
};

class NameComparisonResult {
public:
    enum NameRelation {
        SUPERDOMAIN = 0,
        SUBDOMAIN = 1,
        EQUAL = 2,
        COMMONANCESTOR = 3,
        NONE = 4
    };

    NameComparisonResult(int order, unsigned int nlabels,
                         NameRelation relation) :
        order_(order), nlabels_(nlabels), relation_(relation) {}

    int getOrder() const { return (order_); }
    unsigned int getCommonLabels() const { return (nlabels_); }
    NameRelation getRelation() const { return (relation_); }
private:
    int order_;
    unsigned int nlabels_;
    NameRelation relation_;
};

class Name {
    friend class LabelSequence;

private:
    typedef std::basic_string<uint8_t> NameString;
    typedef std::vector<uint8_t> NameOffsets;

    Name() : length_(0), labelcount_(0) {}

public:
    explicit Name(const std::string& namestr, bool downcase = false);
    Name(const char* name_data, size_t data_len, const Name* origin,
         bool downcase = false);

    explicit Name(kea::util::InputBuffer& buffer, bool downcase = false);

    uint8_t at(size_t pos) const
    {
        if (pos >= length_) {
            kea_throw(OutOfRange, "Out of range access in Name::at()");
        }
        return (ndata_[pos]);
    }

    size_t getLength() const { return (length_); }

    unsigned int getLabelCount() const { return (labelcount_); }
    std::string toText(bool omit_final_dot = false) const;

    void toWire(kea::util::OutputBuffer& buffer) const;
    NameComparisonResult compare(const Name& other) const;

public:
    bool equals(const Name& other) const;

    bool operator==(const Name& other) const { return (equals(other)); }

    bool nequals(const Name& other) const { return (!(equals(other))); }

    bool operator!=(const Name& other) const { return (nequals(other)); }

    bool leq(const Name& other) const;

    bool operator<=(const Name& other) const { return (leq(other)); }

    bool geq(const Name& other) const;

    bool operator>=(const Name& other) const { return (geq(other)); }

    bool lthan(const Name& other) const;

    bool operator<(const Name& other) const { return (lthan(other)); }

    bool gthan(const Name& other) const;

    bool operator>(const Name& other) const { return (gthan(other)); }

    Name split(unsigned int first, unsigned int n) const;

    Name split(unsigned int level) const;

    Name reverse() const;

    Name concatenate(const Name& suffix) const;

    Name& downcase();

    bool isWildcard() const;

    static const size_t MAX_WIRE = 255;

    static const size_t MAX_LABELS = 128;

    static const size_t MAX_LABELLEN = 63;

    static const uint16_t MAX_COMPRESS_POINTER = 0x3fff;
    static const uint16_t COMPRESS_POINTER_MARK8 = 0xc0;
    static const uint16_t COMPRESS_POINTER_MARK16 = 0xc000;

    static const Name& ROOT_NAME();

private:
    NameString ndata_;
    NameOffsets offsets_;
    unsigned int length_;
    unsigned int labelcount_;
};

inline const Name&
Name::ROOT_NAME() {
    static Name root_name(".");
    return (root_name);
}

std::ostream&
operator<<(std::ostream& os, const Name& name);

};
} ;
