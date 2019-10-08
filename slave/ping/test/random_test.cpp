#include <kea/ping/random_generate.h>
#include <gtest/gtest.h>

using namespace kea;
using namespace pinger;

TEST(TestRandom, generate) {
	uint16_t random1;
	uint16_t random2;
	RandomGenerate<uint16_t> rg;

    for (int i=0; i<2000; ++i) {
        EXPECT_NE(rg.getRandom(), rg.getRandom());
    }
}
