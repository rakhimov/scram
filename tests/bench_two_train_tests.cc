#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark Tests for an example fault tree with Two trains of Pumps and
// Valves.
// Test Minimal cut sets and total probabilty.
TEST_F(RiskAnalysisTest, TwoTrain) {
  std::string tree_input = "./share/scram/input/benchmark/two_train.xml";
  std::string ValveOne = "valveone";  // 0.5
  std::string ValveTwo = "valvetwo";  // 0.5
  std::string PumpOne = "pumpone";  // 0.7
  std::string PumpTwo = "pumptwo";  // 0.7
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.7225, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(ValveOne);
  cut_set.insert(ValveTwo);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(PumpOne);
  cut_set.insert(PumpTwo);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(PumpOne);
  cut_set.insert(ValveTwo);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(ValveOne);
  cut_set.insert(PumpTwo);
  mcs.insert(cut_set);
  EXPECT_EQ(4, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}
