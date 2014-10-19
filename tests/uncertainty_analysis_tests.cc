#include "uncertainty_analysis_tests.h"

#include "error.h"

TEST_F(UncertaintyAnalysisTest, CombineElAndSet) {
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

  // Multi element checks
  set_one.insert(el_one);  // Insert (1, 2)

  // After the above intantiation the set_one is [(1), (3), (1,2), (1,3)].
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

// ------------------------ Monte Carlo -----------------------------
TEST_F(UncertaintyAnalysisTest, MProbOr) {
  std::set<int> mcs;  // Minimal cut set.
  std::set< std::set<int> > p_terms;  // Positive terms of the equation.
  std::set< std::set<int> > n_terms;  // Negative terms of the equation.
  std::set< std::set<int> > temp_set;  // Temp set for dumping the output.
  std::set< std::set<int> > min_cut_sets;  // A set of minimal cut sets.
  ASSERT_NO_THROW(MProbOr(&min_cut_sets));  // Empty sets.
  ASSERT_TRUE(pos_terms().empty() && neg_terms().empty());

  // Check for one element calculation for A.
  mcs.insert(0);
  p_terms.insert(mcs);
  min_cut_sets.insert(mcs);
  ASSERT_NO_THROW(MProbOr(&min_cut_sets));
  EXPECT_EQ(0, min_cut_sets.size());  // This set is emptied recursively.
  temp_set.insert(pos_terms().begin(), pos_terms().end());
  EXPECT_EQ(p_terms, temp_set);

  // Check that recursive nsums=0 returns without changing anything.
  min_cut_sets.insert(mcs);
  pos_terms().clear();
  ASSERT_NO_THROW(MProbOr(1, 0, &min_cut_sets));
  EXPECT_EQ(1, min_cut_sets.size());
  EXPECT_EQ(0, pos_terms().size());

  // Check for [A or B]
  pos_terms().clear();
  neg_terms().clear();
  min_cut_sets.clear();
  mcs.clear();
  p_terms.clear();
  mcs.insert(0);
  p_terms.insert(mcs);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(1);
  p_terms.insert(mcs);
  min_cut_sets.insert(mcs);
  mcs.insert(0);
  n_terms.insert(mcs);
  ASSERT_NO_THROW(MProbOr(&min_cut_sets));
  temp_set.clear();
  temp_set.insert(pos_terms().begin(), pos_terms().end());
  EXPECT_EQ(p_terms, temp_set);
  temp_set.clear();
  temp_set.insert(neg_terms().begin(), neg_terms().end());
  EXPECT_EQ(n_terms, temp_set);

  // Check for [(A,B) or (B,C)]
  pos_terms().clear();
  neg_terms().clear();
  min_cut_sets.clear();
  mcs.clear();
  p_terms.clear();
  n_terms.clear();
  pos_terms().clear();
  neg_terms().clear();
  mcs.insert(0);
  mcs.insert(1);
  p_terms.insert(mcs);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(1);
  mcs.insert(2);
  p_terms.insert(mcs);
  min_cut_sets.insert(mcs);
  mcs.insert(0);
  n_terms.insert(mcs);
  ASSERT_NO_THROW(MProbOr(&min_cut_sets));
  temp_set.clear();
  temp_set.insert(pos_terms().begin(), pos_terms().end());
  EXPECT_EQ(p_terms, temp_set);
  temp_set.clear();
  temp_set.insert(neg_terms().begin(), neg_terms().end());
  EXPECT_EQ(n_terms, temp_set);
}
// ----------------------------------------------------------------------
// ---------------------- Test Public Functions --------------------------
// Invalid options for the constructor.
TEST_F(UncertaintyAnalysisTest, Constructor) {
  ASSERT_NO_THROW(UncertaintyAnalysis(1));
  // Incorrect number of series in the probability equation.
  ASSERT_THROW(UncertaintyAnalysis(-1), InvalidArgument);
  // Invalid cut-off probability.
  ASSERT_NO_THROW(UncertaintyAnalysis(1, 1));
  ASSERT_THROW(UncertaintyAnalysis(1, -1), InvalidArgument);
  // Invalid number of trials.
  ASSERT_NO_THROW(UncertaintyAnalysis(1, 1, 100));
  ASSERT_THROW(UncertaintyAnalysis(1, 1, -1), InvalidArgument);
}
