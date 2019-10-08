#include <kea/ping/ping.h>
#include <gtest/gtest.h>

using namespace kea::pinger;

TEST(PingTest, ping) {
	int expect_ok = 0;
	int expect_time_out = 0;
	Pinger pg(2048);
    sleep(1);
	PingerCallBack callback = [&](std::string result, bool is_ping){
		if (is_ping) {
			++expect_ok;
		} else {
			++expect_time_out;
		}
	};
	for(int i=0; i<100; ++i) {
		pg.ping("192.168.1.8", callback);
		pg.ping("19.16.1.8", callback);
	}

	sleep(6);
	EXPECT_EQ(100, expect_ok);
	EXPECT_EQ(100, expect_time_out);
}
