#pragma once


#include <kea/dhcp++/option_definition.h>
#include <map>
#include <unordered_map>
#include <memory>


namespace kea {
namespace dhcp {

class OptionMeta {
public:
    explicit OptionMeta();

    OptionMeta(const OptionMeta&) = delete;
    OptionMeta(const OptionMeta&&) = delete;
    OptionMeta& operator=(const OptionMeta&) = delete;
    OptionMeta& operator=(const OptionMeta&&) = delete;

    void clear();

    void addOptionDef(std::unique_ptr<OptionDefinition> def);
    const OptionDefinition* getOptionDef(const std::string& ) const;
    const OptionDefinition* getOptionDef(std::uint16_t) const;

private:
    std::map<uint16_t, std::shared_ptr<OptionDefinition>> code_to_def_;
    std::unordered_map<std::string, std::shared_ptr<OptionDefinition>> name_to_def_;
};

};
};
