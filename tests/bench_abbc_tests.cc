#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for [AB or BC] fault tree.
// Test Minimal cut sets and total probability.
TEST_F(RiskAnalysisTest, AB_BC) {
  std::string tree_input = "./share/scram/input/benchmark/ab_bc.xml";
  std::string A = "a";  // 0.1
  std::string B = "b";  // 0.2
  std::string C = "c";  // 0.3
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.074, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(A);
  cut_set.insert(B);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(B);
  cut_set.insert(C);
  mcs.insert(cut_set);
  EXPECT_EQ(2, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}
