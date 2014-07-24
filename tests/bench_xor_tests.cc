#include <gtest/gtest.h>

#include "fault_tree_tests.h"

// Benchmark Tests for [A xor B xor C] fault tree.
// Test Minimal cut sets and total probabilty.
TEST_F(FaultTreeTest, XOR_ABC) {
  std::string tree_input = "./input/benchmark/xor.scramf";
  std::string prob_input = "./input/benchmark/abc.scramp";
  std::string A = "a";  // 0.1
  std::string B = "b";  // 0.2
  std::string C = "c";  // 0.3
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
  EXPECT_DOUBLE_EQ(0.404, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(A);
  cut_set.insert(B);
  cut_set.insert(C);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(A);
  cut_set.insert("not " + B);
  cut_set.insert("not " + C);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert("not " + A);
  cut_set.insert(B);
  cut_set.insert("not " + C);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert("not " + A);
  cut_set.insert("not " + B);
  cut_set.insert(C);
  mcs.insert(cut_set);
  EXPECT_EQ(4, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}
