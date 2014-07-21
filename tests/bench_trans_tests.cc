#include <gtest/gtest.h>

#include "fault_tree_tests.h"

// Benchmark Tests for Trans Test fault tree from OpenFTA.
// This tests transfer gates.
TEST_F(FaultTreeTest, TransTest) {
  std::string trans_tree_input = "./input/benchmark/trans_one.scramf";
  std::string full_tree_input = "./input/benchmark/trans_full.scramf";
  std::string prob_input = "./input/benchmark/trans_full.scramp";
  std::string A = "a";  // 2e-2
  std::string B = "b";  // 4e-2
  std::string C = "c";  // 3e-2
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  // Check the tree with the transfer gate.
  ASSERT_NO_THROW(fta->ProcessInput(trans_tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
  EXPECT_DOUBLE_EQ(2.4e-5, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(A);
  cut_set.insert(B);
  cut_set.insert(C);
  mcs.insert(cut_set);
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());

  // Check the full tree without the transfer gate.
  delete fta;
  fta = new FaultTree("fta-default", false);
  ASSERT_NO_THROW(fta->ProcessInput(trans_tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
  EXPECT_DOUBLE_EQ(2.4e-5, p_total());  // Total prob check.
  // Minimal cut set check.
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}
