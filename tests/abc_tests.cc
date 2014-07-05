#include <gtest/gtest.h>

#include "fault_tree_tests.h"

// Benchmark Tests for [A or B or C] fault tree.
// Test Minimal cut sets.
TEST_F(FaultTreeTest, ABC) {
  std::string tree_input = "./input/benchmark/abc.scramf";
  std::string prob_input = "./input/benchmark/abc.scramp";
  std::string A = "a";
  std::string B = "b";
  std::string C = "c";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  EXPECT_DOUBLE_EQ(0.496, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(A);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(B);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(C);
  mcs.insert(cut_set);
  EXPECT_EQ(3, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}
// Test Probabilty.
