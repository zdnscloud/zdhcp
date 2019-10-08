#include <kea/util/lru_cache.h>
#include <gtest/gtest.h>

namespace kea {

using namespace kea::util;

TEST(LruCacheTest, SimplePut) {
    LruCache<int, int> cache(1);
    cache.put(7, 777);
    EXPECT_TRUE(cache.exists(7));
    EXPECT_EQ(777, cache.get(7));
    EXPECT_EQ(1, cache.size());
}

TEST(LruCacheTest, MissingValue) {
    LruCache<int, int> cache(1);
    EXPECT_THROW(cache.get(7), std::range_error);
}

TEST(LruCacheTest1, KeepsAllValuesWithinCapacity) {
    int cap = 100;
    LruCache<int, int> cache(cap);

    for (int i = 0; i < cap; i++) {
        cache.put(i, i);
    }

    for (int i = cap; i < cap + cap; i++) {
        EXPECT_FALSE(cache.exists(i));
        EXPECT_THROW(cache.get(i), std::range_error);
    }

    for (int i = 0; i < cap; i++) {
        EXPECT_TRUE(cache.exists(i));
        EXPECT_EQ(i, cache.get(i));
    }

    int* pint = nullptr;
    for (int i = 0; i < cap; i++) {
        EXPECT_TRUE(cache.find(i, &pint));
        EXPECT_EQ(i, *pint);
    }

    size_t size = cache.size();
    EXPECT_EQ(cap, size);
}

};
