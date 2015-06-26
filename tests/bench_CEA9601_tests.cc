#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for CEA9601 fault tree from XFTA.
// Test Minimal cut sets.
TEST_F(RiskAnalysisTest, CEA9601_Test) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/CEA9601/CEA9601.xml");
  input_files.push_back("./share/scram/input/CEA9601/CEA9601-basic-events.xml");
  settings.limit_order(4);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(ran->Analyze());
  // Minimal cut set check.
  EXPECT_EQ(2732, min_cut_sets().size());
  std::vector<int> distr(5, 0);
  distr[3] = 858;
  distr[4] = 1874;
  EXPECT_EQ(distr, McsDistribution());
}
