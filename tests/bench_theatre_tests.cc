#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for Theatre fault tree from OpenFTA.
// Test Minimal cut sets and total probabilty.
TEST_F(RiskAnalysisTest, Theatre) {
  std::string tree_input = "./share/scram/input/Theatre/theatre.xml";
  std::string GEN_FAIL = "gen_fail";  // 2e-2
  std::string RELAY_FAIL = "relay_fail";  // 5e-2
  std::string MAINS_FAIL = "mains_fail";  // 3e-2
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.00207, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(GEN_FAIL);
  cut_set.insert(MAINS_FAIL);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(MAINS_FAIL);
  cut_set.insert(RELAY_FAIL);
  mcs.insert(cut_set);
  EXPECT_EQ(2, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}
