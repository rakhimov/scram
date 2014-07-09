#include <gtest/gtest.h>

#include "fault_tree_tests.h"

// Benchmark Tests for an example fault tree given in NE574 Risk Analysis
// class at UW-Madison.
// Test Minimal cut sets and total probabilty.
TEST_F(FaultTreeTest, ne574) {
  std::string tree_input = "./input/benchmark/ne574.scramf";
  std::string prob_input = "./input/benchmark/ne574.scramp";
  std::string B = "b";  // 0.1
  std::string C = "c";  // 0.3
  std::string D = "d";  // 0.5
  std::string F = "f";  // 0.7
  std::string G = "g";  // 0.2
  std::string H = "h";  // 0.4
  std::string I = "i";  // 0.8
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  EXPECT_DOUBLE_EQ(0.662208, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(C);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(D);
  cut_set.insert(F);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(D);
  cut_set.insert(G);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(D);
  cut_set.insert(B);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(H);
  cut_set.insert(I);
  cut_set.insert(F);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(H);
  cut_set.insert(I);
  cut_set.insert(G);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert(H);
  cut_set.insert(I);
  cut_set.insert(B);
  mcs.insert(cut_set);
  EXPECT_EQ(7, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}
