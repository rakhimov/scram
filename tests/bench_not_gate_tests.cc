#include <gtest/gtest.h>

#include "risk_analysis_tests.h"

// Benchmark tests for NOT gate.
// [A OR NOT A]
// This produces UNITY top gate.
TEST_F(RiskAnalysisTest, A_OR_NOT_A) {
  std::string tree_input = "./share/scram/input/benchmark/a_or_not_a.xml";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(1, p_total());  // Total prob check.
  // Minimal cut set check.
  // Special case of one empty cut set in a container.
  mcs.insert(cut_set);
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// [A OR NOT B]
TEST_F(RiskAnalysisTest, A_OR_NOT_B) {
  std::string tree_input = "./share/scram/input/benchmark/a_or_not_b.xml";
  std::string A = "a";  // 0.1
  std::string B = "b";  // 0.2
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
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
TEST_F(RiskAnalysisTest, A_AND_NOT_A) {
  std::string tree_input = "./share/scram/input/benchmark/a_and_not_a.xml";
  std::string A = "a";  // 0.1
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0, p_total());  // Total prob check.
  // Minimal cut set check.
  EXPECT_EQ(0, min_cut_sets().size());
}

// [A AND NOT B]
TEST_F(RiskAnalysisTest, A_AND_NOT_B) {
  std::string tree_input = "./share/scram/input/benchmark/a_and_not_b.xml";
  std::string A = "a";  // 0.1
  std::string B = "b";  // 0.2
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.08, p_total());  // Total prob check.
  // Minimal cut set check.
  cut_set.insert(A);
  cut_set.insert("not " + B);
  mcs.insert(cut_set);
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// [A OR (B, NOT A)]
TEST_F(RiskAnalysisTest, A_OR_NOT_AB) {
  std::string tree_input = "./share/scram/input/benchmark/a_or_not_ab.xml";
  std::string A = "a";  // 0.1
  std::string B = "b";  // 0.2
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
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

// Uncertainty report for Unity case.
TEST_F(RiskAnalysisTest, MC_A_OR_NOT_A) {
  std::string tree_input = "./share/scram/input/benchmark/a_or_not_a.xml";
  ran->AddSettings(settings.uncertainty_analysis(true));
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
}

// [A OR NOT B] FTA MC
TEST_F(RiskAnalysisTest, MC_A_OR_NOT_B) {
  ran->AddSettings(settings.uncertainty_analysis(true));
  std::string tree_input = "./share/scram/input/benchmark/a_or_not_b.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
}

// Repeated negative gate expansion.
TEST_F(RiskAnalysisTest, MultipleParentNegativeGate) {
  std::string tree_input = "./share/scram/input/benchmark/"
                           "multiple_parent_negative_gate.xml";
  std::string A = "a";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  // Minimal cut set check.
  cut_set.insert("not " + A);
  mcs.insert(cut_set);
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// Checks for NAND UNITY top gate cases.
TEST_F(RiskAnalysisTest, NAND_UNITY) {
  std::string tree_input = "./share/scram/input/benchmark/nand_or_equality.xml";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(1, p_total());  // Total prob check.
  // Minimal cut set check.
  // Special case of one empty cut set in a container.
  mcs.insert(cut_set);
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// Checks for OR UNITY top gate cases.
TEST_F(RiskAnalysisTest, OR_UNITY) {
  std::string tree_input =
      "./share/scram/input/benchmark/not_and_or_equality.xml";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(1, p_total());  // Total prob check.
  // Minimal cut set check.
  // Special case of one empty cut set in a container.
  mcs.insert(cut_set);
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// Checks for UNITY due to house event.
TEST_F(RiskAnalysisTest, HOUSE_UNITY) {
  std::string tree_input =
      "./share/scram/input/benchmark/unity.xml";
  std::set<std::string> cut_set;
  std::set< std::set<std::string> > mcs;  // For expected min cut sets.

  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->GraphingInstructions("/dev/null"));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(1, p_total());  // Total prob check.
  // Minimal cut set check.
  // Special case of one empty cut set in a container.
  mcs.insert(cut_set);
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(mcs, min_cut_sets());
}

// Checks for NULL due to house event.
TEST_F(RiskAnalysisTest, HOUSE_NULL) {
  std::string tree_input =
      "./share/scram/input/benchmark/null.xml";
  ran->AddSettings(settings.probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->GraphingInstructions("/dev/null"));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0, p_total());  // Total prob check.
  // Minimal cut set check.
  // Special case of one empty cut set in a container.
  EXPECT_EQ(0, min_cut_sets().size());
}
