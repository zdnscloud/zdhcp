#include <kea/ping/timer_queue.h>
#include <gtest/gtest.h>

using namespace kea;
using namespace kea::pinger;

TEST(TimerTest, testTimer) {
	uint32_t count=0;
	TimerQueue tq(100, std::chrono::milliseconds(1000));
	sleep(1);
	
	TimerCallBack callback = [&](){
		++count;
	};	

	for (int i=0; i<20; ++i) {
		tq.addTimer(callback);
	}
	sleep(2);

	EXPECT_EQ(20, count);
}

TEST(TimerTest, testFull) {
	bool expect_true = false;
	bool expect_false = true;
	TimerQueue tq(10, std::chrono::milliseconds(1000));
	sleep(1);
	
	TimerCallBack callback = [&](){
	};	
	
	for (int i=0; i<10; ++i) {
        EXPECT_TRUE(tq.addTimer(callback));
		
	}
	expect_false = tq.addTimer(callback);
	EXPECT_FALSE(expect_false);

	sleep(2);
    EXPECT_TRUE(tq.addTimer(callback));
}
