#include <kea/server/pgsql_exchange.h>
#include <boost/lexical_cast.hpp>
#include <gtest/gtest.h>

using namespace kea;
using namespace kea::server;

namespace kea {
std::string timeToDbString(const time_t time_val) {
    struct tm tinfo;
    char buffer[20];

    localtime_r(&time_val, &tinfo);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tinfo);
    return(std::string(buffer));
}

TEST(PgSqlExchangeTest, convertTimeTest) {
    // Get a reference time and time string
    time_t ref_time;
    time(&ref_time);

    std::string ref_time_str(timeToDbString(ref_time));
    // Verify convertToDatabaseTime gives us the expected localtime string
    std::string time_str = PgSqlExchange::convertToDatabaseTime(ref_time);
    EXPECT_EQ(time_str, ref_time_str);

    time_str = PgSqlExchange::convertToDatabaseTime(ref_time, 0);
    EXPECT_EQ(time_str, ref_time_str);

    ref_time_str = timeToDbString(ref_time + (24*3600));
    ASSERT_NO_THROW(time_str = PgSqlExchange::convertToDatabaseTime(ref_time,
                                                                    24*3600));
    EXPECT_EQ(time_str, ref_time_str);
    ASSERT_THROW(PgSqlExchange::convertToDatabaseTime(DatabaseConnection::
                                                      MAX_DB_TIME - 1,
                                                      24*3600),
                 kea::BadValue);

    std::string ref_secs_str = boost::lexical_cast<std::string>(ref_time);
    time_t from_time = PgSqlExchange::convertFromDatabaseTime(ref_secs_str);
    from_time = PgSqlExchange::convertFromDatabaseTime(ref_secs_str);
    EXPECT_EQ(ref_time, from_time);
}
}; 
