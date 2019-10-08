#pragma once

#include <kea/dhcp++/duid.h>
#include <cstdint>
#include <string>
#include <vector>

namespace kea {
namespace dhcp {

class DUIDFactory {
public:

    DUIDFactory(const std::string& storage_location = "");

    bool isStored() const;
    void createLLT(const uint16_t htype, const uint32_t time_in,
                   const std::vector<uint8_t>& ll_identifier);
    void createEN(const uint32_t enterprise_id,
                  const std::vector<uint8_t>& identifier);
    void createLL(const uint16_t htype,
                  const std::vector<uint8_t>& ll_identifier);
    std::shared_ptr<DUID> get();

private:

    void createLinkLayerId(std::vector<uint8_t>& identifier,
                           uint16_t& htype) const;

    void set(const std::vector<uint8_t>& duid_vector);
    void readFromFile();

    std::string storage_location_;
    std::shared_ptr<DUID> duid_;

};
}; 
};
