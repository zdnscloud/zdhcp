// Copyright (C) 2011-2016 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/vendor_option_defs.h>
#include <kea/dhcp++/libdhcp++.h>
#include <kea/dhcp++/option4_addrlst.h>
#include <kea/dhcp++/option4_client_fqdn.h>
#include <kea/dhcp++/option_custom.h>
#include <kea/dhcp++/option_int.h>
#include <kea/dhcp++/option_int_array.h>
#include <kea/dhcp++/option_opaque_data_tuples.h>
#include <kea/dhcp++/option_space.h>
#include <kea/dhcp++/option_string.h>
#include <kea/dhcp++/option_vendor.h>
#include <kea/dhcp++/option_vendor_class.h>
#include <kea/util/buffer.h>
#include <kea/util/encode/hex.h>

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

#include <iostream>
#include <sstream>
#include <typeinfo>

#include <arpa/inet.h>

using namespace std;
using namespace folly;
using namespace kea;
using namespace kea::dhcp;
using namespace kea::util;

namespace {


class LibDhcpTest : public ::testing::Test {
public:
    LibDhcpTest() {
        LibDHCP::initOptions();
        LibDHCP::clearRuntimeOptionDefs();
    }

    virtual ~LibDhcpTest() {
        LibDHCP::clearRuntimeOptionDefs();
    }

    static std::unique_ptr<Option> genericOptionFactory(uint16_t type,
                                          const OptionBuffer& buf) {
        return (std::unique_ptr<Option>(new Option(type, buf)));
    }

    static void testStdOptionDefs4(const uint16_t code,
                                   const OptionBufferConstIter begin,
                                   const OptionBufferConstIter end,
                                   const std::type_info& expected_type,
                                   const std::string& encapsulates = "") {
        // Use V4 universe.
        testStdOptionDefs(code, begin, end, expected_type,
                          encapsulates);
    }


    /// @brief Create a sample DHCPv4 option 43 with suboptions.
    static OptionBuffer createVendorOption() {
        const uint8_t opt_data[] = {
            0x2B, 0x0D,  // Vendor-Specific Information (CableLabs)
            // Suboptions start here...
            0x02, 0x05,  // Device Type Option (length = 5)
            'D', 'u', 'm', 'm', 'y',
            0x04, 0x04,   // Serial Number Option (length = 4)
            0x42, 0x52, 0x32, 0x32 // Serial number
        };
        return (OptionBuffer(opt_data, opt_data + sizeof(opt_data)));
    }

    /// @brief Create a sample DHCPv4 option 82 with suboptions.
    static OptionBuffer createAgentInformationOption() {
        const uint8_t opt_data[] = {
            0x52, 0x0E, // Agent Information Option (length = 14)
            // Suboptions start here...
            0x01, 0x04, // Agent Circuit ID (length = 4)
            0x20, 0x00, 0x00, 0x02, // ID
            0x02, 0x06, // Agent Remote ID
            0x20, 0xE5, 0x2A, 0xB8, 0x15, 0x14 // ID
        };
        return (OptionBuffer(opt_data, opt_data + sizeof(opt_data)));
    }

    /*
    static void createRuntimeOptionDefs(const uint16_t spaces_num,
                                        const uint16_t defs_num,
                                        OptionDefSpaceContainer& defs) {
        for (uint16_t space = 0; space < spaces_num; ++space) {
            std::ostringstream space_name;
            space_name << "option-space-" << space;
            for (uint16_t code = 0; code < defs_num; ++code) {
                std::ostringstream name;
                name << "name-for-option-" << code;
                OptionDefinitionPtr opt_def(new OptionDefinition(name.str(),
                                                                 code, "string"));
                defs.addItem(opt_def, space_name.str());
            }
        }
    }

    static void testRuntimeOptionDefs(const uint16_t spaces_num,
                                      const uint16_t defs_num,
                                      const bool should_exist) {
        for (uint16_t space = 0; space < spaces_num; ++space) {
            std::ostringstream space_name;
            space_name << "option-space-" << space;
            for (uint16_t code = 0; code < defs_num; ++code) {
                std::ostringstream name;
                name << "name-for-option-" << code;
                OptionDefinitionPtr opt_def =
                    LibDHCP::getRuntimeOptionDef(space_name.str(), name.str());
                if (should_exist) {
                    ASSERT_TRUE(opt_def);
                } else {
                    ASSERT_FALSE(opt_def);
                }
            }
        }
    }
    */


private:

