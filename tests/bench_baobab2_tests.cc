#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for Baobab 2 fault tree from XFTA.
// Test Minimal cut sets.
TEST_F(RiskAnalysisTest, Baobab_2_Test) {
  std::string tree_input = "./share/scram/input/benchmark/baobab2.xml";
  ran->AddSettings(settings.limit_order(3));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  // Minimal cut set check.
  EXPECT_EQ(127, min_cut_sets().size());
  std::vector<int> distr(4, 0);
  distr[2] = 6;
  distr[3] = 121;
  EXPECT_EQ(distr, McsDistribution());
}
