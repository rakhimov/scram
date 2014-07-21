#include <gtest/gtest.h>

#include "fault_tree_tests.h"

// Bechmark tests for NOT gate.
// [A OR NOT A]
TEST_F(FaultTreeTest, A_OR_NOT_A) {
  std::string tree_input = "./input/benchmark/a_or_not_a.scramf";
  std::string prob_input = "./input/benchmark/abc.scramp";
  std::string A = "a";  // 0.1
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
  EXPECT_DOUBLE_EQ(1, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(A);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert("not " + A);
  mcs.insert(cut_set);
  EXPECT_EQ(2, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// [A OR NOT B]
TEST_F(FaultTreeTest, A_OR_NOT_B) {
  std::string tree_input = "./input/benchmark/a_or_not_b.scramf";
  std::string prob_input = "./input/benchmark/abc.scramp";
  std::string A = "a";  // 0.1
  std::string B = "b";  // 0.2
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
  EXPECT_DOUBLE_EQ(0.82, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(A);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert("not " + B);
  mcs.insert(cut_set);
  EXPECT_EQ(2, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// [A AND NOT A]
TEST_F(FaultTreeTest, A_AND_NOT_A) {
  std::string tree_input = "./input/benchmark/a_and_not_a.scramf";
  std::string prob_input = "./input/benchmark/abc.scramp";
  std::string A = "a";  // 0.1
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
  EXPECT_DOUBLE_EQ(0, p_total());  // Total prob check.
  // Minimal cut set check.
  EXPECT_EQ(0, min_cut_sets().size());
}

// [A AND NOT B]
TEST_F(FaultTreeTest, A_AND_NOT_B) {
  std::string tree_input = "./input/benchmark/a_and_not_b.scramf";
  std::string prob_input = "./input/benchmark/abc.scramp";
  std::string A = "a";  // 0.1
  std::string B = "b";  // 0.2
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
  EXPECT_DOUBLE_EQ(0.08, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(A);
  cut_set.insert("not " + B);
  mcs.insert(cut_set);
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// [A OR (B, NOT A)]
TEST_F(FaultTreeTest, A_OR_NOT_AB) {
  std::string tree_input = "./input/benchmark/a_or_not_ab.scramf";
  std::string prob_input = "./input/benchmark/abc.scramp";
  std::string A = "a";  // 0.1
  std::string B = "b";  // 0.2
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
  EXPECT_DOUBLE_EQ(0.28, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(A);
  mcs.insert(cut_set);
  cut_set.clear();
  cut_set.insert("not " + A);
  cut_set.insert(B);
  mcs.insert(cut_set);
  EXPECT_EQ(2, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// [A OR NOT B] FTA MC
TEST_F(FaultTreeTest, MC_A_OR_NOT_B) {
  delete fta;
  fta = new FaultTree("fta-mc", false);
  std::string tree_input = "./input/benchmark/a_or_not_b.scramf";
  std::string prob_input = "./input/benchmark/abc.scramp";
  std::set< std::set<int> > p_terms;
  std::set< std::set<int> > n_terms;
  std::set<int> cut_set;
  std::set< std::set<int> > temp_sets;

  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
}
