#pragma once

#include <kea/nic/iface_mgr.h>
#include <kea/server/ctrl_server.h>
#include <kea/server/hosts_in_mem.h>
#include <kea/server/subnet_mgr.h>
#include <kea/server/client_class_manager.h>
#include <kea/dhcp++/std_option_defs.h>
#include <kea/dhcp++/vendor_option_defs.h>
#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/libdhcp++.h>
#include <kea/dhcp++/option_description.h>
#include <kea/util/strutil.h>
#include <kea/hooks/hooks_manager.h>
#include <kea/rpc/rpc_allocate_engine.h>
#include <kea/logging/logging.h>
#include <kea/statistics/pkt_statistic.h>
#include <kea/ping/ping.h>

#include <regex>
#include <string>
#include <memory>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

using namespace kea;
using namespace kea::nic;
using namespace kea::server;
using namespace kea::configure;
using namespace kea::dhcp;
using namespace kea::util;
using namespace kea::hooks;
using namespace kea::pinger;
using namespace kea::logging;
using namespace kea::statis;
using namespace std;

namespace kea{
namespace server{

using namespace kea::server;
using namespace kea::configure;
using namespace kea::nic;

static const string DEFAULT_KEA_MASTER_IP = "127.0.0.1";
static const int DEFAULT_KEA_MASTER_PORT = 5555;

void 
initNic(const JsonConf& conf) {
    IfaceMgr::init();
    vector<string> interfaces = conf.root().getStrings("dhcp4.interfaces-config.interfaces");
    int port = DHCP4_SERVER_PORT;
    if (conf.root().hasKey("dhcp4.interfaces-config.port")) {
        port = conf.root().getInt("dhcp4.interfaces-config.port");
    }

    for (auto& interface : interfaces) {
        vector<string> nicAndIp = str::tokens(interface, "/");

        if (nicAndIp.size() == 2) {
            IfaceMgr::instance().openSocket(nicAndIp[0], IOAddress(nicAndIp[1]), port, true, true);
        }
    }
}

void
initLog(const JsonConf& conf) {
    string log_path = "";
    string log_level = "trace";
    bool log_enable = false;
    if (conf.root().hasKey("dhcp4.logging")) {
        if (conf.root().hasKey("dhcp4.logging.log-enable")) {
            log_enable = conf.root().getBool("dhcp4.logging.log-enable");
        }
        if (conf.root().hasKey("dhcp4.logging.log-file-dir")) {
            log_path = conf.root().getString("dhcp4.logging.log-file-dir");
            while (*log_path.rbegin() == '/') {
                log_path.erase(log_path.end() - 1);
            }
            log_path += "/kea-slave.log";
        }
        if (conf.root().hasKey("dhcp4.logging.log-level")) {
            log_level = conf.root().getString("dhcp4.logging.log-level");
        }
        Logger::open_log(log_path, "kea_slave ", strToLogLevel(log_level), log_enable);
    }
}
void
initPingCheck(const JsonConf& conf) {
    bool ping_enable = false;
    uint32_t time_out = 1;
    if (conf.root().hasKey("dhcp4.ping-check")) {
        if (conf.root().hasKey("dhcp4.ping-check.enable")) {
            ping_enable = conf.root().getBool("dhcp4.ping-check.enable");
            if (ping_enable) {
                if (conf.root().hasKey("dhcp4.ping-check.timeout")) {
                    time_out = conf.root().getInt("dhcp4.ping-check.timeout");
                }
            }
        }
    }
    Pinger::init(ping_enable, time_out);
}

void 
initRpcAllocateEngine(const JsonConf& conf) {
    std::string kea_master_ip = DEFAULT_KEA_MASTER_IP;
    if (conf.root().hasKey("dhcp4.kea-master-ip")) {
            kea_master_ip = conf.root().getString("dhcp4.kea-master-ip");
    }
    int kea_master_port = DEFAULT_KEA_MASTER_PORT;
    if (conf.root().hasKey("dhcp4.kea-master-port")) {
            kea_master_port = conf.root().getInt("dhcp4.kea-master-port");
    }
    
    kea::rpc::RpcAllocateEngine::init(kea_master_ip, kea_master_port);
}

void 
initHooks(const JsonConf& conf) {
    if (conf.root().hasKey("dhcp4.hooks-libraries")) {
        HooksManager::instance().loadLibraries(conf.root().getObjects("dhcp4.hooks-libraries"));
    }
}

void 
initClientClasses(const JsonConf& conf) {
    ClientClassManager::instance().removeAll();
    if (conf.root().hasKey("dhcp4.client-classes")) {
        vector<JsonObject> client_classes = conf.root().getObjects("dhcp4.client-classes");
        for(auto& client_class : client_classes) {
            ClientClassManager::instance().addClientClass(client_class.getString("name"), 
                    client_class.getString("test"));
        }
    }
}

void 
initCustomOptions(const JsonConf& conf) {
    if (conf.root().hasKey("dhcp4.option-def")) {
        vector<JsonObject> option_defs = conf.root().getObjects("dhcp4.option-def");
        for (auto& option_def : option_defs) {
            const char* opt_name = (option_def.getString("name")).c_str();
            uint16_t opt_code = option_def.getInt("code");
            OptionDataType opt_type = OptionDataTypeUtil::getDataType(option_def.getString("type"));
            bool opt_array = false;
            OptionDefinition::RecordFieldsCollection opt_record_types {};
            if (option_def.hasKey("array")) {
                opt_array = option_def.getBool("array");
            }

            if (opt_type == OPT_RECORD_TYPE && option_def.hasKey("record-types")) {
                vector<string> record_types = str::tokens(option_def.getString("record-types"), ",");
                for (auto& record_type : record_types) {
                    OptionDataType data_type = OptionDataTypeUtil::getDataType(record_type);
                    opt_record_types.push_back(data_type);
                }
            }
            OptionDescription option_desc {opt_name, opt_code, opt_type, opt_array, opt_record_types, ""}; 
            getStdV4Options().addOptionDef(optionDefFromDescripton(option_desc));
        }
    }
}

void 
initOptionData(OptionCollection& opt_data_col, const JsonObject& conf) {
    vector<JsonObject> op_datas = conf.getObjects("option-data");
    for(auto& op_data : op_datas) {
        const OptionDefinition* def = nullptr;
        if (op_data.hasKey("code")) {
            int code = op_data.getInt("code");
            def = LibDHCP::getStdOptionDef(code);
        } else if (op_data.hasKey("name")) {
            string name = op_data.getString("name");
            def = LibDHCP::getStdOptionDef(name);
        }
        if (def == nullptr)
            return;

        string data = op_data.getString("data");		
        bool cvs_format = op_data.getBool("csv-format");

        unique_ptr<Option> opt;
        if (cvs_format) {
            std::vector<std::string> data_tokens = str::tokens(data, ",");   
            opt = def->optionFactory(def->getCode(), data_tokens);
        }
        else {
            std::vector<uint8_t> binary;
            util::encode::decodeHex(data, binary);
            opt = def->optionFactory(def->getCode(), binary);
        }

        opt_data_col.insert(make_pair(def->getCode(), std::move(opt)));
    }
}

Subnet*
createSubnet(const JsonObject& subnet) {
    if (subnet.hasKey("subnet")) {
        vector<string> subnet_str = str::tokens(subnet.getString("subnet"), "/");
        if (subnet_str.size() == 2) {
            IOAddress subnet_ip(subnet_str[0]);
            uint8_t mask_len = std::atoi(subnet_str[1].c_str());
            SubnetID subnetID = 0;
            if (subnet.hasKey("id")) {
                subnetID = SubnetID(subnet.getUint("id"));
            } else {
                kea_throw(BadValue, "ID of the new IPv4 subnet is no exists");
            }

            uint32_t maxValidTime = 0;
            uint32_t minValidTime = 0;
            uint32_t validLifeTime = 4800;

            if (subnet.hasKey("default-valid-lifetime")) {
                validLifeTime = subnet.getInt("default-valid-lifetime");
            }

            if (subnet.hasKey("max-valid-lifetime")) {
                maxValidTime = subnet.getInt("max-valid-lifetime");
            }

            if (subnet.hasKey("min-valid-lifetime")) {
                minValidTime = subnet.getInt("min-valid-lifetime"); 
            }

            if (maxValidTime < validLifeTime) {
                maxValidTime = validLifeTime;
            }

            if (minValidTime > validLifeTime) {
                minValidTime = validLifeTime;
            }

            std::unique_ptr<Triplet<uint32_t>> valid_time_trip = std::unique_ptr<Triplet<uint32_t>>
                (new Triplet<uint32_t>(minValidTime, validLifeTime, maxValidTime));

            Subnet* subnet4 = new Subnet(subnet_ip, mask_len, 4000, 4000, *valid_time_trip, subnetID);

            if (subnet.hasKey("white-client-class")) {
                vector<string> client_classes = subnet.getStrings("white-client-class");
                for (auto& client_class : client_classes) {
                    subnet4->allowClientClass(client_class);
                }
            }

            if (subnet.hasKey("black-client-class")) {
                vector<string> client_classes = subnet.getStrings("black-client-class");
                for (auto& client_class : client_classes) {
                    subnet4->denyClientClass(client_class);
                }
            }

            if (subnet.hasKey("option-data")) {
                initOptionData(subnet4->getOptdata(), subnet);
            }

            return subnet4;
        } else {
            logWarning("Dhcpv4Srv ", "The format of subnet should be address/mask");
        }
    } else {
        logWarning("Dhcpv4Srv ", "JsonObject no key subnet");
    }
    return nullptr;
}

std::unique_ptr<SubnetMgr> 
createSubnetMgr(const JsonConf& conf, BaseHostDataSource& host_mgr) {
    SubnetMgr* subnet_mgr = new SubnetMgr();
    vector<JsonObject> subnets = conf.root().getObjects("dhcp4.subnet4");

    for (auto& subnet : subnets) {
        auto subnet_ptr = createSubnet(subnet);
        if ( subnet_ptr != nullptr) { 
            if (!subnet_mgr->add(std::unique_ptr<Subnet>(subnet_ptr))) {
                kea_throw(DuplicateSubnetID, "ID of the new IPv4 subnet '" << subnet_ptr->getID() << "' is already in use");
            }
        }
    }

    return std::unique_ptr<SubnetMgr>(subnet_mgr);
}


void 
drainQueue(PktQueue& queue) {
    PktPtr query(nullptr);
    while (queue.isEmpty() == false) {
        queue.blockingRead(query);
    }
}

}
}
