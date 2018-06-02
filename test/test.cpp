#include "gtest/gtest.h"
#include "input.h"
#include "../main.cpp"

TEST(basicComputation, Addition) {
  EXPECT_EQ(1, 1);
}

int main (int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  RUN_ALL_TESTS();
}
