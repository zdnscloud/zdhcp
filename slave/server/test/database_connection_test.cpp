#include <kea/exceptions/exceptions.h>
#include <kea/server/database_connection.h>
#include <gtest/gtest.h>

using namespace kea::server;


TEST(DatabaseConnectionTest, getParameter) {
    DatabaseConnection::ParameterMap pmap;
    pmap[std::string("param1")] = std::string("value1");
    pmap[std::string("param2")] = std::string("value2");
    DatabaseConnection datasrc(pmap);

    EXPECT_EQ("value1", datasrc.getParameter("param1"));
    EXPECT_EQ("value2", datasrc.getParameter("param2"));
    EXPECT_THROW(datasrc.getParameter("param3"), kea::BadValue);
}

TEST(DatabaseConnectionTest, parse) {

    DatabaseConnection::ParameterMap parameters = DatabaseConnection::parse(
        "user=me password=forbidden name=kea somethingelse= type=mysql");

    EXPECT_EQ(5, parameters.size());
    EXPECT_EQ("me", parameters["user"]);
    EXPECT_EQ("forbidden", parameters["password"]);
    EXPECT_EQ("kea", parameters["name"]);
    EXPECT_EQ("mysql", parameters["type"]);
    EXPECT_EQ("", parameters["somethingelse"]);
}

TEST(DatabaseConnectionTest, parseInvalid) {

    std::string invalid = "";
    DatabaseConnection::ParameterMap parameters = DatabaseConnection::parse(invalid);
    EXPECT_EQ(0, parameters.size());

    invalid = "   \t  ";
    EXPECT_THROW(DatabaseConnection::parse(invalid), kea::InvalidParameter);

    invalid = "   noequalshere  ";
    EXPECT_THROW(DatabaseConnection::parse(invalid), kea::InvalidParameter);

    invalid = "=";
    parameters = DatabaseConnection::parse(invalid);
    EXPECT_EQ(1, parameters.size());
    EXPECT_EQ("", parameters[""]);
}

TEST(DatabaseConnectionTest, redactAccessString) {
    DatabaseConnection::ParameterMap parameters =
        DatabaseConnection::parse("user=me password=forbidden name=kea type=mysql");
    EXPECT_EQ(4, parameters.size());
    EXPECT_EQ("me", parameters["user"]);
    EXPECT_EQ("forbidden", parameters["password"]);
    EXPECT_EQ("kea", parameters["name"]);
    EXPECT_EQ("mysql", parameters["type"]);

    std::string redacted = DatabaseConnection::redactedAccessString(parameters);
    parameters = DatabaseConnection::parse(redacted);

    EXPECT_EQ(4, parameters.size());
    EXPECT_EQ("me", parameters["user"]);
    EXPECT_EQ("*****", parameters["password"]);
    EXPECT_EQ("kea", parameters["name"]);
    EXPECT_EQ("mysql", parameters["type"]);
}

TEST(DatabaseConnectionTest, redactAccessStringEmptyPassword) {
    DatabaseConnection::ParameterMap parameters =
        DatabaseConnection::parse("user=me name=kea type=mysql password=");
    EXPECT_EQ(4, parameters.size());
    EXPECT_EQ("me", parameters["user"]);
    EXPECT_EQ("", parameters["password"]);
    EXPECT_EQ("kea", parameters["name"]);
    EXPECT_EQ("mysql", parameters["type"]);

    std::string redacted = DatabaseConnection::redactedAccessString(parameters);
    parameters = DatabaseConnection::parse(redacted);

    EXPECT_EQ(4, parameters.size());
    EXPECT_EQ("me", parameters["user"]);
    EXPECT_EQ("*****", parameters["password"]);
    EXPECT_EQ("kea", parameters["name"]);
    EXPECT_EQ("mysql", parameters["type"]);

    parameters = DatabaseConnection::parse("user=me password= name=kea type=mysql");
    EXPECT_EQ(4, parameters.size());
    EXPECT_EQ("me", parameters["user"]);
    EXPECT_EQ("", parameters["password"]);
    EXPECT_EQ("kea", parameters["name"]);
    EXPECT_EQ("mysql", parameters["type"]);

    redacted = DatabaseConnection::redactedAccessString(parameters);
    parameters = DatabaseConnection::parse(redacted);

    EXPECT_EQ(4, parameters.size());
    EXPECT_EQ("me", parameters["user"]);
    EXPECT_EQ("*****", parameters["password"]);
    EXPECT_EQ("kea", parameters["name"]);
    EXPECT_EQ("mysql", parameters["type"]);
}

TEST(DatabaseConnectionTest, redactAccessStringNoPassword) {
    DatabaseConnection::ParameterMap parameters =
        DatabaseConnection::parse("user=me name=kea type=mysql");
    EXPECT_EQ(3, parameters.size());
    EXPECT_EQ("me", parameters["user"]);
    EXPECT_EQ("kea", parameters["name"]);
    EXPECT_EQ("mysql", parameters["type"]);

    std::string redacted = DatabaseConnection::redactedAccessString(parameters);
    parameters = DatabaseConnection::parse(redacted);

    EXPECT_EQ(3, parameters.size());
    EXPECT_EQ("me", parameters["user"]);
    EXPECT_EQ("kea", parameters["name"]);
    EXPECT_EQ("mysql", parameters["type"]);
}
