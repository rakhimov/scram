#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for Chinese fault tree from XFTA.
// Test Minimal cut sets and probability.
/// @todo Test importance factors.
TEST_F(RiskAnalysisTest, ChineseTree) {
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/Chinese/chinese.xml");
  input_files.push_back("./share/scram/input/Chinese/chinese-basic-events.xml");
  settings.probability_analysis(true).limit_order(6).num_sums(3);
  ASSERT_NO_THROW(ProcessInputFiles(input_files));
  ASSERT_NO_THROW(ran->Analyze());
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
