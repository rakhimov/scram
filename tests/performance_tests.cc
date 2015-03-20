#include "performance_tests.h"

#include <string>

using namespace scram;

// Performance testing is done only if requested by activating
// disabled tests.
//
// To run the performance tests, supply "--gtest_also_run_disable_tests" flag
// to GTest. The GTest filter may applied to sortout only performance tests.
// Different tests are compiled depending on the build type. Generally,
// debug or non-debug types are recognized.
//
// Performance testing values are taken from a computer with the following
// specs:
//   Core i5-2410M, Ubuntu 14.04 64bit on VirtualBox
//   Compilation with GCC 4.8.2 and Boost 1.55
//
// The values for performance are expected to have some random variation.
// Better as well as worse performance are reported as test failures to
// indicate the change.
//
// NOTE: Running all the tests may take considerable time.
// NOTE: Running tests several times is recommended to take into account
//       the variation of time results.

// Tests the performance of probability calculations
// with cut-off approximations tests are done.
TEST_F(PerformanceTest, DISABLED_ThreeMotor) {
  double p_time_std = 0.010;
  std::string input = "./share/scram/input/benchmark/three_motor.xml";
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_LT(ProbCalcTime(), p_time_std);
}

// Test performance of MCS generation.
TEST_F(PerformanceTest, DISABLED_200Event_L17) {
  double mcs_time = 9.2;
#ifdef NDEBUG
  mcs_time = 2.60;
#endif
  std::string input = "./share/scram/input/benchmark/200_event.xml";
  settings.limit_order(17);
  settings.num_sums(1);
  settings.cut_off(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(8487, NumOfMcs());
  EXPECT_NEAR(mcs_time, McsGenerationTime(), mcs_time * delta);
}

// Test performance of MCS generation.
TEST_F(PerformanceTest, DISABLED_Baobab1_L6) {
  double mcs_time = 1.800;
#ifdef NDEBUG
  mcs_time = 0.280;
#endif
  std::string input = "./share/scram/input/benchmark/baobab1.xml";
  settings.limit_order(6);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(2684, NumOfMcs());
  EXPECT_NEAR(mcs_time, McsGenerationTime(), mcs_time * delta);
}

// Test performance of MCS generation.
TEST_F(PerformanceTest, DISABLED_Baobab1_L7) {
  double mcs_time = 20.500;
#ifdef NDEBUG
  mcs_time = 3.500;
#endif
  std::string input = "./share/scram/input/benchmark/baobab1.xml";
  settings.limit_order(7);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(17432, NumOfMcs());
  EXPECT_NEAR(mcs_time, McsGenerationTime(), mcs_time * delta);
}

TEST_F(PerformanceTest, DISABLED_CEA9601_L5) {
  double mcs_time = 12.500;
#ifdef NDEBUG
  mcs_time = 1.900;
#endif
  std::string input = "./share/scram/input/benchmark/CEA9601.xml";
  settings.limit_order(5);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(3274, NumOfMcs());
  EXPECT_NEAR(mcs_time, McsGenerationTime(), mcs_time * delta);
}

// Release only tests.
#ifdef NDEBUG
// Test performance of minimality detection and MCS generation.
TEST_F(PerformanceTest, DISABLED_200Event_L24) {
  double mcs_time = 8.75;
  std::string input = "./share/scram/input/benchmark/200_event.xml";
  settings.limit_order(24);
  settings.num_sums(1);
  settings.cut_off(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(16951, NumOfMcs());
  EXPECT_NEAR(mcs_time, McsGenerationTime(), mcs_time * delta);
}

TEST_F(PerformanceTest, DISABLED_CEA9601_L7) {
  double mcs_time = 11.000;
  std::string input = "./share/scram/input/benchmark/CEA9601.xml";
  settings.limit_order(7);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(4578, NumOfMcs());
  EXPECT_NEAR(mcs_time, McsGenerationTime(), mcs_time * delta);
}
#endif
