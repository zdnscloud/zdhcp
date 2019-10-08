#include <kea/server/response_gen.h>
#include <kea/nic/iface_mgr.h>
#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_int.h>
#include <kea/dhcp++/option4_addrlst.h>
#include <kea/dhcp++/option_int_array.h>
#include <kea/util/ipaddress_extend.h>
using namespace std;
using namespace kea::nic;

namespace kea {
namespace server {
PktPtr initResponse(const Pkt& req) {
	uint8_t resp_type = 0;
	switch (req.getType()) {
	case DHCPDISCOVER:
		resp_type = DHCPOFFER;
		break;
	case DHCPREQUEST:
	case DHCPINFORM:
		resp_type = DHCPACK;
		break;
	default:
		assert(false);
	}

	PktPtr resp(new Pkt(resp_type, req.getTransid()));
	resp->setIface(req.getIface());
	resp->setIfaceIndex(req.getIfaceIndex());

	//copyDefaultFields
	resp->setSiaddr(IOAddress(0));
	resp->setCiaddr(IOAddress(0));
	resp->setHops(req.getHops());
	resp->setHWAddr(req.getHWAddr());
	resp->setGiaddr(req.getGiaddr());
	resp->setLocalHWAddr(req.getLocalHWAddr());
	resp->setRemoteHWAddr(req.getRemoteHWAddr());
	resp->setFlags(req.getFlags());

	//copyDefaultOptions();
	const Option* client_id = req.getOption(DHO_DHCP_CLIENT_IDENTIFIER);
	if (client_id) {
		resp->addOption(client_id->clone());
	}

	// If this packet is relayed, we want to copy Relay Agent Info option
	const Option* rai = req.getOption(DHO_DHCP_AGENT_OPTIONS);
	if (rai) {
		resp->addOption(rai->clone());
	}
	const Option* subnet_sel = req.getOption(DHO_SUBNET_SELECTION);
	if (subnet_sel) {
		resp->addOption(subnet_sel->clone());
	}
	return move(resp);
}

void appendBasicOptions(const Pkt& query, Pkt& resp, const Subnet& subnet) {
	resp.setSiaddr(subnet.getSiaddr());
	if (query.getType() != DHCPDISCOVER) {
		resp.setCiaddr(query.getCiaddr());
	}

    if (query.getType() != DHCPINFORM) {
        int valid_lft = subnet.getValid().get();
        const OptionInt<uint32_t>* opt_lease_time = dynamic_cast<const OptionInt<uint32_t>*>
            (query.getOption(DHO_DHCP_LEASE_TIME));
        if (opt_lease_time != nullptr) {
            valid_lft = subnet.getValid().get(opt_lease_time->getValue());
        }

        resp.addOption(unique_ptr<Option>(new OptionUint32(DHO_DHCP_LEASE_TIME, valid_lft)));

        if (!subnet.getT1().unspecified()) {
            resp.addOption(unique_ptr<Option>(new OptionUint32(DHO_DHCP_RENEWAL_TIME, valid_lft/2)));
        }

        if (!subnet.getT2().unspecified()) {
            resp.addOption(unique_ptr<Option>(new OptionUint32(DHO_DHCP_REBINDING_TIME, valid_lft*3/4)));
        }
    }

    uint32_t netmask = getNetmask4(subnet.get().second);
    resp.addOption(unique_ptr<Option>(new OptionInt<uint32_t>(DHO_SUBNET_MASK, netmask)));

	static const uint16_t required_options[] = {
		DHO_ROUTERS,
		DHO_DOMAIN_NAME_SERVERS,
		DHO_DOMAIN_NAME,
        DHO_VENDOR_CLASS_IDENTIFIER
	};

	static size_t required_options_size =
		sizeof(required_options) / sizeof(required_options[0]);

	const OptionCollection& opt_data = subnet.getOptdata();
	for (int i = 0; i < required_options_size; ++i) {
		if (!resp.getOption(required_options[i])) {
			auto data = opt_data.find(required_options[i]);
			if (data != opt_data.end()) {
				resp.addOption(data->second->clone());
			}
		}
	}
}

void appendRequestedOptions(const Pkt& query, Pkt& resp, const Subnet& subnet) {
	const OptionUint8Array* option_prl = dynamic_cast<const OptionUint8Array*>
		(query.getOption(DHO_DHCP_PARAMETER_REQUEST_LIST));
	if (option_prl == nullptr) {
		return;
	}

	const OptionCollection& opt_data = subnet.getOptdata();
	const vector<uint8_t>& requested_opts = option_prl->getValues();
	for (auto opt : requested_opts) {
		if (!resp.getOption(opt)) {
			auto data = opt_data.find(opt);
			if (data != opt_data.end())
			{
				data->second;
				resp.addOption(data->second->clone());
			}
		}
	}
}

void adjustRemoteAddr(const Pkt& query, Pkt& resp) {
	if (query.getType() == DHCPINFORM) {
		if (!query.getCiaddr().isV4Zero()) {
			resp.setRemoteAddr(query.getCiaddr());
		}
		else if (query.isRelayed()) {
			resp.setRemoteAddr(query.getGiaddr());
			resp.setFlags(resp.getFlags() | BOOTP_BROADCAST);
		}
		else {
			resp.setRemoteAddr(query.getRemoteAddr());
		}
		return;
	}

	if (query.isRelayed()) {
		if ((query.getType() == DHCPINFORM) && query.getCiaddr().isV4Zero()) {
			resp.setFlags(BOOTP_BROADCAST);
		}
		resp.setRemoteAddr(query.getGiaddr());
	}
	else if (!query.getCiaddr().isV4Zero()) {
		resp.setRemoteAddr(query.getCiaddr());
	}
	else if (resp.getType() == DHCPNAK) {
		resp.setRemoteAddr(IOAddress(0xFFFFFFFF));
	}
	else if (!resp.getYiaddr().isV4Zero()) {
		const bool bcast_flag = ((query.getFlags() & Pkt::FLAG_BROADCAST_MASK) != 0);
		if (!IfaceMgr::instance().isDirectResponseSupported() || bcast_flag) {
			resp.setRemoteAddr(IOAddress(0xFFFFFFFF));
		}
		else {
			resp.setRemoteAddr(resp.getYiaddr());
		}
	}
	else {
		resp.setRemoteAddr(query.getRemoteAddr());
	}
}

void appendIfaceData(const Pkt& query, Pkt& resp) {
	adjustRemoteAddr(query, resp);
	resp.setRemotePort(query.isRelayed() ? DHCP4_SERVER_PORT : DHCP4_CLIENT_PORT);
	//resp.setRemotePort(query.getRemotePort());
	IOAddress local_addr = query.getLocalAddr();
	if (local_addr.isV4Bcast()) {
		SocketInfo sock_info = IfaceMgr::instance().getSocket(query);
		local_addr = sock_info.addr_;
	}
	resp.setLocalAddr(local_addr);
	//response->setLocalPort(DHCP4_SERVER_PORT);
	resp.setLocalPort(query.getLocalPort());
	resp.setIface(query.getIface());
	resp.setIfaceIndex(query.getIfaceIndex());
	Option* opt_srvid = new Option4AddrLst(DHO_DHCP_SERVER_IDENTIFIER, resp.getLocalAddr());
	resp.addOption(unique_ptr<Option>(opt_srvid));
}

PktPtr genNakResponse(const Pkt& query) {
    PktPtr resp = initResponse(query);
    resp->setType(DHCPNAK);
    resp->setYiaddr(IOAddress(0));
    appendIfaceData(query, *resp);
    return std::move(resp);
}

PktPtr genAckResponse(const Pkt& query, IOAddress ip_addr, const Subnet& subnet) {
    PktPtr resp = initResponse(query);
    resp->setYiaddr(ip_addr);
    appendBasicOptions(query, *resp, subnet);
    appendRequestedOptions(query, *resp, subnet);
    appendIfaceData(query, *resp);
    return move(resp);
}

}
}
