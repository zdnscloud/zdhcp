#include <kea/hooks/callout_handle.h>
#include <kea/hooks/callout_manager.h>
#include <kea/hooks/server_hooks.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <climits>
#include <string>
#include <vector>
#include <memory>

using namespace kea::hooks;
using namespace std;

namespace kea {

class CalloutManagerTest : public ::testing::Test {
public:
    CalloutManagerTest() {
        ServerHooks& hooks = ServerHooks::instance();
        hooks.reset();
        alpha_index_ = hooks.registerHook("alpha");
        beta_index_ = hooks.registerHook("beta");
        gamma_index_ = hooks.registerHook("gamma");
        delta_index_ = hooks.registerHook("delta");

        callout_manager_.reset(new CalloutManager());
        callout_handle_.reset(new CalloutHandle(*callout_manager_));
        callout_value_ = 0;
    }

    CalloutHandle& getCalloutHandle() {
        return (*callout_handle_);
    }

    CalloutManager* getCalloutManager() {
        return (callout_manager_.get());
    }

    static int callout_value_;

    int alpha_index_;
    int beta_index_;
    int gamma_index_;
    int delta_index_;

private:
    std::unique_ptr<CalloutHandle> callout_handle_;
    std::unique_ptr<CalloutManager> callout_manager_;
};

int CalloutManagerTest::callout_value_ = 0;

extern "C" {
int callout_general(int number) {
    CalloutManagerTest::callout_value_ =
        10 * CalloutManagerTest::callout_value_ + number;
    return (0);
}

int callout_one(CalloutHandle&) {
    return (callout_general(1));
}

int callout_two(CalloutHandle&) {
    return (callout_general(2));
}

int callout_three(CalloutHandle&) {
    return (callout_general(3));
}

int callout_four(CalloutHandle&) {
    return (callout_general(4));
}

int callout_five(CalloutHandle&) {
    return (callout_general(5));
}

int callout_six(CalloutHandle&) {
    return (callout_general(6));
}

int callout_seven(CalloutHandle&) {
    return (callout_general(7));
}

int callout_one_error(CalloutHandle& handle) {
    (void) callout_one(handle);
    return (1);
}

int callout_two_error(CalloutHandle& handle) {
    (void) callout_two(handle);
    return (1);
}

int callout_three_error(CalloutHandle& handle) {
    (void) callout_three(handle);
    return (1);
}

int callout_four_error(CalloutHandle& handle) {
    (void) callout_four(handle);
    return (1);
}

};  

TEST_F(CalloutManagerTest, ValidHookNames) {
    EXPECT_NO_THROW(getCalloutManager()->registerCallout(1, "alpha", callout_one));
    EXPECT_THROW(getCalloutManager()->registerCallout(0, "unknown", callout_one),
                                                      NoSuchHook);
}


TEST_F(CalloutManagerTest, RegisterCallout) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));

    getCalloutManager()->registerCallout(0, "alpha", callout_one);
    getCalloutManager()->registerCallout(1, "beta", callout_two);

    EXPECT_TRUE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_TRUE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(1, callout_value_);

    callout_value_ = 0;
    getCalloutManager()->callCallouts(beta_index_, getCalloutHandle());
    EXPECT_EQ(2, callout_value_);

    // Register some more callouts from different libraries on hook "alpha".
    getCalloutManager()->registerCallout(2, "alpha", callout_three);
    getCalloutManager()->registerCallout(2, "alpha", callout_four);
    getCalloutManager()->registerCallout(3, "alpha", callout_five);

    // Check it is as expected.
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(1345, callout_value_);

    callout_value_ = 0;
    getCalloutManager()->callCallouts(beta_index_, getCalloutHandle());
    EXPECT_EQ(2, callout_value_);

    getCalloutManager()->registerCallout(2, "alpha", callout_six);
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(13465, callout_value_);

    getCalloutManager()->registerCallout(1, "alpha", callout_seven);
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(173465, callout_value_);
}

TEST_F(CalloutManagerTest, CalloutsPresent) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    getCalloutManager()->registerCallout(0, "alpha", callout_one);

    getCalloutManager()->registerCallout(1, "alpha", callout_two);
    getCalloutManager()->registerCallout(1, "beta", callout_two);

    getCalloutManager()->registerCallout(3, "alpha", callout_three);
    getCalloutManager()->registerCallout(3, "delta", callout_four);

    EXPECT_TRUE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_TRUE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_TRUE(getCalloutManager()->calloutsPresent(delta_index_));

    EXPECT_FALSE(getCalloutManager()->calloutsPresent(42));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(-1));
}

