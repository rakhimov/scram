#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for Chinese fault tree from XFTA.
// Test Minimal cut sets and probability.
/// @todo Test importance factors.
TEST_F(RiskAnalysisTest, ChineseTree) {
  std::string tree_input = "./share/scram/input/benchmark/chinese.xml";
  ran->AddSettings(settings.limit_order(6).num_sums(3));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_NO_THROW(ran->Report("/dev/null"));
  EXPECT_NEAR(0.0045691, p_total(), 1e-5);
  // Minimal cut set check.
  EXPECT_EQ(392, min_cut_sets().size());
  std::vector<int> distr(7, 0);
  distr[2] = 12;
  distr[4] = 24;
  distr[5] = 188;
  distr[6] = 168;
  EXPECT_EQ(distr, McsDistribution());
}
