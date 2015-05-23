#include "probability_analysis_tests.h"

#include "error.h"

TEST_F(ProbabilityAnalysisTest, ProbAndInt) {
  std::set<int> min_cut_set;

  // 0 probability for an empty set.
  EXPECT_DOUBLE_EQ(0, ProbAnd(min_cut_set));

  AddPrimaryIntProb(0.0);  // Dummy element.

  min_cut_set.insert(1);
  AddPrimaryIntProb(0.1);
  EXPECT_DOUBLE_EQ(0.1, ProbAnd(min_cut_set));
  min_cut_set.insert(2);
  AddPrimaryIntProb(0.2);
  EXPECT_DOUBLE_EQ(0.02, ProbAnd(min_cut_set));
  min_cut_set.insert(3);
  AddPrimaryIntProb(0.3);
  EXPECT_DOUBLE_EQ(0.006, ProbAnd(min_cut_set));

  // Test for negative event calculations.
  min_cut_set.clear();
  min_cut_set.insert(-1);
  EXPECT_DOUBLE_EQ(0.9, ProbAnd(min_cut_set));
  min_cut_set.insert(-2);
  EXPECT_DOUBLE_EQ(0.72, ProbAnd(min_cut_set));
  min_cut_set.insert(3);
  EXPECT_DOUBLE_EQ(0.216, ProbAnd(min_cut_set));
}

TEST_F(ProbabilityAnalysisTest, CombineElAndSet) {
  std::set<int> el_one;
  std::set<int> el_two;
  std::set< std::set<int> > set_one;
  std::set< std::set<int> > set_two;
  std::set< std::set<int> > combo_set;

  // One element checks.
  el_one.insert(1);
  set_one.insert(el_one);  // Insert (1)
  ASSERT_NO_THROW(CombineElAndSet(el_one, set_one, &combo_set));
  EXPECT_EQ(set_one, combo_set);  // Must be only (1)
  combo_set.clear();

  el_two.insert(3);
  ASSERT_NO_THROW(CombineElAndSet(el_two, set_one, &combo_set));

  set_one.insert(el_two);  // Insert (3)

  EXPECT_EQ(1, combo_set.size());
  el_two.insert(1);
  set_two.insert(el_two);  // set_two is (1,3)
  EXPECT_EQ(set_two, combo_set);  // Must be only (1,3)
  combo_set.clear();

  // Two element checks.
  el_one.insert(2);  // el_one is (1, 2)
  ASSERT_NO_THROW(CombineElAndSet(el_one, set_two, &combo_set));

  set_one.insert(el_two);  // Insert (1, 3)

  el_two.insert(2);
  set_two.clear();
  set_two.insert(el_two);
  EXPECT_EQ(set_two, combo_set);  // Expected (1,2,3)
  combo_set.clear();

  // Multi-element checks
  set_one.insert(el_one);  // Insert (1, 2)

  // After the above instantiation the set_one is [(1), (3), (1,2), (1,3)].
  // The result of [ el_one AND set_one ] is [(1,2), (1,2,3)].
  EXPECT_EQ(4, set_one.size());
  EXPECT_EQ(2, el_one.size());
  EXPECT_EQ(0, combo_set.size());
  ASSERT_NO_THROW(CombineElAndSet(el_one, set_one, &combo_set));
  EXPECT_EQ(2, combo_set.size());
  set_one.clear();  // To construct the expected output set_one.
  set_one.insert(el_one);
  el_one.insert(3);
  set_one.insert(el_one);
  EXPECT_EQ(set_one, combo_set);

  // Testing for operations with negative elements.
  el_one.clear();
  el_two.clear();
  set_one.clear();
  set_two.clear();
  combo_set.clear();

  el_one.insert(-1);
  set_one.insert(el_one);
  ASSERT_NO_THROW(CombineElAndSet(el_one, set_one, &combo_set));
  EXPECT_EQ(set_one, combo_set);

  el_two.insert(1);
  combo_set.clear();
  ASSERT_NO_THROW(CombineElAndSet(el_two, set_one, &combo_set));
  EXPECT_TRUE(combo_set.empty());
}

TEST_F(ProbabilityAnalysisTest, ProbOrInt) {
  std::set<int> mcs;  // Minimal cut set.
  std::set<std::set<int> > min_cut_sets;  // A set of minimal cut sets.
  AddPrimaryIntProb(0.0);  // Dummy element.
  AddPrimaryIntProb(0.1);  // A is element 0.
  AddPrimaryIntProb(0.2);  // B is element 1.
  AddPrimaryIntProb(0.3);  // C is element 2.

  // 0 probability for an empty set.
  EXPECT_DOUBLE_EQ(0, ProbOr(&min_cut_sets));

  // Check for one element calculation for A.
  mcs.insert(1);
  min_cut_sets.insert(mcs);
  EXPECT_DOUBLE_EQ(0.1, ProbOr(&min_cut_sets));

  // Check that recursive nsums=0 returns without changing anything.
  mcs.insert(1);
  min_cut_sets.insert(mcs);
  EXPECT_EQ(0, ProbOr(0, &min_cut_sets));

  // Check for [A or B]
  min_cut_sets.clear();
  mcs.clear();
  mcs.insert(1);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(2);
  min_cut_sets.insert(mcs);
  EXPECT_DOUBLE_EQ(0.28, ProbOr(&min_cut_sets));

  // Check for [A or B or C]
  min_cut_sets.clear();
  mcs.clear();
  mcs.insert(1);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(2);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(3);
  min_cut_sets.insert(mcs);
  EXPECT_DOUBLE_EQ(0.496, ProbOr(&min_cut_sets));

  // Check for [(A,B) or (B,C)]
  mcs.clear();
  min_cut_sets.clear();
  mcs.insert(1);
  mcs.insert(2);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(2);
  mcs.insert(3);
  min_cut_sets.insert(mcs);
  EXPECT_DOUBLE_EQ(0.074, ProbOr(&min_cut_sets));
}

// ---------------------- Test Public Functions --------------------------
// Invalid options for the constructor.
TEST_F(ProbabilityAnalysisTest, Constructor) {
  // Incorrect approximation argument.
  ASSERT_THROW(ProbabilityAnalysis("approx"), InvalidArgument);
  // Incorrect number of series in the probability equation.
  ASSERT_NO_THROW(ProbabilityAnalysis("no"));
  ASSERT_NO_THROW(ProbabilityAnalysis("mcub"));
  ASSERT_NO_THROW(ProbabilityAnalysis("rare-event"));
  ASSERT_THROW(ProbabilityAnalysis("no", -1), InvalidArgument);
  // Incorrect cut-off probability.
  ASSERT_NO_THROW(ProbabilityAnalysis("no", 1));
  ASSERT_THROW(ProbabilityAnalysis("no", 1, -1), InvalidArgument);
  ASSERT_THROW(ProbabilityAnalysis("no", 1, 10), InvalidArgument);
}
