#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/std_option_defs.h>
#include <kea/dhcp++/option_description.h>
#include <kea/dhcp++/option_definition.h>
#include <kea/exceptions/exceptions.h>
#include <memory>

namespace kea {
namespace dhcp {

static OptionMeta v4_option_meta_;

static const OptionDefinition::RecordFieldsCollection FQDN_RECORDS {OPT_UINT8_TYPE, OPT_UINT8_TYPE, OPT_UINT8_TYPE, OPT_FQDN_TYPE};
static const OptionDefinition::RecordFieldsCollection VIVCO_RECORDS {OPT_UINT32_TYPE, OPT_BINARY_TYPE};
static const OptionDefinition::RecordFieldsCollection CLIENT_NDI_RECORDS {OPT_UINT8_TYPE, OPT_UINT8_TYPE, OPT_UINT8_TYPE};
static const OptionDefinition::RecordFieldsCollection UUID_GUID_RECORDS {OPT_UINT8_TYPE, OPT_BINARY_TYPE};
static const OptionDefinition::RecordFieldsCollection CLIENT_FQDN_RECORDS {OPT_UINT8_TYPE, OPT_FQDN_TYPE};
static const OptionDefinition::RecordFieldsCollection GEOCONF_CIVIC_RECORDS {OPT_UINT8_TYPE, OPT_UINT16_TYPE, OPT_BINARY_TYPE};
static const OptionDefinition::RecordFieldsCollection IA_NA_RECORDS {OPT_UINT32_TYPE, OPT_UINT32_TYPE, OPT_UINT32_TYPE};
static const OptionDefinition::RecordFieldsCollection IA_PD_RECORDS {OPT_UINT32_TYPE, OPT_UINT32_TYPE, OPT_UINT32_TYPE};
static const OptionDefinition::RecordFieldsCollection REMOTE_ID_RECORDS {OPT_UINT32_TYPE, OPT_BINARY_TYPE};
static const OptionDefinition::RecordFieldsCollection STATUS_CODE_RECORDS {OPT_UINT16_TYPE, OPT_STRING_TYPE};
static const OptionDefinition::RecordFieldsCollection VENDOR_CLASS_RECORDS {OPT_UINT32_TYPE, OPT_BINARY_TYPE};
static const OptionDefinition::RecordFieldsCollection SIGNATURE_RECORDS {OPT_UINT8_TYPE, OPT_UINT8_TYPE, OPT_BINARY_TYPE};
static const OptionDefinition::RecordFieldsCollection CLIENT_NII_RECORDS {OPT_UINT8_TYPE, OPT_UINT8_TYPE, OPT_UINT8_TYPE};

const std::vector<OptionDescription> OPTION_DEF_PARAMS4 {
    { "subnet-mask", DHO_SUBNET_MASK, OPT_IPV4_ADDRESS_TYPE,false, EMPTY_RECORDS, ""},
    { "time-offset", DHO_TIME_OFFSET, OPT_INT32_TYPE, false, EMPTY_RECORDS, ""},
    { "routers", DHO_ROUTERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "time-servers", DHO_TIME_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "name-servers", DHO_NAME_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "domain-name-servers", DHO_DOMAIN_NAME_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "log-servers", DHO_LOG_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "cookie-servers", DHO_COOKIE_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "lpr-servers", DHO_LPR_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "impress-servers", DHO_IMPRESS_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "resource-location-servers", DHO_RESOURCE_LOCATION_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "host-name", DHO_HOST_NAME, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "boot-size", DHO_BOOT_SIZE, OPT_UINT16_TYPE, false, EMPTY_RECORDS, "" },
    { "merit-dump", DHO_MERIT_DUMP, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "domain-name", DHO_DOMAIN_NAME, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "swap-server", DHO_SWAP_SERVER, OPT_IPV4_ADDRESS_TYPE, false, EMPTY_RECORDS, "" },
    { "root-path", DHO_ROOT_PATH, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "extensions-path", DHO_EXTENSIONS_PATH, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "ip-forwarding", DHO_IP_FORWARDING, OPT_BOOLEAN_TYPE, false, EMPTY_RECORDS, "" },
    { "non-local-source-routing", DHO_NON_LOCAL_SOURCE_ROUTING, OPT_BOOLEAN_TYPE, false, EMPTY_RECORDS, "" },
    { "policy-filter", DHO_POLICY_FILTER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "max-dgram-reassembly", DHO_MAX_DGRAM_REASSEMBLY, OPT_UINT16_TYPE, false, EMPTY_RECORDS, "" },
    { "default-ip-ttl", DHO_DEFAULT_IP_TTL, OPT_UINT8_TYPE, false, EMPTY_RECORDS, "" },
    { "path-mtu-aging-timeout", DHO_PATH_MTU_AGING_TIMEOUT, OPT_UINT32_TYPE, false, EMPTY_RECORDS, "" },
    { "path-mtu-plateau-table", DHO_PATH_MTU_PLATEAU_TABLE, OPT_UINT16_TYPE, true, EMPTY_RECORDS, "" },
    { "interface-mtu", DHO_INTERFACE_MTU, OPT_UINT16_TYPE, false, EMPTY_RECORDS, "" },
    { "all-subnets-local", DHO_ALL_SUBNETS_LOCAL, OPT_BOOLEAN_TYPE, false, EMPTY_RECORDS, "" },
    { "broadcast-address", DHO_BROADCAST_ADDRESS, OPT_IPV4_ADDRESS_TYPE, false, EMPTY_RECORDS, "" },
    { "perform-mask-discovery", DHO_PERFORM_MASK_DISCOVERY, OPT_BOOLEAN_TYPE, false, EMPTY_RECORDS, "" },
    { "mask-supplier", DHO_MASK_SUPPLIER, OPT_BOOLEAN_TYPE, false, EMPTY_RECORDS, "" },
    { "router-discovery", DHO_ROUTER_DISCOVERY, OPT_BOOLEAN_TYPE, false, EMPTY_RECORDS, "" },
    { "router-solicitation-address", DHO_ROUTER_SOLICITATION_ADDRESS, OPT_IPV4_ADDRESS_TYPE, false, EMPTY_RECORDS, "" },
    { "static-routes", DHO_STATIC_ROUTES, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "trailer-encapsulation", DHO_TRAILER_ENCAPSULATION, OPT_BOOLEAN_TYPE, false, EMPTY_RECORDS, "" },
    { "arp-cache-timeout", DHO_ARP_CACHE_TIMEOUT, OPT_UINT32_TYPE, false, EMPTY_RECORDS, "" },
    { "ieee802-3-encapsulation", DHO_IEEE802_3_ENCAPSULATION, OPT_BOOLEAN_TYPE, false, EMPTY_RECORDS, "" },
    { "default-tcp-ttl", DHO_DEFAULT_TCP_TTL, OPT_UINT8_TYPE, false, EMPTY_RECORDS, "" },
    { "tcp-keepalive-interval", DHO_TCP_KEEPALIVE_INTERVAL, OPT_UINT32_TYPE, false, EMPTY_RECORDS, "" },
    { "tcp-keepalive-garbage", DHO_TCP_KEEPALIVE_GARBAGE, OPT_BOOLEAN_TYPE, false, EMPTY_RECORDS, "" },
    { "nis-domain", DHO_NIS_DOMAIN, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "nis-servers", DHO_NIS_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "ntp-servers", DHO_NTP_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "vendor-encapsulated-options", DHO_VENDOR_ENCAPSULATED_OPTIONS, OPT_EMPTY_TYPE, false, EMPTY_RECORDS, "vendor-encapsulated-options-space" },
    { "netbios-name-servers", DHO_NETBIOS_NAME_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "netbios-dd-server", DHO_NETBIOS_DD_SERVER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "netbios-node-type", DHO_NETBIOS_NODE_TYPE, OPT_UINT8_TYPE, false, EMPTY_RECORDS, "" },
    { "netbios-scope", DHO_NETBIOS_SCOPE, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "font-servers", DHO_FONT_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "x-display-manager", DHO_X_DISPLAY_MANAGER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "dhcp-requested-address", DHO_DHCP_REQUESTED_ADDRESS, OPT_IPV4_ADDRESS_TYPE, false, EMPTY_RECORDS, "" },
    { "dhcp-lease-time", DHO_DHCP_LEASE_TIME, OPT_UINT32_TYPE, false, EMPTY_RECORDS, "" },
    { "dhcp-option-overload", DHO_DHCP_OPTION_OVERLOAD, OPT_UINT8_TYPE, false, EMPTY_RECORDS, "" },
    { "dhcp-message-type", DHO_DHCP_MESSAGE_TYPE, OPT_UINT8_TYPE, false, EMPTY_RECORDS, "" },
    { "dhcp-server-identifier", DHO_DHCP_SERVER_IDENTIFIER, OPT_IPV4_ADDRESS_TYPE, false, EMPTY_RECORDS, "" },
    { "dhcp-parameter-request-list", DHO_DHCP_PARAMETER_REQUEST_LIST, OPT_UINT8_TYPE, true, EMPTY_RECORDS, "" },
    { "dhcp-message", DHO_DHCP_MESSAGE, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "dhcp-max-message-size", DHO_DHCP_MAX_MESSAGE_SIZE, OPT_UINT16_TYPE, false, EMPTY_RECORDS, "" },
    { "dhcp-renewal-time", DHO_DHCP_RENEWAL_TIME, OPT_UINT32_TYPE, false, EMPTY_RECORDS, "" },
    { "dhcp-rebinding-time", DHO_DHCP_REBINDING_TIME, OPT_UINT32_TYPE, false, EMPTY_RECORDS, "" },
    { "vendor-class-identifier", DHO_VENDOR_CLASS_IDENTIFIER, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "dhcp-client-identifier", DHO_DHCP_CLIENT_IDENTIFIER, OPT_BINARY_TYPE, false, EMPTY_RECORDS, "" },
    { "nwip-domain-name", DHO_NWIP_DOMAIN_NAME, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "nwip-suboptions", DHO_NWIP_SUBOPTIONS, OPT_BINARY_TYPE, false, EMPTY_RECORDS, "" },
    { "nisplus-domain-name", DHO_NISP_DOMAIN_NAME, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "nisplus-servers", DHO_NISP_SERVER_ADDR, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "tftp-server-name", DHO_TFTP_SERVER_NAME, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "boot-file-name", DHO_BOOT_FILE_NAME, OPT_STRING_TYPE, false, EMPTY_RECORDS, "" },
    { "mobile-ip-home-agent", DHO_HOME_AGENT_ADDRS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "smtp-server", DHO_SMTP_SERVER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "pop-server", DHO_POP3_SERVER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "nntp-server", DHO_NNTP_SERVER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "www-server", DHO_WWW_SERVER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "finger-server", DHO_FINGER_SERVER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "irc-server", DHO_IRC_SERVER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "streettalk-server", DHO_STREETTALK_SERVER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "streettalk-directory-assistance-server", DHO_STDASERVER, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "user-class", DHO_USER_CLASS, OPT_BINARY_TYPE, false, EMPTY_RECORDS, "" },
    { "fqdn", DHO_FQDN, OPT_RECORD_TYPE, false, FQDN_RECORDS, "" },
    { "dhcp-agent-options", DHO_DHCP_AGENT_OPTIONS, OPT_EMPTY_TYPE, false, EMPTY_RECORDS, "dhcp-agent-options-space" },
    // Unfortunatelly the AUTHENTICATE option contains a 64-bit
    // data field called 'replay-detection' that can't be added
    // as a record field to a custom option. Also, there is no
    // dedicated option class to handle it so we simply return
    // binary option type for now.
    // @todo implement a class to handle AUTH option.
    { "authenticate", DHO_AUTHENTICATE, OPT_BINARY_TYPE, false, EMPTY_RECORDS, "" },
    { "client-last-transaction-time", DHO_CLIENT_LAST_TRANSACTION_TIME, OPT_UINT32_TYPE, false, EMPTY_RECORDS, "" },
    { "associated-ip", DHO_ASSOCIATED_IP, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" },
    { "client-system", DHO_SYSTEM, OPT_UINT16_TYPE, true, EMPTY_RECORDS, "" },
    { "client-ndi", DHO_NDI, OPT_RECORD_TYPE, false, CLIENT_NDI_RECORDS, "" },
    { "uuid-guid", DHO_UUID_GUID, OPT_RECORD_TYPE, false, UUID_GUID_RECORDS, "" },
    { "subnet-selection", DHO_SUBNET_SELECTION, OPT_IPV4_ADDRESS_TYPE, false, EMPTY_RECORDS, "" },
    // The following options need a special encoding of data
    // being carried by them. Therefore, there is no way they can
    // be handled by OptionCustom. We may need to implement
    // dedicated classes to handle them. Until that happens
    // let's treat them as 'binary' options.
    { "domain-search", DHO_DOMAIN_SEARCH, OPT_FQDN_TYPE, true, EMPTY_RECORDS, "" },
    { "vivco-suboptions", DHO_VIVCO_SUBOPTIONS, OPT_RECORD_TYPE, false, VIVCO_RECORDS, "" },
    // Vendor-Identifying Vendor Specific Information option payload begins with a
    // 32-bit log enterprise number, followed by a tuple of data-len/option-data.
    // The format defined here includes 32-bit field holding enterprise number.
    // This allows for specifying option-data information where the enterprise-id
    // is represented by a uint32_t value. Previously we represented this option
    // as a binary, but that would imply that enterprise number would have to be
    // represented in binary format in the server configuration. That would be
    // inconvenient and non-intuitive.
    /// @todo We need to extend support for vendor options with ability to specify
    /// multiple enterprise numbers for a single option. Perhaps it would be
    /// ok to specify multiple instances of the "vivso-suboptions" which will be
    /// combined in a single option by the server before responding to a client.
    { "vivso-suboptions", DHO_VIVSO_SUBOPTIONS, OPT_UINT32_TYPE, false, EMPTY_RECORDS, "" }
};


void initStdOptions() {
    v4_option_meta_.clear();
    for (auto& description : OPTION_DEF_PARAMS4) {
        v4_option_meta_.addOptionDef(optionDefFromDescripton(description));
    }
}

OptionMeta& getStdV4Options() {
    return v4_option_meta_;
}


};
};

