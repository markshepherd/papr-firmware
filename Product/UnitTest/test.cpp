#include "pch.h"
#include "..\Main.h"

Main paprMain;

TEST(TestCaseName, TestName) {
    paprMain.setup();
    paprMain.loop();

    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
}