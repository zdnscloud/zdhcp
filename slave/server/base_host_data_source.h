#pragma once
#include <kea/dhcp++/duid.h>
#include <kea/dhcp++/hwaddr.h>
#include <kea/server/host.h>
#include <kea/exceptions/exceptions.h>

using namespace kea::dhcp;

namespace kea {
namespace server {

class DuplicateHost : public Exception {
public:
    DuplicateHost(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};

class ReservedAddress : public Exception {
public:
    ReservedAddress(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};

class BadHostAddress : public kea::BadValue {
public:
    BadHostAddress(const char* file, size_t line, const char* what) :
        kea::BadValue(file, line, what) { };
};

class BaseHostDataSource {
public:
    enum IdType {
        ID_HWADDR = 0, ///< Hardware address
        ID_DUID = 1    ///< DUID/client-id
    };

    virtual ~BaseHostDataSource() { }

    virtual HostCollection getAll(const HWAddr*, const DUID* duid) const = 0;

    virtual HostCollection getAll4(const IOAddress&) const = 0;

    virtual const Host* get4(SubnetID , const HWAddr* , const DUID*) const = 0;

    virtual const Host* get4(SubnetID, const IOAddress&) const = 0;

    virtual const Host*  get4(SubnetID, Host::IdentifierType ,
            const uint8_t* identifier_begin, size_t identifier_len) const = 0;

    virtual void add(std::unique_ptr<Host>) = 0;

    virtual std::string getType() const = 0;

    virtual void commit() {};

    virtual void rollback() {};
};
};
}
