#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for CEA9601 fault tree from XFTA.
// Test Minimal cut sets.
TEST_F(RiskAnalysisTest, CEA9601_Test) {
  std::string tree_input = "./share/scram/input/benchmark/CEA9601.xml";
  ran->AddSettings(settings.limit_order(4));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_NO_THROW(ran->Report("/dev/null"));
  // Minimal cut set check.
  EXPECT_EQ(2732, min_cut_sets().size());
  std::vector<int> distr(5, 0);
  distr[3] = 858;
  distr[4] = 1874;
  EXPECT_EQ(distr, McsDistribution());
}