TEST_F(CalloutManagerTest, CallNoCallouts) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    callout_value_ = 475;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(475, callout_value_); // Unchanged
}

TEST_F(CalloutManagerTest, CallCalloutsSuccess) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    callout_value_ = 0;
    getCalloutManager()->registerCallout(1, "alpha", callout_one);
    getCalloutManager()->registerCallout(1, "alpha", callout_two);
    getCalloutManager()->registerCallout(2, "alpha", callout_three);
    getCalloutManager()->registerCallout(3, "alpha", callout_four);
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(1234, callout_value_);

    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "beta", callout_one);
    getCalloutManager()->registerCallout(0, "beta", callout_three);
    getCalloutManager()->registerCallout(1, "beta", callout_two);
    getCalloutManager()->registerCallout(3, "beta", callout_four);
    getCalloutManager()->callCallouts(beta_index_, getCalloutHandle());
    EXPECT_EQ(1324, callout_value_);

    // Ensure that calling the callouts on a hook with no callouts works.
    callout_value_ = 0;
    getCalloutManager()->callCallouts(gamma_index_, getCalloutHandle());
    EXPECT_EQ(0, callout_value_);
}

TEST_F(CalloutManagerTest, CallCalloutsError) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "alpha", callout_one_error);
    getCalloutManager()->registerCallout(1, "alpha", callout_two);
    getCalloutManager()->registerCallout(2, "alpha", callout_three);
    getCalloutManager()->registerCallout(3, "alpha", callout_four);
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(1234, callout_value_);

    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "beta", callout_one);
    getCalloutManager()->registerCallout(0, "beta", callout_one_error);
    getCalloutManager()->registerCallout(1, "beta", callout_two);
    getCalloutManager()->registerCallout(1, "beta", callout_two);
    getCalloutManager()->registerCallout(1, "beta", callout_three);
    getCalloutManager()->registerCallout(1, "beta", callout_three);
    getCalloutManager()->registerCallout(3, "beta", callout_four);
    getCalloutManager()->registerCallout(3, "beta", callout_four);
    getCalloutManager()->callCallouts(beta_index_, getCalloutHandle());
    EXPECT_EQ(11223344, callout_value_);

    // A callout in a random position in the callout list returns an error.
    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "gamma", callout_one);
    getCalloutManager()->registerCallout(0, "gamma", callout_one);
    getCalloutManager()->registerCallout(1, "gamma", callout_two);
    getCalloutManager()->registerCallout(1, "gamma", callout_two);
    getCalloutManager()->registerCallout(3, "gamma", callout_four_error);
    getCalloutManager()->registerCallout(3, "gamma", callout_four);
    getCalloutManager()->callCallouts(gamma_index_, getCalloutHandle());
    EXPECT_EQ(112244, callout_value_);

    // The last callout on a hook returns an error.
    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "delta", callout_one);
    getCalloutManager()->registerCallout(0, "delta", callout_one);
    getCalloutManager()->registerCallout(1, "delta", callout_two);
    getCalloutManager()->registerCallout(1, "delta", callout_two);
    getCalloutManager()->registerCallout(2, "delta", callout_three);
    getCalloutManager()->registerCallout(2, "delta", callout_three);
    getCalloutManager()->registerCallout(3, "delta", callout_four);
    getCalloutManager()->registerCallout(3, "delta", callout_four_error);
    getCalloutManager()->callCallouts(delta_index_, getCalloutHandle());
    EXPECT_EQ(11223344, callout_value_);
}

TEST_F(CalloutManagerTest, DeregisterSingleCallout) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    // Add a callout to hook "alpha" and check it is added correctly.
    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "alpha", callout_two);
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(2, callout_value_);

    EXPECT_TRUE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_TRUE(getCalloutManager()->deregisterCallout(0, "alpha", callout_two));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
}

TEST_F(CalloutManagerTest, DeregisterSingleCalloutSameLibrary) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "alpha", callout_one);
    getCalloutManager()->registerCallout(0, "alpha", callout_two);
    getCalloutManager()->registerCallout(0, "alpha", callout_three);
    getCalloutManager()->registerCallout(0, "alpha", callout_four);
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(1234, callout_value_);

    EXPECT_TRUE(getCalloutManager()->deregisterCallout(0, "alpha", callout_two));
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(134, callout_value_);

    EXPECT_FALSE(getCalloutManager()->deregisterCallout(0, "alpha", callout_two));
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(134, callout_value_);

}

