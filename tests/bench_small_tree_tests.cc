#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for Small Tree fault tree from XFTA.
// This benchmark is for uncertainty analysis.
TEST_F(RiskAnalysisTest, SmallTree) {
  std::string tree_input = "./share/scram/input/benchmark/SmallTree.xml";
  std::string e1 = "e1";
  std::string e2 = "e2";
  std::string e3 = "e3";
  std::string e4 = "e4";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.
  settings.uncertainty_analysis(true);
  ran->AddSettings(settings);
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  // Minimal cut set check.
  cut_set.insert(e1);
  cut_set.insert(e2);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(e3);
  cut_set.insert(e4);
  mcs.insert(cut_set);
  EXPECT_EQ(2, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
  double delta_sqr = std::abs(mean() - 0.02495);
  EXPECT_TRUE(delta_sqr < 5e-3);
  delta_sqr = std::abs(sigma() - 0.023625);
  EXPECT_TRUE(delta_sqr < 5e-3);
}
