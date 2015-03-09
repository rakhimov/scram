#include <gtest/gtest.h>

#include <boost/assign/list_inserter.hpp>

#include "risk_analysis_tests.h"

// Benchmark Tests for Beta factor common cause failure model.
// Test Minimal cut sets and total probabilty.
TEST_F(RiskAnalysisTest, BetaFactorCCF) {
  std::string tree_input = "./share/scram/input/benchmark/beta_factor_ccf.xml";
  std::string p1 = "[pumpone]";
  std::string p2 = "[pumptwo]";
  std::string p3 = "[pumpthree]";
  std::string v1 = "[valveone]";
  std::string v2 = "[valvetwo]";
  std::string v3 = "[valvethree]";
  std::string pumps = "[pumpone pumpthree pumptwo]";
  std::string valves = "[valveone valvethree valvetwo]";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.ccf_analysis(true).probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(0.04308, p_total(), 1e-5);  // Total prob check.
  // Minimal cut set check.
  using namespace boost::assign;
  insert(cut_set) (pumps);
  mcs.insert(cut_set);
  cut_set.clear();
  insert(cut_set) (valves);
  mcs.insert(cut_set);
  cut_set.clear();
  insert(cut_set) (v1) (v2) (v3);
  mcs.insert(cut_set);
  cut_set.clear();
  insert(cut_set) (p1) (v2) (v3);
  mcs.insert(cut_set);
  cut_set.clear();
  insert(cut_set) (p2) (v1) (v3);
  mcs.insert(cut_set);
  cut_set.clear();
  insert(cut_set) (p3) (v1) (v2);
  mcs.insert(cut_set);
  cut_set.clear();
  insert(cut_set) (p3) (p2) (v1);
  mcs.insert(cut_set);
  cut_set.clear();
  insert(cut_set) (p1) (p2) (v3);
  mcs.insert(cut_set);
  cut_set.clear();
  insert(cut_set) (p1) (p3) (v2);
  mcs.insert(cut_set);
  cut_set.clear();
  insert(cut_set) (p1) (p2) (p3);
  mcs.insert(cut_set);
  EXPECT_EQ(10, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// Benchmark Tests for Phi factor common cause failure calculations.
// Test Minimal cut sets and total probabilty.
TEST_F(RiskAnalysisTest, PhiFactorCCF) {
  std::string tree_input = "./share/scram/input/benchmark/phi_factor_ccf.xml";
  ASSERT_NO_THROW(ran->AddSettings(settings.ccf_analysis(true).num_sums(3)
                                           .probability_analysis(true)));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(0.04109, p_total(), 1e-5);  // Total prob check.
  EXPECT_EQ(34, min_cut_sets().size());
  std::vector<int> distr(4, 0);
  distr[1] = 2;
  distr[2] = 24;
  distr[3] = 8;
  EXPECT_EQ(distr, McsDistribution());
}

// Benchmark Tests for MGL factor common cause failure calculations.
// Test Minimal cut sets and total probabilty.
TEST_F(RiskAnalysisTest, MGLFactorCCF) {
  std::string tree_input = "./share/scram/input/benchmark/mgl_ccf.xml";
  ASSERT_NO_THROW(ran->AddSettings(settings.ccf_analysis(true).num_sums(3)
                                           .probability_analysis(true)));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(0.01631, p_total(), 1e-5);  // Total prob check.
  EXPECT_EQ(34, min_cut_sets().size());
  std::vector<int> distr(4, 0);
  distr[1] = 2;
  distr[2] = 24;
  distr[3] = 8;
  EXPECT_EQ(distr, McsDistribution());
}

// Benchmark Tests for Alpha factor common cause failure calculations.
// Test Minimal cut sets and total probabilty.
TEST_F(RiskAnalysisTest, AlphaFactorCCF) {
  std::string tree_input = "./share/scram/input/benchmark/alpha_factor_ccf.xml";
  ASSERT_NO_THROW(ran->AddSettings(settings.ccf_analysis(true).num_sums(3)
                                           .probability_analysis(true)));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(0.03093, p_total(), 1e-5);  // Total prob check.
  EXPECT_EQ(34, min_cut_sets().size());
  std::vector<int> distr(4, 0);
  distr[1] = 2;
  distr[2] = 24;
  distr[3] = 8;
  EXPECT_EQ(distr, McsDistribution());
}
