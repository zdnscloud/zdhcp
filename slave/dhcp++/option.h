#pragma once

#include <kea/util/buffer.h>
#include <map>
#include <string>
#include <vector>

namespace kea {
namespace dhcp {

typedef std::vector<uint8_t> OptionBuffer;
typedef OptionBuffer::iterator OptionBufferIter;
typedef OptionBuffer::const_iterator OptionBufferConstIter;
typedef std::shared_ptr<OptionBuffer> OptionBufferPtr;

class Option;

typedef std::multimap<unsigned int, std::unique_ptr<Option>> OptionCollection;

class Option {
public:
    const static size_t OPTION4_HDR_LEN = 2;


    typedef std::unique_ptr<Option> Factory(uint16_t type, 
                                            const OptionBuffer& buf);

    static std::unique_ptr<Option> factory(uint16_t type,
                                           const OptionBuffer& buf);

    static std::unique_ptr<Option> factory(uint16_t type) {
        return factory(type, OptionBuffer());
    }

    Option( uint16_t type);
    Option( uint16_t type, const OptionBuffer& data);
    Option( uint16_t type, OptionBufferConstIter first,
           OptionBufferConstIter last);

    Option(const Option& source);
    Option& operator=(const Option& rhs);

    virtual std::unique_ptr<Option> clone() const;

    virtual void pack(kea::util::OutputBuffer& buf) const;
    virtual void unpack(OptionBufferConstIter begin,
                        OptionBufferConstIter end);

    virtual std::string toText(int indent = 0) const;
    virtual std::string toString() const;
    virtual std::vector<uint8_t> toBinary(const bool include_header = false) const;
    virtual std::string toHexString(const bool include_header = false) const;

    uint16_t getType() const { return (type_); }
    virtual uint16_t len() const;
    virtual uint16_t getHeaderLen() const;

    virtual const OptionBuffer& getData() const { return (data_); }

    //sub option
    void addOption(std::unique_ptr<Option> opt);
    const Option* getOption(uint16_t type) const;
    const OptionCollection& getOptions() const {
        return (options_);
    }
    void getOptionsCopy(OptionCollection& options_copy) const;
    bool delOption(uint16_t type);

    uint8_t getUint8() const;
    uint16_t getUint16() const;
    uint32_t getUint32() const;
    void setUint8(uint8_t value);
    void setUint16(uint16_t value);
    void setUint32(uint32_t value);

    template<typename InputIterator>
    void setData(InputIterator first, InputIterator last) {
        data_.assign(first, last);
    }

    void setEncapsulatedSpace(const std::string& encapsulated_space) {
        encapsulated_space_ = encapsulated_space;
    }
    std::string getEncapsulatedSpace() const {
        return (encapsulated_space_);
    }

    virtual ~Option();

    bool equals(const Option& other) const;
protected:

    template<typename OptionType>
    std::unique_ptr<Option> cloneInternal() const {
        return std::unique_ptr<OptionType> (new OptionType(*dynamic_cast<const OptionType*>(this)));
    }
    void packHeader(kea::util::OutputBuffer& buf) const;
    void packOptions(kea::util::OutputBuffer& buf) const;
    void unpackOptions(const OptionBuffer& buf);
    std::string headerToText(const int indent = 0,
                             const std::string& type_name = "") const;
    std::string suboptionsToText(const int indent = 0) const;

    void check() const;

    uint16_t type_;
    OptionBuffer data_;
    OptionCollection options_;
    std::string encapsulated_space_;
};

}; 
};
