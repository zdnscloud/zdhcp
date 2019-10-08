#include <kea/dhcp++/std_option_defs.h>
#include <kea/dhcp++/option4_addrlst.h>
#include <kea/server/client_class_matcher.h>
#include <gtest/gtest.h>

using namespace kea;
using namespace kea::dhcp;
using namespace kea::server;

namespace kea {

const uint32_t dummyTransid = 0x12345678;
const uint8_t dummyOp = BOOTREQUEST;
const uint8_t dummyHtype = 6; 
const uint8_t dummyHlen = 6; 
const uint8_t dummyHops = 13;
const uint16_t dummySecs = 42;
const uint16_t dummyFlags = BOOTP_BROADCAST;

const IPAddressV4 dummyCiaddr("192.0.2.1");
const IPAddressV4 dummyYiaddr("1.2.3.4");
const IPAddressV4 dummySiaddr("192.0.2.255");
const IPAddressV4 dummyGiaddr("255.255.255.255");
const uint8_t dummyMacAddr[] = {0, 1, 2, 3, 4, 5};
const uint8_t dummyChaddr[16] = {0, 1, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; 
const uint8_t dummyFile[] = "Lorem ipsum dolor sit amet, consectetur "
                            "adipiscing elit. Proin mollis placerat metus, at "
                            "lacinia orci ornare vitae. Mauris amet.";
const uint8_t dummySname[] = "Lorem ipsum dolor sit amet, consectetur " 
                             "adipiscing elit posuere.";


std::unique_ptr<Pkt4> generatePacket() {
    std::unique_ptr<Pkt4> pkt(new Pkt4(DHCPDISCOVER, dummyTransid));
    vector<uint8_t> vectorMacAddr(dummyMacAddr, dummyMacAddr + sizeof(dummyMacAddr));
    pkt->setHWAddr(dummyHtype, dummyHlen, vectorMacAddr);
    pkt->setHops(dummyHops);
    pkt->setSecs(dummySecs);
    pkt->setFlags(dummyFlags); 
    pkt->setCiaddr(dummyCiaddr);
    pkt->setYiaddr(dummyYiaddr);
    pkt->setSiaddr(dummySiaddr);
    pkt->setGiaddr(dummyGiaddr);
    pkt->setSname(dummySname, 64);
    pkt->setFile(dummyFile, 128);

    std::unique_ptr<Option> request_addr_opt(new Option4AddrLst(DHO_DHCP_REQUESTED_ADDRESS, IPAddress("2.2.2.2")));
    pkt->addOption(std::move(request_addr_opt));
    return (std::move(pkt));
}

TEST(MatcherBuilder, optionValueMatcher) {
    initStdOptions();

    std::unique_ptr<Pkt4> pkt = generatePacket();
    ClientClassMatcherBuilder builder;
    builder.addOptionValueMatcher("dhcp-requested-address", true, "1.1.1.1");
    EXPECT_FALSE(builder.getMatcher()(*pkt));


    builder.clean();
    builder.addOptionValueMatcher("dhcp-requested-address", true, "2.2.2.2");
    EXPECT_TRUE(builder.getMatcher()(*pkt));
}

TEST(MatcherBuilder, optionExistsMatcher) {
    initStdOptions();

    std::unique_ptr<Pkt4> pkt = generatePacket();
    ClientClassMatcherBuilder builder;
    builder.addOptionExistsMatcher("dhcp-requested-address", false);
    EXPECT_FALSE(builder.getMatcher()(*pkt));


    builder.clean();
    builder.addOptionExistsMatcher("dhcp-requested-address", true);
    EXPECT_TRUE(builder.getMatcher()(*pkt));
}

TEST(MatcherBuilder, subStringMatcher) {
    initStdOptions();

    std::unique_ptr<Pkt4> pkt = generatePacket();
    ClientClassMatcherBuilder builder;
    builder.addSubstringMatcher("dhcp-requested-address", 0, 4, false, "2.2.");
    EXPECT_FALSE(builder.getMatcher()(*pkt));

    builder.clean();
    builder.addSubstringMatcher("dhcp-requested-address", 0, 4, true, "2.2.");
    EXPECT_TRUE(builder.getMatcher()(*pkt));
}

TEST(MatcherBuilder, andOrMatcher) {
    initStdOptions();

    std::unique_ptr<Pkt4> pkt = generatePacket();
    ClientClassMatcherBuilder builder;
    builder.addSubstringMatcher("dhcp-requested-address", 0, 4, false, "2.2.");
    builder.onAnd();
    builder.addOptionValueMatcher("dhcp-requested-address", true, "2.2.2.2");
    EXPECT_FALSE(builder.getMatcher()(*pkt));

    builder.clean();
    builder.addSubstringMatcher("dhcp-requested-address", 0, 4, false, "2.2.");
    builder.onOr();
    builder.addOptionValueMatcher("dhcp-requested-address", true, "2.2.2.2");
    EXPECT_TRUE(builder.getMatcher()(*pkt));

    builder.clean();
    builder.addSubstringMatcher("dhcp-requested-address", 0, 4, true, "2.2.");
    builder.onAnd();
    builder.addOptionValueMatcher("dhcp-requested-address", true, "2.2.2.2");
    EXPECT_TRUE(builder.getMatcher()(*pkt));

    builder.clean();
    builder.addSubstringMatcher("dhcp-requested-address", 0, 4, true, "2.2.");
    builder.onAnd();
    builder.addOptionValueMatcher("dhcp-requested-address", true, "2.2.2.2");
    builder.onAnd();
    builder.addOptionExistsMatcher("dhcp-requested-address", true);
    EXPECT_TRUE(builder.getMatcher()(*pkt));
}

TEST(MatcherBuilder, bracketMatcher) {
    initStdOptions();

    std::unique_ptr<Pkt4> pkt = generatePacket();
    ClientClassMatcherBuilder builder;
    builder.addSubstringMatcher("dhcp-requested-address", 0, 4, false, "2.2.");
    builder.onAnd();
    builder.addOptionValueMatcher("dhcp-requested-address", false, "2.2.2.2");
    builder.onOr();
    builder.addOptionExistsMatcher("dhcp-requested-address", true);
    EXPECT_TRUE(builder.getMatcher()(*pkt));

    builder.clean();
    builder.addSubstringMatcher("dhcp-requested-address", 0, 4, false, "2.2.");
    builder.onAnd();
    builder.onLeftBracket();
    builder.addOptionValueMatcher("dhcp-requested-address", false, "2.2.2.2");
    builder.onOr();
    builder.addOptionExistsMatcher("dhcp-requested-address", true);
    builder.onRightBracket();
    EXPECT_FALSE(builder.getMatcher()(*pkt));
}

};
