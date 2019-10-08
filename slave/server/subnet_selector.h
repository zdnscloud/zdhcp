#pragma once

#include <kea/dhcp++/classify.h>
#include <kea/dhcp++/option.h>
#include <kea/dhcp++/subnet.h>
#include <kea/util/io_address.h>

namespace kea {
namespace server {

using IOAddress = kea::util::IOAddress;
using ClientClasses = kea::dhcp::ClientClasses;

struct SubnetSelector {
    IOAddress ciaddr_;
    IOAddress giaddr_;
    IOAddress option_select_; //RAI link select or subnet select option

    IOAddress local_address_;
    IOAddress remote_address_;
    ClientClasses client_classes_;
    std::string iface_name_;

    SubnetSelector()
        : ciaddr_(IOAddress("0.0.0.0")),
          giaddr_(IOAddress("0.0.0.0")),
          option_select_(IOAddress("0.0.0.0")),
          local_address_(IOAddress("0.0.0.0")),
          remote_address_(IOAddress("0.0.0.0")),
          client_classes_(), 
          iface_name_(std::string()){}
};
};
};
