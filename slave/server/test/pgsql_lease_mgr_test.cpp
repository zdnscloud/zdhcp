#include <kea/server/pgsql_lease_mgr.h>
#include <kea/server/testutil/generic_lease_mgr_test.h>
#include <kea/server/testutil/pgsql_schema.h>
#include <kea/exceptions/exceptions.h>

#include <gtest/gtest.h>

using namespace kea;
using namespace kea::dhcp;
using namespace kea::server;
using namespace kea::server::testutil;
using namespace std;

namespace kea {

class PgSqlLeaseMgrTest : public GenericLeaseMgrTest {
public:
    PgSqlLeaseMgrTest() {
        destroyPgSQLSchema();
        createPgSQLSchema();

        try {
            lmptr_ = new PgSqlLeaseMgr(DatabaseConnection::parse(validPgSQLConnectionString()));
        } catch (...) {
            std::cerr << "*** ERROR: unable to open database. The test\n"
                         "*** environment is broken and must be fixed before\n"
                         "*** the PostgreSQL tests will run correctly.\n"
                         "*** The reason for the problem is described in the\n"
                         "*** accompanying exception output.\n";
            throw;
        }
    }

    virtual ~PgSqlLeaseMgrTest() {
        lmptr_->rollback();
        delete lmptr_;
    }

    void reopen(Universe) {
        lmptr_ = new PgSqlLeaseMgr(DatabaseConnection::parse(validPgSQLConnectionString()));
    }

};

TEST_F(PgSqlLeaseMgrTest, getType) {
    EXPECT_EQ(std::string("postgresql"), lmptr_->getType());
}

TEST_F(PgSqlLeaseMgrTest, getName) {
    EXPECT_EQ(std::string("zdns"), lmptr_->getName());
}

TEST_F(PgSqlLeaseMgrTest, checkVersion) {
    // Check version
    pair<uint32_t, uint32_t> version;
    ASSERT_NO_THROW(version = lmptr_->getVersion());
    EXPECT_EQ(PG_CURRENT_VERSION, version.first);
    EXPECT_EQ(PG_CURRENT_MINOR, version.second);
}

TEST_F(PgSqlLeaseMgrTest, basicLease4) {
    testBasicLease4();
}

/// @brief Check that Lease4 code safely handles invalid dates.
TEST_F(PgSqlLeaseMgrTest, maxDate4) {
    testMaxDate4();
}

/// @brief Lease4 update tests
///
/// Checks that we are able to update a lease in the database.
TEST_F(PgSqlLeaseMgrTest, updateLease4) {
    //testUpdateLease4();
}

/// @brief Check GetLease4 methods - access by Hardware Address
TEST_F(PgSqlLeaseMgrTest, getLease4HWAddr1) {
    testGetLease4HWAddr1();
}

/// @brief Check GetLease4 methods - access by Hardware Address
TEST_F(PgSqlLeaseMgrTest, getLease4HWAddr2) {
    testGetLease4HWAddr2();
}

// @brief Get lease4 by hardware address (2)
//
// Check that the system can cope with getting a hardware address of
// any size.
TEST_F(PgSqlLeaseMgrTest, getLease4HWAddrSize) {
    testGetLease4HWAddrSize();
}

TEST_F(PgSqlLeaseMgrTest, getLease4HwaddrSubnetId) {
    testGetLease4HWAddrSubnetId();
}

// @brief Get lease4 by hardware address and subnet ID (2)
//
// Check that the system can cope with getting a hardware address of
// any size.
TEST_F(PgSqlLeaseMgrTest, getLease4HWAddrSubnetIdSize) {
    testGetLease4HWAddrSubnetIdSize();
}

// This test was derived from memfile.
TEST_F(PgSqlLeaseMgrTest, getLease4ClientId) {
    testGetLease4ClientId();
}


TEST_F(PgSqlLeaseMgrTest, getLease4NullClientId) {
    testGetLease4NullClientId();
}

TEST_F(PgSqlLeaseMgrTest, getLease4ClientId2) {
    testGetLease4ClientId2();
}

TEST_F(PgSqlLeaseMgrTest, getLease4ClientIdSize) {
    testGetLease4ClientIdSize();
}

TEST_F(PgSqlLeaseMgrTest, getLease4ClientIdSubnetId) {
    testGetLease4ClientIdSubnetId();
}

TEST_F(PgSqlLeaseMgrTest, lease4NullClientId) {
    testLease4NullClientId();
}

TEST_F(PgSqlLeaseMgrTest, lease4InvalidHostname) {
    testLease4InvalidHostname();
}

TEST_F(PgSqlLeaseMgrTest, getExpiredLeases4) {
    testGetExpiredLeases4();
}

TEST_F(PgSqlLeaseMgrTest, deleteExpiredReclaimedLeases4) {
    testDeleteExpiredReclaimedLeases4();
}

}; 
