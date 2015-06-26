#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for Baobab 1 fault tree from XFTA.
// Test Minimal cut sets.
TEST_F(RiskAnalysisTest, Baobab_1_Test) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/Baobab/baobab1.xml");
  input_files.push_back("./share/scram/input/Baobab/baobab1-basic-events.xml");
  settings.limit_order(6);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(ran->Analyze());
  // Minimal cut set check.
  EXPECT_EQ(2684, min_cut_sets().size());
  std::vector<int> distr(7, 0);
  distr[2] = 1;
  distr[3] = 1;
  distr[4] = 70;
  distr[5] = 400;
  distr[6] = 2212;
  EXPECT_EQ(distr, McsDistribution());
}
