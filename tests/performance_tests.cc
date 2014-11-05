#include <gtest/gtest.h>

#include <string>

#include "performance_tests.h"

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
  double p_time_std = 0.230;
#ifdef NDEBUG
  p_time_std = 0.019;
#endif
  std::string input = "./share/scram/input/benchmark/three_motor.xml";
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());  // Standard analysis.
  double delta_sqr = std::abs(p_total() - 0.0211538);
  EXPECT_TRUE(delta_sqr < 1e-5);
  EXPECT_GT(ProbCalcTime(), p_time_std * (1 - delta));
  EXPECT_LT(ProbCalcTime(), p_time_std * (1 + delta));
}

// Tests the performance of probability calculations
// without approximations tests are done.
TEST_F(PerformanceTest, DISABLED_ThreeMotor_C0) {
  double p_time_full = 4.100;  // Without cut-off approximation.
#ifdef NDEBUG
  p_time_full = 0.320;
#endif
  std::string input = "./share/scram/input/benchmark/three_motor.xml";
  settings.cut_off(0);  // No approximation.
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  double delta_sqr = std::abs(p_total() - 0.0211538);
  EXPECT_TRUE(delta_sqr < 1e-5);
  EXPECT_GT(ProbCalcTime(), p_time_full * (1 - delta));
  EXPECT_LT(ProbCalcTime(), p_time_full * (1 + delta));
}

// Test performance of MCS generation.
TEST_F(PerformanceTest, DISABLED_200Event_L16) {
  double mcs_time = 4.800;
#ifdef NDEBUG
  mcs_time = 1.030;
#endif
  std::string input = "./share/scram/input/benchmark/200_event.xml";
  settings.limit_order(16);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(8351, NumOfMcs());
  EXPECT_GT(McsGenerationTime(), mcs_time * (1 - delta));
  EXPECT_LT(McsGenerationTime(), mcs_time * (1 + delta));
}

// Test performance of MCS generation.
TEST_F(PerformanceTest, DISABLED_200Event_L17) {
  double mcs_time = 105.900;
#ifdef NDEBUG
  mcs_time = 15.300;
#endif
  std::string input = "./share/scram/input/benchmark/200_event.xml";
  settings.limit_order(17);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(8487, NumOfMcs());
  EXPECT_GT(McsGenerationTime(), mcs_time * (1 - delta));
  EXPECT_LT(McsGenerationTime(), mcs_time * (1 + delta));
}

// Test performance of MCS generation.
TEST_F(PerformanceTest, DISABLED_Baobab1_L6) {
  double mcs_time = 29.700;
#ifdef NDEBUG
  mcs_time = 5.670;
#endif
  std::string input = "./share/scram/input/benchmark/baobab1.xml";
  settings.limit_order(6);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(2684, NumOfMcs());
  EXPECT_GT(McsGenerationTime(), mcs_time * (1 - delta));
  EXPECT_LT(McsGenerationTime(), mcs_time * (1 + delta));
}

// Release only tests.
#ifdef NDEBUG
// Test performance of MCS generation.
TEST_F(PerformanceTest, DISABLED_Baobab1_L7) {
  double mcs_time = 87.300;
  std::string input = "./share/scram/input/benchmark/baobab1.xml";
  settings.limit_order(7);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(17432, NumOfMcs());
  EXPECT_GT(McsGenerationTime(), mcs_time * (1 - delta));
  EXPECT_LT(McsGenerationTime(), mcs_time * (1 + delta));
}

TEST_F(PerformanceTest, DISABLED_CEA9601_L4) {
  double mcs_time = 19.550;
  std::string input = "./share/scram/input/benchmark/CEA9601.xml";
  settings.limit_order(4);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(2732, NumOfMcs());
  EXPECT_GT(McsGenerationTime(), mcs_time * (1 - delta));
  EXPECT_LT(McsGenerationTime(), mcs_time * (1 + delta));
}

TEST_F(PerformanceTest, DISABLED_CEA9601_L5) {
  double mcs_time = 43.530;
  std::string input = "./share/scram/input/benchmark/CEA9601.xml";
  settings.limit_order(5);
  settings.num_sums(1);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_EQ(3274, NumOfMcs());
  EXPECT_GT(McsGenerationTime(), mcs_time * (1 - delta));
  EXPECT_LT(McsGenerationTime(), mcs_time * (1 + delta));
}
#endif
