#include <gtest/gtest.h>
#include "..\lib\FanState.h"
TEST(FanState, Cycling) {
    FanState fs;
    ASSERT_EQ(fs.fans(), FANS_Low);
    fs.increase();
    ASSERT_EQ(fs.fans(), FANS_Medium);
    fs.increase();
    ASSERT_EQ(fs.fans(), FANS_High);
    fs.increase();
    ASSERT_EQ(fs.fans(), FANS_High);
    fs.decrease();
    ASSERT_EQ(fs.fans(), FANS_Medium);
    fs.decrease();
    ASSERT_EQ(fs.fans(), FANS_Low);
    fs.decrease();
    ASSERT_EQ(fs.fans(), FANS_Low);
}