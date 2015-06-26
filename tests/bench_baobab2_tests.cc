#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for Baobab 2 fault tree from XFTA.
// Test Minimal cut sets.
TEST_F(RiskAnalysisTest, Baobab_2_Test) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/Baobab/baobab2.xml");
  input_files.push_back("./share/scram/input/Baobab/baobab2-basic-events.xml");
  settings.limit_order(3);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(ran->Analyze());
  // Minimal cut set check.
  EXPECT_EQ(127, min_cut_sets().size());
  std::vector<int> distr(4, 0);
  distr[2] = 6;
  distr[3] = 121;
  EXPECT_EQ(distr, McsDistribution());
}
