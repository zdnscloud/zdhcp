#pragma once

#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_definition.h>
#include <kea/util/buffer.h>

#include <iostream>
#include <stdint.h>
#include <string>

namespace kea {
namespace dhcp {

class LibDHCP {

public:

    typedef std::unordered_map<unsigned short, Option::Factory*>  FactoryMap;
    static void initOptions();

    static const OptionDefinition* getStdOptionDef(uint16_t code);
    static const OptionDefinition* getStdOptionDef(const std::string& name);

    static const OptionDefinition* getVendorOptionDef(uint32_t vendor_id,
                                                  uint16_t code);
    static const OptionDefinition* getVendorOptionDef(uint32_t vendor_id,
                                                  const std::string& name);

    static const OptionDefinition*  getRuntimeOptionDef(const std::string& space,
                                                   uint16_t code);
    static const OptionDefinition* getRuntimeOptionDef(const std::string& space,
                                                   const std::string& name);

    static bool isStandardOption(uint16_t code);

    static std::unique_ptr<Option> optionFactory(uint16_t type,
                                                 const OptionBuffer& buf);

    static void packOptions4(kea::util::OutputBuffer& buf,
                             const OptionCollection& options);


    static size_t unpackOptions4(const OptionBuffer& buf,
                                 const std::string& option_space,
                                 OptionCollection& options);

    static void OptionFactoryRegister(uint16_t type,
                                      Option::Factory* factory);


    static size_t unpackVendorOptions4(uint32_t vendor_id,
                                       const OptionBuffer& buf,
                                       OptionCollection& options);

    static void addRuntimeOptionDef(const string &space, std::unique_ptr<OptionDefinition>);
    static void clearRuntimeOptionDefs();

    static uint32_t optionSpaceToVendorId(const std::string& option_space);
};
};
};
