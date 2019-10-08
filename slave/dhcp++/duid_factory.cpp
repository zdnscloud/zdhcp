#include <kea/dhcp++/duid_factory.h>
#include <kea/exceptions/exceptions.h>
#include <kea/util/io_utilities.h>
#include <kea/util/strutil.h>
#include <kea/dhcp++/dhcp4.h>
#include <ctime>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>

using namespace kea::util;
using namespace kea::util::str;

namespace kea {
namespace dhcp {

const size_t DUID_TYPE_LEN = 2;
const size_t MIN_MAC_LEN = 6;
const size_t ENTERPRISE_ID_LEN = 4;
const size_t DUID_EN_IDENTIFIER_LEN = 6;

DUIDFactory::DUIDFactory(const std::string& storage_location)
    : storage_location_(trim(storage_location)), duid_() {
}

bool DUIDFactory::isStored() const {
    return (!storage_location_.empty());
}

void DUIDFactory::createLLT(const uint16_t htype, const uint32_t time_in,
                       const std::vector<uint8_t>& ll_identifier) {
    readFromFile();

    uint16_t htype_current = 0;
    uint32_t time_current = 0;
    std::vector<uint8_t> identifier_current;

    if (duid_) {
        std::vector<uint8_t> duid_vec = duid_->getDuid();
        if ((duid_->getType() == DUID::DUID_LLT) && (duid_vec.size() > 8)) {
            htype_current = readUint16(&duid_vec[2], duid_vec.size() - 2);
            time_current = readUint32(&duid_vec[4], duid_vec.size() - 4);
            identifier_current.assign(duid_vec.begin() + 8, duid_vec.end());
        }
    }

    uint32_t time_out = time_in;
    if (time_out == 0) {
        time_out = (time_current != 0 ? time_current :
            static_cast<uint32_t>(time(NULL) - DUID_TIME_EPOCH));
    }

    std::vector<uint8_t> ll_identifier_out = ll_identifier;
    uint16_t htype_out = htype;

    if (ll_identifier_out.empty()) {
        if (identifier_current.empty()) {
            createLinkLayerId(ll_identifier_out, htype_out);
        } else {
            ll_identifier_out = identifier_current;
            htype_out = htype_current;
        }

    } else if (htype_out == 0) {
        htype_out = ((htype_current != 0) ? htype_current :
                     static_cast<uint16_t>(HTYPE_ETHER));
    }

    // Render DUID.
    std::vector<uint8_t> duid_out(DUID_TYPE_LEN + sizeof(time_out) +
                                  sizeof(htype_out));
    writeUint16(DUID::DUID_LLT, &duid_out[0], 2);
    writeUint16(htype_out, &duid_out[2], 2);
    writeUint32(time_out, &duid_out[4], 4);
    duid_out.insert(duid_out.end(), ll_identifier_out.begin(),
                    ll_identifier_out.end());

    // Set new DUID and persist in a file.
    set(duid_out);
}

void DUIDFactory::createEN(const uint32_t enterprise_id,
                      const std::vector<uint8_t>& identifier) {
    readFromFile();

    uint32_t enterprise_id_current = 0;
    std::vector<uint8_t> identifier_current;

    if (duid_) {
        std::vector<uint8_t> duid_vec = duid_->getDuid();
        if ((duid_->getType() == DUID::DUID_EN) && (duid_vec.size() > 6)) {
            enterprise_id_current = readUint32(&duid_vec[2], duid_vec.size() - 2);
            identifier_current.assign(duid_vec.begin() + 6, duid_vec.end());
        }
    }

    uint32_t enterprise_id_out = enterprise_id;
    if (enterprise_id_out == 0) {
        if (enterprise_id_current != 0) {
            enterprise_id_out = enterprise_id_current;
        } else {
            enterprise_id_out = ENTERPRISE_ID_ISC;
        }
    }

    // Render DUID.
    std::vector<uint8_t> duid_out(DUID_TYPE_LEN + ENTERPRISE_ID_LEN);
    writeUint16(DUID::DUID_EN, &duid_out[0], 2);
    writeUint32(enterprise_id_out, &duid_out[2], ENTERPRISE_ID_LEN);

    if (identifier.empty()) {
        if (identifier_current.empty()) {
            duid_out.resize(DUID_TYPE_LEN + ENTERPRISE_ID_LEN +
                            DUID_EN_IDENTIFIER_LEN);
            ::srandom(time(NULL));
            uint8_t *ep_ptr = &duid_out[DUID_TYPE_LEN + ENTERPRISE_ID_LEN];
            for (int i = 0; i < DUID_EN_IDENTIFIER_LEN; i++) {
                *ep_ptr = random();
                ep_ptr += 1;
            }
        } else {
            duid_out.insert(duid_out.end(), identifier_current.begin(),
                            identifier_current.end());
        }

    } else {
        duid_out.insert(duid_out.end(), identifier.begin(), identifier.end());
    }

    set(duid_out);
}

void DUIDFactory::createLL(const uint16_t htype,
                      const std::vector<uint8_t>& ll_identifier) {
    readFromFile();

    uint16_t htype_current = 0;
    std::vector<uint8_t> identifier_current;

    if (duid_) {
        std::vector<uint8_t> duid_vec = duid_->getDuid();
        if ((duid_->getType() == DUID::DUID_LL) && (duid_vec.size() > 4)) {
            htype_current = readUint16(&duid_vec[2], duid_vec.size() - 2);
            identifier_current.assign(duid_vec.begin() + 4, duid_vec.end());
        }
    }

    std::vector<uint8_t> ll_identifier_out = ll_identifier;
    uint16_t htype_out = htype;

    if (ll_identifier_out.empty()) {
        if (identifier_current.empty()) {
            createLinkLayerId(ll_identifier_out, htype_out);
        } else {
            ll_identifier_out = identifier_current;
            htype_out = htype_current;
        }

    } else if (htype_out == 0) {
        htype_out = ((htype_current != 0) ? htype_current :
            static_cast<uint16_t>(HTYPE_ETHER));

    }

    std::vector<uint8_t> duid_out(DUID_TYPE_LEN + sizeof(htype_out));
    writeUint16(DUID::DUID_LL, &duid_out[0], 2);
    writeUint16(htype_out, &duid_out[2], 2);
    duid_out.insert(duid_out.end(), ll_identifier_out.begin(),
                    ll_identifier_out.end());

    set(duid_out);
}

void DUIDFactory::createLinkLayerId(std::vector<uint8_t>& identifier,
                               uint16_t& htype) const {
    /*
    const IfaceMgr::IfaceCollection& ifaces = IfaceMgr::instance().getIfaces();

    BOOST_FOREACH(IfacePtr iface, ifaces) {
        if (iface->getMacLen() < MIN_MAC_LEN) {
            continue;
        }

        if (iface->flag_loopback_) {
            continue;
        }

        if (!iface->flag_up_) {
            continue;
        }

        if (isRangeZero(iface->getMac(), iface->getMac() + iface->getMacLen())) {
            continue;
        }

        identifier.assign(iface->getMac(), iface->getMac() + iface->getMacLen());
        htype = iface->getHWType();
    }
    */

    (void)htype;
    if (identifier.empty()) {
        kea_throw(Unexpected, "unable to find suitable interface for "
                  " generating a DUID-LLT");
    }
}

void DUIDFactory::set(const std::vector<uint8_t>& duid_vector) {
    if (duid_vector.size() < DUID::MIN_DUID_LEN) {
        kea_throw(BadValue, "generated DUID must have at least "
                  << DUID::MIN_DUID_LEN << " bytes");
    }

    if (isStored()) {
        std::ofstream ofs;
        try {
            ofs.open(storage_location_.c_str(), std::ofstream::out |
                     std::ofstream::trunc);
            if (!ofs.good()) {
                kea_throw(InvalidOperation, "unable to open DUID file "
                          << storage_location_ << " for writing");
            }

            DUID duid(duid_vector);

            ofs << duid.toText();
            if (!ofs.good()) {
                kea_throw(InvalidOperation, "unable to write to DUID file "
                          << storage_location_);
            }
        } catch (...) {
            ofs.close();
            throw;
        }
        ofs.close();
    }

    duid_.reset(new DUID(duid_vector));
}

std::shared_ptr<DUID> DUIDFactory::get() {
    if (duid_) {
        return (duid_);
    }

    readFromFile();
    if (duid_) {
        return (duid_);
    }

    const std::vector<uint8_t> empty_vector;
    try {
        createLLT(0, 0, empty_vector);
    } catch (...) {
    }

    if (!duid_) {
        createEN(0, empty_vector);
    }

    return (duid_);
}

void DUIDFactory::readFromFile() {
    duid_.reset();

    std::ostringstream duid_str;
   if (isStored()) {
        std::ifstream ifs;
        ifs.open(storage_location_.c_str(), std::ifstream::in);
        if (ifs.good()) {
            std::string read_contents;
            while (!ifs.eof() && ifs.good()) {
                ifs >> read_contents;
                duid_str << read_contents;
            }
        }
        ifs.close();

        if (duid_str.tellp() != std::streampos(0)) {
            try {
                duid_.reset(new DUID(DUID::fromText(duid_str.str())));

            } catch (...) {
            }
        }
   }
}

}; 
};