    static void testStdOptionDefs(const uint16_t code,
                                  const OptionBufferConstIter begin,
                                  const OptionBufferConstIter end,
                                  const std::type_info& expected_type,
                                  const std::string& encapsulates) {
        const OptionDefinition* def = LibDHCP::getStdOptionDef(code);
        ASSERT_TRUE(def != nullptr) << "Option definition for the code "
                         << code << " is NULL.";
        // Check that option definition is valid.
        ASSERT_NO_THROW(def->validate())
            << "Option definition for the option code " << code
            << " is invalid";
        // Check that the valid encapsulated option space name
        // has been specified.
        EXPECT_EQ(encapsulates, def->getEncapsulatedSpace());
        std::unique_ptr<Option> option;
        // Create the option.
        ASSERT_NO_THROW(option = def->optionFactory(code, begin, end))
            << "Option creation failed for option code " << code;
        // And the actual object type is the one that we expect.
        // Note that for many options there are dedicated classes
        // derived from Option class to represent them.
        const Option* optptr = option.get();
        EXPECT_TRUE(typeid(*optptr) == expected_type)
            << "Invalid class returned for option code " << code;
    }
};


TEST_F(LibDhcpTest, optionFactory) {
    OptionBuffer buf;
    // Factory functions for specific options must be registered before
    // they can be used to create options instances. Otherwise exception
    // is rised.
    EXPECT_THROW(LibDHCP::optionFactory(DHO_SUBNET_MASK, buf),
                 kea::BadValue);

    // Let's register some factory functions (two v4 function).
    // Registration may trigger exception if function for the specified
    // option has been registered already.
    ASSERT_NO_THROW(
        LibDHCP::OptionFactoryRegister(DHO_SUBNET_MASK,
                                       &LibDhcpTest::genericOptionFactory);
    );
    ASSERT_NO_THROW(
        LibDHCP::OptionFactoryRegister(DHO_TIME_OFFSET,
                                       &LibDhcpTest::genericOptionFactory);
    );

    // Invoke factory functions for all options (check if registration
    // was successful).
    std::unique_ptr<Option> opt_subnet_mask;
    opt_subnet_mask = LibDHCP::optionFactory(DHO_SUBNET_MASK,
                                             buf);
    // Validate if type and universe is correct.
    EXPECT_EQ(DHO_SUBNET_MASK, opt_subnet_mask->getType());
    // Expect that option does not have content..
    EXPECT_EQ(0, opt_subnet_mask->len() - opt_subnet_mask->getHeaderLen());

    // Fill the time offset buffer with 4 bytes of data. Each byte set to 1.
    OptionBuffer time_offset_buf(4, 1);
    std::unique_ptr<Option> opt_time_offset;
    opt_time_offset = LibDHCP::optionFactory(DHO_TIME_OFFSET,
                                             time_offset_buf);
    // Validate if option length, type and universe is correct.
    EXPECT_EQ(DHO_TIME_OFFSET, opt_time_offset->getType());
    EXPECT_EQ(time_offset_buf.size(),
              opt_time_offset->len() - opt_time_offset->getHeaderLen());
    // Validate data in the option.
    EXPECT_TRUE(std::equal(time_offset_buf.begin(), time_offset_buf.end(),
                           opt_time_offset->getData().begin()));

}


/// V4 Options being used to test pack/unpack operations.
/// These are variable length options only so as there
/// is no restriction on the data length being carried by them.
/// For simplicity, we assign data of the length 3 for each
/// of them.
static uint8_t v4_opts[] = {
    12,  3, 0,   1,  2,        // Hostname
    60,  3, 10, 11, 12,        // Class Id
    14,  3, 20, 21, 22,        // Merit Dump File
    254, 3, 30, 31, 32,        // Reserved
    128, 3, 40, 41, 42,        // Vendor specific
    125, 11, 0, 0, 0x11, 0x8B, // V-I Vendor-Specific Information (Cable Labs)
    6, 2, 4, 10, 0, 0, 10,     // TFTP servers suboption (2)
    43, 2,                     // Vendor Specific Information
    0xDC, 0,                   // VSI suboption
    0x52, 0x19,                // RAI
    0x01, 0x04, 0x20, 0x00, 0x00, 0x02, // Agent Circuit ID
    0x02, 0x06, 0x20, 0xE5, 0x2A, 0xB8, 0x15, 0x14, // Agent Remote ID
    0x09, 0x09, 0x00, 0x00, 0x11, 0x8B, 0x04, // Vendor Specific Information
    0x01, 0x02, 0x03, 0x00 // Vendor Specific Information continued
};

// This test verifies that pack options for v4 is working correctly.
TEST_F(LibDhcpTest, packOptions4) {

    vector<uint8_t> payload[5];
    for (unsigned i = 0; i < 5; i++) {
        payload[i].resize(3);
        payload[i][0] = i*10;
        payload[i][1] = i*10+1;
        payload[i][2] = i*10+2;
    }

    std::unique_ptr<Option> opt1(new Option(12, payload[0]));
    std::unique_ptr<Option> opt2(new Option(60, payload[1]));
    std::unique_ptr<Option> opt3(new Option(14, payload[2]));
    std::unique_ptr<Option> opt4(new Option(254, payload[3]));
    std::unique_ptr<Option> opt5(new Option(128, payload[4]));

    // Create vendor option instance with DOCSIS3.0 enterprise id.
    std::unique_ptr<Option> vivsi(new OptionVendor(4491));
    vivsi->addOption(std::unique_ptr<Option>(new Option4AddrLst(DOCSIS3_V4_TFTP_SERVERS,
                                                  IPAddress("10.0.0.10"))));

    std::unique_ptr<Option> vsi(new Option(DHO_VENDOR_ENCAPSULATED_OPTIONS,
                              OptionBuffer()));
    vsi->addOption(std::unique_ptr<Option>(new Option(0xDC, OptionBuffer())));

    // Add RAI option, which comprises 3 sub-options.

    // Get the option definition for RAI option. This option is represented
    // by OptionCustom which requires a definition to be passed to
    // the constructor.
    const OptionDefinition* rai_def = LibDHCP::getStdOptionDef(DHO_DHCP_AGENT_OPTIONS);
    ASSERT_TRUE(rai_def != nullptr);
    // Create RAI option.
    std::unique_ptr<Option> rai(new OptionCustom(*rai_def));

    // The sub-options are created using the bits of v4_opts buffer because
    // we want to use this buffer as a reference to verify that produced
    // option in on-wire format is correct.

    // Create Ciruit ID sub-option and add to RAI.
    std::unique_ptr<Option> circuit_id(new Option(RAI_OPTION_AGENT_CIRCUIT_ID,
                                    OptionBuffer(v4_opts + 46,
                                                 v4_opts + 50)));
    rai->addOption(std::move(circuit_id));

    // Create Remote ID option and add to RAI.
    std::unique_ptr<Option> remote_id(new Option(RAI_OPTION_REMOTE_ID,
                                   OptionBuffer(v4_opts + 52, v4_opts + 58)));
    rai->addOption(std::move(remote_id));

    // Create Vendor Specific Information and add to RAI.
    std::unique_ptr<Option> rai_vsi(new Option(RAI_OPTION_VSI,
                                 OptionBuffer(v4_opts + 60, v4_opts + 69)));
    rai->addOption(std::move(rai_vsi));

    kea::dhcp::OptionCollection opts; // list of options
    // Note that we insert each option under the same option code into
    // the map. This gurantees that options are packed in the same order
    // they were added. Otherwise, options would get sorted by code and
    // the resulting buffer wouldn't match with the reference buffer.
    auto type = opt1->getType();
    opts.insert(make_pair(type, std::move(opt1)));
    opts.insert(make_pair(type, std::move(opt2)));
    opts.insert(make_pair(type, std::move(opt3)));
    opts.insert(make_pair(type, std::move(opt4)));
    opts.insert(make_pair(type, std::move(opt5)));
    opts.insert(make_pair(type, std::move(vivsi)));
    opts.insert(make_pair(type, std::move(vsi)));
    opts.insert(make_pair(type, std::move(rai)));

    OutputBuffer buf(100);
    EXPECT_NO_THROW(LibDHCP::packOptions4(buf, opts));
    ASSERT_EQ(buf.getLength(), sizeof(v4_opts));
    EXPECT_EQ(0, memcmp(v4_opts, buf.getData(), sizeof(v4_opts)));
}

// This test verifies that pack options for v4 is working correctly
// and RAI option is packed last.
TEST_F(LibDhcpTest, packOptions4Order) {
    uint8_t expected[] = {
        12,  3, 0,   1,  2, // Just a random option
        99,  3, 10, 11, 12, // Another random option
        82,  3, 20, 21, 22 // Relay Agent Info option
    };

    vector<uint8_t> payload[3];
    for (unsigned i = 0; i < 3; i++) {
        payload[i].resize(3);
        payload[i][0] = i*10;
        payload[i][1] = i*10+1;
        payload[i][2] = i*10+2;
    }

    std::unique_ptr<Option> opt12(new Option(12, payload[0]));
    std::unique_ptr<Option> opt99(new Option(99, payload[1]));
    std::unique_ptr<Option> opt82(new Option(82, payload[2]));

    // Let's create options. They are added in 82,12,99, but the should be
    // packed in 12,99,82 order (82, which is RAI, should go last)
    kea::dhcp::OptionCollection opts;
    opts.insert(make_pair(opt82->getType(), std::move(opt82)));
    opts.insert(make_pair(opt12->getType(), std::move(opt12)));
    opts.insert(make_pair(opt99->getType(), std::move(opt99)));

    OutputBuffer buf(100);
    EXPECT_NO_THROW(LibDHCP::packOptions4(buf, opts));
    ASSERT_EQ(buf.getLength(), sizeof(expected));
    EXPECT_EQ(0, memcmp(expected, buf.getData(), sizeof(expected)));
}

TEST_F(LibDhcpTest, unpackOptions4) {

    vector<uint8_t> v4packed(v4_opts, v4_opts + sizeof(v4_opts));
    kea::dhcp::OptionCollection options; // list of options

    ASSERT_NO_THROW(
        LibDHCP::unpackOptions4(v4packed, "dhcp4", options);
    );

    kea::dhcp::OptionCollection::const_iterator x = options.find(12);
    ASSERT_FALSE(x == options.end()); // option 1 should exist
    // Option 12 holds a string so let's cast it to an appropriate type.
    const OptionString *option12 = dynamic_cast<const OptionString *>(x->second.get());
    ASSERT_TRUE(option12 != nullptr);
    EXPECT_EQ(12, option12->getType());  // this should be option 12
    ASSERT_EQ(3, option12->getValue().length()); // it should be of length 3
    EXPECT_EQ(5, option12->len()); // total option length 5
    EXPECT_EQ(0, memcmp(&option12->getValue()[0], v4_opts + 2, 3)); // data len=3

    x = options.find(60);
    ASSERT_FALSE(x == options.end()); // option 2 should exist
    EXPECT_EQ(60, x->second->getType());  // this should be option 60
    ASSERT_EQ(3, x->second->getData().size()); // it should be of length 3
    EXPECT_EQ(5, x->second->len()); // total option length 5
    EXPECT_EQ(0, memcmp(&x->second->getData()[0], v4_opts + 7, 3)); // data len=3

    x = options.find(14);
    ASSERT_FALSE(x == options.end()); // option 3 should exist
    const OptionString *option14 = dynamic_cast<const OptionString *>(x->second.get());
    ASSERT_TRUE(option14 != nullptr);
    EXPECT_EQ(14, option14->getType());  // this should be option 14
    ASSERT_EQ(3, option14->getValue().length()); // it should be of length 3
    EXPECT_EQ(5, option14->len()); // total option length 5
    EXPECT_EQ(0, memcmp(&option14->getValue()[0], v4_opts + 12, 3)); // data len=3

    x = options.find(254);
    ASSERT_FALSE(x == options.end()); // option 4 should exist
    EXPECT_EQ(254, x->second->getType());  // this should be option 254
    ASSERT_EQ(3, x->second->getData().size()); // it should be of length 3
    EXPECT_EQ(5, x->second->len()); // total option length 5
    EXPECT_EQ(0, memcmp(&x->second->getData()[0], v4_opts + 17, 3)); // data len=3

    x = options.find(128);
    ASSERT_FALSE(x == options.end()); // option 5 should exist
    EXPECT_EQ(128, x->second->getType());  // this should be option 128
    ASSERT_EQ(3, x->second->getData().size()); // it should be of length 3
    EXPECT_EQ(5, x->second->len()); // total option length 5
    EXPECT_EQ(0, memcmp(&x->second->getData()[0], v4_opts + 22, 3)); // data len=3

    // Verify that V-I Vendor Specific Information option is parsed correctly.
    x = options.find(125);
    ASSERT_FALSE(x == options.end());
    const OptionVendor *vivsi = dynamic_cast<const OptionVendor*>(x->second.get());
    ASSERT_TRUE(vivsi != nullptr);
    EXPECT_EQ(DHO_VIVSO_SUBOPTIONS, vivsi->getType());
    EXPECT_EQ(4491, vivsi->getVendorId());
    const OptionCollection &suboptions = vivsi->getOptions();

    // There should be one suboption of V-I VSI.
    ASSERT_EQ(1, suboptions.size());
    // This vendor option has a standard definition and thus should be
    // converted to appropriate class, i.e. Option4AddrLst. If this cast
    // fails, it means that its definition was not used while it was
    // parsed.
    const Option4AddrLst* tftp = dynamic_cast<Option4AddrLst*>(suboptions.begin()->second.get());
    ASSERT_TRUE(tftp != nullptr);
    EXPECT_EQ(DOCSIS3_V4_TFTP_SERVERS, tftp->getType());
    EXPECT_EQ(6, tftp->len());
    Option4AddrLst::AddressContainer addresses = tftp->getAddresses();
    ASSERT_EQ(1, addresses.size());
    EXPECT_EQ("10.0.0.10", addresses[0].str());

    // Vendor Specific Information option
    x = options.find(43);
    ASSERT_FALSE(x == options.end());
    EXPECT_EQ(DHO_VENDOR_ENCAPSULATED_OPTIONS, x->second->getType());
    const OptionCollection &suboptions1 = x->second->getOptions();

    // There should be one suboption of VSI.
    ASSERT_EQ(1, suboptions1.size());
    const Option* eso = suboptions1.begin()->second.get();
    ASSERT_TRUE(eso != nullptr);
    EXPECT_EQ(0xdc, eso->getType());
    EXPECT_EQ(2, eso->len());

    // Checking DHCP Relay Agent Information Option.
    x = options.find(DHO_DHCP_AGENT_OPTIONS);
    ASSERT_FALSE(x == options.end());
    EXPECT_EQ(DHO_DHCP_AGENT_OPTIONS, x->second->getType());
    // RAI is represented by OptionCustom.
    const OptionCustom * rai = dynamic_cast<OptionCustom *>(x->second.get());
    ASSERT_TRUE(rai != nullptr);
    // RAI should have 3 sub-options: Circuit ID, Agent Remote ID, Vendor
    // Specific Information option. Note that by parsing these suboptions we
    // are checking that unpackOptions4 differentiates between standard option
    // space called "dhcp4" and other option spaces. These sub-options do not
    // belong to standard option space and should be parsed using different
    // option definitions.
    // @todo Currently, definitions for option space "dhcp-agent-options-space"
    // are not defined. Therefore all suboptions will be represented here by
    // the generic Option class.

    // Check that Circuit ID option is among parsed options.
    const Option* rai_option = rai->getOption(RAI_OPTION_AGENT_CIRCUIT_ID);
    ASSERT_TRUE(rai_option != nullptr);
    EXPECT_EQ(RAI_OPTION_AGENT_CIRCUIT_ID, rai_option->getType());
    ASSERT_EQ(6, rai_option->len());
    EXPECT_EQ(0, memcmp(&rai_option->getData()[0], v4_opts + 46, 4));

    // Check that Remote ID option is among parsed options.
    rai_option = rai->getOption(RAI_OPTION_REMOTE_ID);
    ASSERT_TRUE(rai_option != nullptr);
    EXPECT_EQ(RAI_OPTION_REMOTE_ID, rai_option->getType());
    ASSERT_EQ(8, rai_option->len());
    EXPECT_EQ(0, memcmp(&rai_option->getData()[0], v4_opts + 52, 6));

    // Check that Vendor Specific Information option is among parsed options.
    rai_option = rai->getOption(RAI_OPTION_VSI);
    ASSERT_TRUE(rai_option != nullptr);
    EXPECT_EQ(RAI_OPTION_VSI, rai_option->getType());
    ASSERT_EQ(11, rai_option->len());
    EXPECT_EQ(0, memcmp(&rai_option->getData()[0], v4_opts + 60, 9));

    // Make sure, that option other than those above is not present.
    EXPECT_EQ(rai->getOption(10), nullptr);

    // Check the same for the global option space.
    x = options.find(0);
    EXPECT_TRUE(x == options.end()); // option 0 not found

    x = options.find(1);
    EXPECT_TRUE(x == options.end()); // option 1 not found

    x = options.find(2);
    EXPECT_TRUE(x == options.end()); // option 2 not found

}

// Check parsing of an empty option.
TEST_F(LibDhcpTest, unpackEmptyOption4) {
    // Create option definition for the option code 254 without fields.
    LibDHCP::addRuntimeOptionDef(DHCP4_OPTION_SPACE, std::unique_ptr<OptionDefinition>(new OptionDefinition("option-empty", 254, "empty", false)));

    // Create the buffer holding the structure of the empty option.
    const uint8_t raw_data[] = {
      0xFE,                     // option code = 254
      0x00                      // option length = 0
    };
    size_t raw_data_len = sizeof(raw_data) / sizeof(uint8_t);
    OptionBuffer buf(raw_data, raw_data + raw_data_len);

    // Parse options.
    OptionCollection options;
    ASSERT_NO_THROW(LibDHCP::unpackOptions4(buf, DHCP4_OPTION_SPACE,
                                            options));

    // There should be one option.
    ASSERT_EQ(1, options.size());
    const Option* option_empty = options.begin()->second.get();
    EXPECT_EQ(254, option_empty->getType());
    EXPECT_EQ(2, option_empty->len());
}

// This test verifies that the following option structure can be parsed:
// - option (option space 'foobar')
//   - sub option (option space 'foo')
//      - sub option (option space 'bar')
// @todo Add more thorough unit tests for unpackOptions.
TEST_F(LibDhcpTest, unpackSubOptions4) {
    // Create option definition for each level of encapsulation. Each option
    // definition is for the option code 1. Options may have the same
    // option code because they belong to different option spaces.

    // Top level option encapsulates options which belong to 'space-foo'.
    LibDHCP::addRuntimeOptionDef("space-foobar", std::unique_ptr<OptionDefinition>(new OptionDefinition("option-foobar", 1, "uint32", "space-foo")));
    LibDHCP::addRuntimeOptionDef("space-foo", std::unique_ptr<OptionDefinition>(new OptionDefinition("option-foo", 1, "uint16", "space-bar")));
    LibDHCP::addRuntimeOptionDef("space-bar", std::unique_ptr<OptionDefinition>(new OptionDefinition("option-bar", 1, "uint8")));

    // Create the buffer holding the structure of options.
    const uint8_t raw_data[] = {
        // First option starts here.
        0x01,                   // option code = 1
        0x0B,                   // option length = 11
        0x00, 0x01, 0x02, 0x03, // This option carries uint32 value
        // Sub option starts here.
        0x01,                   // option code = 1
        0x05,                   // option length = 5
        0x01, 0x02,             // this option carries uint16 value
        // Last option starts here.
        0x01,                   // option code = 1
        0x01,                   // option length = 1
        0x00                    // This option carries a single uint8
                                // value and has no sub options.
    };
    size_t raw_data_len = sizeof(raw_data) / sizeof(uint8_t);
    OptionBuffer buf(raw_data, raw_data + raw_data_len);

    // Parse options.
    OptionCollection options;
    ASSERT_NO_THROW(LibDHCP::unpackOptions4(buf, "space-foobar", options));

    // There should be one top level option.
    ASSERT_EQ(1, options.size());
    const OptionInt<uint32_t>* option_foobar = dynamic_cast<const OptionInt<uint32_t>*>(options.begin()->second.get());
    ASSERT_TRUE(option_foobar != nullptr);
    EXPECT_EQ(1, option_foobar->getType());
    EXPECT_EQ(0x00010203, option_foobar->getValue());
    // There should be a middle level option held in option_foobar.
    const OptionInt<uint16_t>* option_foo = dynamic_cast<const OptionInt<uint16_t>*>(option_foobar->getOption(1));
    ASSERT_TRUE(option_foo != nullptr);
    EXPECT_EQ(1, option_foo->getType());
    EXPECT_EQ(0x0102, option_foo->getValue());
    // Finally, there should be a low level option under option_foo.
    const OptionInt<uint8_t>* option_bar = dynamic_cast<const OptionInt<uint8_t>*>(option_foo->getOption(1));
    ASSERT_TRUE(option_bar != nullptr);
    EXPECT_EQ(1, option_bar->getType());
    EXPECT_EQ(0x0, option_bar->getValue());
}

TEST_F(LibDhcpTest, isStandardOption4) {
    // Get all option codes that are not occupied by standard options.
    const uint16_t unassigned_codes[] = { 84, 96, 102, 103, 104, 105, 106, 107, 108,
                                          109, 110, 111, 115, 126, 127, 147, 148, 149,
                                          178, 179, 180, 181, 182, 183, 184, 185, 186,
                                          187, 188, 189, 190, 191, 192, 193, 194, 195,
                                          196, 197, 198, 199, 200, 201, 202, 203, 204,
                                          205, 206, 207, 214, 215, 216, 217, 218, 219,
                                          222, 223, 224, 225, 226, 227, 228, 229, 230,
                                          231, 232, 233, 234, 235, 236, 237, 238, 239,
                                          240, 241, 242, 243, 244, 245, 246, 247, 248,
                                          249, 250, 251, 252, 253, 254 };
    const size_t unassigned_num = sizeof(unassigned_codes) / sizeof(unassigned_codes[0]);

    // Try all possible option codes.
    for (size_t i = 0; i < 256; ++i) {
        // Some ranges of option codes are unassigned and thus the isStandardOption
        // should return false for them.
        bool check_unassigned = false;
        // Check the array of unassigned options to find out whether option code
        // is assigned to standard option or unassigned.
        for (size_t j = 0; j < unassigned_num; ++j) {
            // If option code is found within the array of unassigned options
            // we the isStandardOption function should return false.
            if (unassigned_codes[j] == i) {
                check_unassigned = true;
                EXPECT_FALSE(LibDHCP::isStandardOption(unassigned_codes[j]))
                    << "Test failed for option code " << unassigned_codes[j];
                break;
            }
        }
        // If the option code belongs to the standard option then the
        // isStandardOption should return true.
        if (!check_unassigned) {
            EXPECT_TRUE(LibDHCP::isStandardOption(i))
                << "Test failed for the option code " << i;
        }
    }
}


TEST_F(LibDhcpTest, stdOptionDefs4) {

    // Create a buffer that holds dummy option data.
    // It will be used to create most of the options.
    std::vector<uint8_t> buf(48, 1);
    OptionBufferConstIter begin = buf.begin();
    OptionBufferConstIter end = buf.end();

    LibDhcpTest::testStdOptionDefs4(DHO_SUBNET_MASK, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_TIME_OFFSET, begin, begin + 4,
                                    typeid(OptionInt<int32_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_ROUTERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_TIME_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_NAME_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_DOMAIN_NAME_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_LOG_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_COOKIE_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_LPR_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_IMPRESS_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_RESOURCE_LOCATION_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_HOST_NAME, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_BOOT_SIZE, begin, begin + 2,
                                    typeid(OptionInt<uint16_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_MERIT_DUMP, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_DOMAIN_NAME, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_SWAP_SERVER, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_ROOT_PATH, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_EXTENSIONS_PATH, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_IP_FORWARDING, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_NON_LOCAL_SOURCE_ROUTING, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_POLICY_FILTER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_MAX_DGRAM_REASSEMBLY, begin, begin + 2,
                                    typeid(OptionInt<uint16_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_DEFAULT_IP_TTL, begin, begin + 1,
                                    typeid(OptionInt<uint8_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_PATH_MTU_AGING_TIMEOUT, begin, begin + 4,
                                    typeid(OptionInt<uint32_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_PATH_MTU_PLATEAU_TABLE, begin, begin + 10,
                                    typeid(OptionIntArray<uint16_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_INTERFACE_MTU, begin, begin + 2,
                                    typeid(OptionInt<uint16_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_ALL_SUBNETS_LOCAL, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_BROADCAST_ADDRESS, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_PERFORM_MASK_DISCOVERY, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_MASK_SUPPLIER, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_ROUTER_DISCOVERY, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_ROUTER_SOLICITATION_ADDRESS, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_STATIC_ROUTES, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_TRAILER_ENCAPSULATION, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_ARP_CACHE_TIMEOUT, begin, begin + 4,
                                    typeid(OptionInt<uint32_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_IEEE802_3_ENCAPSULATION, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_DEFAULT_TCP_TTL, begin, begin + 1,
                                    typeid(OptionInt<uint8_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_TCP_KEEPALIVE_INTERVAL, begin,
                                    begin + 4, typeid(OptionInt<uint32_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_TCP_KEEPALIVE_GARBAGE, begin, begin + 1,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_NIS_DOMAIN, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_NIS_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_NTP_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    // The following option requires well formed buffer to be created from.
    // Not just a dummy one. This buffer includes some suboptions.
    OptionBuffer vendor_opts_buf = createVendorOption();
    LibDhcpTest::testStdOptionDefs4(DHO_VENDOR_ENCAPSULATED_OPTIONS,
                                    vendor_opts_buf.begin(),
                                    vendor_opts_buf.end(),
                                    typeid(OptionCustom),
                                    "vendor-encapsulated-options-space");

    LibDhcpTest::testStdOptionDefs4(DHO_NETBIOS_NAME_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_NETBIOS_DD_SERVER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_NETBIOS_NODE_TYPE, begin, begin + 1,
                                    typeid(OptionInt<uint8_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_NETBIOS_SCOPE, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_FONT_SERVERS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_X_DISPLAY_MANAGER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_REQUESTED_ADDRESS, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_LEASE_TIME, begin, begin + 4,
                                    typeid(OptionInt<uint32_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_OPTION_OVERLOAD, begin, begin + 1,
                                    typeid(OptionInt<uint8_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_MESSAGE_TYPE, begin, begin + 1,
                                    typeid(OptionInt<uint8_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_SERVER_IDENTIFIER, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_PARAMETER_REQUEST_LIST, begin, end,
                                    typeid(OptionUint8Array));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_MESSAGE, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_MAX_MESSAGE_SIZE, begin, begin + 2,
                                    typeid(OptionInt<uint16_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_RENEWAL_TIME, begin, begin + 4,
                                    typeid(OptionInt<uint32_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_REBINDING_TIME, begin, begin + 4,
                                    typeid(OptionInt<uint32_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_VENDOR_CLASS_IDENTIFIER, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_CLIENT_IDENTIFIER, begin, end,
                                    typeid(Option));

    LibDhcpTest::testStdOptionDefs4(DHO_NWIP_DOMAIN_NAME, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_NWIP_SUBOPTIONS, begin, end,
                                    typeid(Option));

    LibDhcpTest::testStdOptionDefs4(DHO_NISP_DOMAIN_NAME, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_NISP_SERVER_ADDR, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_TFTP_SERVER_NAME, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_BOOT_FILE_NAME, begin, end,
                                    typeid(OptionString));

    LibDhcpTest::testStdOptionDefs4(DHO_HOME_AGENT_ADDRS, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_SMTP_SERVER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_POP3_SERVER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_NNTP_SERVER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_WWW_SERVER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_FINGER_SERVER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_IRC_SERVER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_STREETTALK_SERVER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_STDASERVER, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_USER_CLASS, begin, end,
                                    typeid(Option));

    LibDhcpTest::testStdOptionDefs4(DHO_FQDN, begin, begin + 3,
                                    typeid(Option4ClientFqdn));

    // The following option requires well formed buffer to be created from.
    // Not just a dummy one. This buffer includes some suboptions.
    OptionBuffer agent_info_buf = createAgentInformationOption();
    LibDhcpTest::testStdOptionDefs4(DHO_DHCP_AGENT_OPTIONS,
                                    agent_info_buf.begin(),
                                    agent_info_buf.end(),
                                    typeid(OptionCustom),
                                    "dhcp-agent-options-space");

    LibDhcpTest::testStdOptionDefs4(DHO_AUTHENTICATE, begin, end,
                                    typeid(Option));

    LibDhcpTest::testStdOptionDefs4(DHO_CLIENT_LAST_TRANSACTION_TIME,
                                    begin, begin + 4,
                                    typeid(OptionInt<uint32_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_ASSOCIATED_IP, begin, end,
                                    typeid(Option4AddrLst));

    LibDhcpTest::testStdOptionDefs4(DHO_SUBNET_SELECTION, begin, end,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_SYSTEM, begin, end,
                                    typeid(OptionIntArray<uint16_t>));

    LibDhcpTest::testStdOptionDefs4(DHO_NDI, begin, begin + 3,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_UUID_GUID, begin, begin + 17,
                                    typeid(OptionCustom));

    LibDhcpTest::testStdOptionDefs4(DHO_DOMAIN_SEARCH, begin, end,
                                    typeid(Option));

    // V-I Vendor option requires specially crafted data.
    const char vivco_data[] = {
        1, 2, 3, 4, // enterprise id
        3, 1, 2, 3  // first byte is opaque data length, the rest is opaque data
    };
    std::vector<uint8_t> vivco_buf(vivco_data, vivco_data + sizeof(vivco_data));
    const char vivsio_data[] = {
        1, 2, 3, 4, // enterprise id
        4,          // first byte is vendor block length
        1, 2, 3, 4  // option type=1 length=2
    };
    std::vector<uint8_t> vivsio_buf(vivsio_data, vivsio_data + sizeof(vivsio_data));

    LibDhcpTest::testStdOptionDefs4(DHO_VIVCO_SUBOPTIONS, vivco_buf.begin(),
                                    vivco_buf.end(), typeid(OptionVendorClass));


    LibDhcpTest::testStdOptionDefs4(DHO_VIVSO_SUBOPTIONS, vivsio_buf.begin(),
                                    vivsio_buf.end(), typeid(OptionVendor));
}

// Test that definitions of standard options have been initialized
// correctly.
// @todo Only limited number of option definitions are now created
// This test have to be extended once all option definitions are
// created.
/*

// This test checks if the DHCPv4 option definition can be searched by
// an option name.
TEST_F(LibDhcpTest, getOptionDefByName4) {
    // Get all definitions.
    const OptionDefContainerPtr defs = LibDHCP::getOptionDefs(Option::V4);
    // For each definition try to find it using option name.
    for (OptionDefContainer::const_iterator def = defs->begin();
         def != defs->end(); ++def) {
        OptionDefinitionPtr def_by_name =
            LibDHCP::getOptionDef(Option::V4, (*def)->getName());
        ASSERT_TRUE(def_by_name);
        ASSERT_TRUE(**def == *def_by_name);
    }
}


// This test checks if the definition of the DHCPv4 vendor option can
// be searched by option name.
TEST_F(LibDhcpTest, getVendorOptionDefByName4) {
    const OptionDefContainerPtr& defs =
        LibDHCP::getVendorOption4Defs(VENDOR_ID_CABLE_LABS);
    ASSERT_TRUE(defs);
    for (OptionDefContainer::const_iterator def = defs->begin();
         def != defs->end(); ++def) {
        OptionDefinitionPtr def_by_name =
            LibDHCP::getVendorOptionDef(Option::V4, VENDOR_ID_CABLE_LABS,
                                        (*def)->getName());
        ASSERT_TRUE(def_by_name);
        ASSERT_TRUE(**def == *def_by_name);
    }
}


// This test verifies that it is possible to add runtime option definitions,
// retrieve them and remove them.
TEST_F(LibDhcpTest, setRuntimeOptionDefs) {
    // Create option definitions in 5 namespaces.
    OptionDefSpaceContainer defs;
    createRuntimeOptionDefs(5, 100, defs);

    // Apply option definitions.
    ASSERT_NO_THROW(LibDHCP::setRuntimeOptionDefs(defs));

    // Retrieve all inserted option definitions.
    testRuntimeOptionDefs(5, 100, true);

    // Attempting to retrieve non existing definitions.
    EXPECT_FALSE(LibDHCP::getRuntimeOptionDef("option-space-non-existent", 1));
    EXPECT_FALSE(LibDHCP::getRuntimeOptionDef("option-space-0", 145));

    // Remove all runtime option definitions.
    ASSERT_NO_THROW(LibDHCP::clearRuntimeOptionDefs());

    // All option definitions should be gone now.
    testRuntimeOptionDefs(5, 100, false);
}
*/

} // end of anonymous space
