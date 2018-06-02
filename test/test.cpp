#include "gtest/gtest.h"
#include "input.h"

TEST(basicComputation, Addition) {
  EXPECT_EQ(1, 1);
}

int main (int argc, char* argv[]) {
  auto test_file = argv[1];
 
  ::testing::InitGoogleTest(&argc, argv);
  RUN_ALL_TESTS();
}