TEST_F(CalloutManagerTest, DeregisterMultipleCalloutsSameLibrary) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "alpha", callout_one);
    getCalloutManager()->registerCallout(0, "alpha", callout_two);
    getCalloutManager()->registerCallout(0, "alpha", callout_one);
    getCalloutManager()->registerCallout(0, "alpha", callout_two);
    getCalloutManager()->registerCallout(0, "alpha", callout_three);
    getCalloutManager()->registerCallout(0, "alpha", callout_four);
    getCalloutManager()->registerCallout(0, "alpha", callout_three);
    getCalloutManager()->registerCallout(0, "alpha", callout_four);
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(12123434, callout_value_);

    EXPECT_TRUE(getCalloutManager()->deregisterCallout(0, "alpha", callout_two));
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(113434, callout_value_);

    EXPECT_TRUE(getCalloutManager()->deregisterCallout(0, "alpha", callout_four));
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(1133, callout_value_);

    EXPECT_TRUE(getCalloutManager()->deregisterCallout(0, "alpha", callout_one));
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(33, callout_value_);

    EXPECT_TRUE(getCalloutManager()->deregisterCallout(0, "alpha", callout_three));
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(0, callout_value_);

    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
}

TEST_F(CalloutManagerTest, DeregisterMultipleCalloutsMultipleLibraries) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "alpha", callout_one);
    getCalloutManager()->registerCallout(0, "alpha", callout_two);
    getCalloutManager()->registerCallout(1, "alpha", callout_three);
    getCalloutManager()->registerCallout(1, "alpha", callout_four);
    getCalloutManager()->registerCallout(2, "alpha", callout_five);
    getCalloutManager()->registerCallout(2, "alpha", callout_two);
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(123452, callout_value_);

    EXPECT_TRUE(getCalloutManager()->deregisterCallout(0, "alpha", callout_two));
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(13452, callout_value_);
}

TEST_F(CalloutManagerTest, DeregisterAllCallouts) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));

    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "alpha", callout_one);
    getCalloutManager()->registerCallout(0, "alpha", callout_two);
    getCalloutManager()->registerCallout(1, "alpha", callout_three);
    getCalloutManager()->registerCallout(1, "alpha", callout_four);
    getCalloutManager()->registerCallout(2, "alpha", callout_five);
    getCalloutManager()->registerCallout(2, "alpha", callout_six);
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(123456, callout_value_);

    EXPECT_TRUE(getCalloutManager()->deregisterAllCallouts(1, "alpha"));
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(1256, callout_value_);

    EXPECT_TRUE(getCalloutManager()->deregisterAllCallouts(2, "alpha"));
    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(12, callout_value_);
}

TEST_F(CalloutManagerTest, MultipleCalloutsLibrariesHooks) {
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(alpha_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(beta_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(gamma_index_));
    EXPECT_FALSE(getCalloutManager()->calloutsPresent(delta_index_));

    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "alpha", callout_one);
    getCalloutManager()->registerCallout(0, "alpha", callout_two);
    getCalloutManager()->registerCallout(1, "alpha", callout_three);
    getCalloutManager()->registerCallout(1, "alpha", callout_four);
    getCalloutManager()->registerCallout(2, "alpha", callout_five);
    getCalloutManager()->registerCallout(2, "alpha", callout_two);
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(123452, callout_value_);

    callout_value_ = 0;
    getCalloutManager()->registerCallout(0, "beta", callout_five);
    getCalloutManager()->registerCallout(0, "beta", callout_one);
    getCalloutManager()->registerCallout(2, "beta", callout_four);
    getCalloutManager()->registerCallout(2, "beta", callout_three);
    getCalloutManager()->callCallouts(beta_index_, getCalloutHandle());
    EXPECT_EQ(5143, callout_value_);

    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(123452, callout_value_);

    EXPECT_TRUE(getCalloutManager()->deregisterCallout(2, "beta", callout_four));

    callout_value_ = 0;
    getCalloutManager()->callCallouts(beta_index_, getCalloutHandle());
    EXPECT_EQ(513, callout_value_);

    callout_value_ = 0;
    getCalloutManager()->callCallouts(alpha_index_, getCalloutHandle());
    EXPECT_EQ(123452, callout_value_);
}
};
