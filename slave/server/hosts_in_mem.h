#pragma once
#include <kea/dhcp++/duid.h>
#include <kea/dhcp++/hwaddr.h>
#include <kea/server/host.h>
#include <kea/server/subnet_id.h>
#include <kea/server/base_host_data_source.h>
#include <vector>

using namespace kea::dhcp;

namespace kea {
namespace server {

class HostsInMem : public BaseHostDataSource {
public:
    virtual ~HostsInMem() { }

    virtual HostCollection getAll(const HWAddr*, const DUID* duid = nullptr) const;

    virtual HostCollection getAll4(const IOAddress&) const;

    virtual const Host* get4(SubnetID , const HWAddr* , const DUID*) const;

    virtual const Host* get4(SubnetID, const IOAddress&) const;

    virtual const Host* get4(SubnetID, Host::IdentifierType, const uint8_t*, size_t) const;

    virtual void add(std::unique_ptr<Host>) ;

    virtual std::string getType() const { return (std::string("in memory")); }

private:
    std::vector<std::unique_ptr<Host>> hosts_;
};
};
};
