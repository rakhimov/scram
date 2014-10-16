#include "fault_tree_analysis_tests.h"

#include "error.h"

TEST_F(FaultTreeAnalysisTest, NO_GATE) {
  std::vector<SupersetPtr> sets;

  // Testing for some UNKNOWN gate.
  SetUpGate("unknown_gate");
  inter->AddChild(A);
  inter->AddChild(B);
  inter->AddChild(C);
  GetIndices();
  EXPECT_THROW(ExpandSets(inter_id, &sets), ValueError);
  EXPECT_THROW(ExpandSets(-inter_id, &sets), ValueError);
}

TEST_F(FaultTreeAnalysisTest, OR_GATE) {
  std::vector<SupersetPtr> sets;
  std::vector<SupersetPtr>::iterator it_set;
  std::set<int> result_set;

  // Testing for OR gate.
  SetUpGate("or");
  inter->AddChild(A);
  inter->AddChild(B);
  inter->AddChild(C);
  inter->AddChild(D);
  GetIndices();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  EXPECT_EQ(4, sets.size());
  bool a_found = false;
  bool b_found = false;
  bool c_found = false;
  bool d_found = false;
  for (it_set = sets.begin(); it_set != sets.end(); ++it_set) {
    if (!(*it_set)->p_events().empty()) {
      std::set<int> result = (*it_set)->p_events();
      EXPECT_EQ(1, result.size());
      EXPECT_EQ(1, result.count(a_id) + result.count(b_id)
                + result.count(c_id));
      if (!a_found && result.count(a_id)) a_found = true;
      else if (!b_found && result.count(b_id)) b_found = true;
      else if (!c_found && result.count(c_id)) c_found = true;
    } else {
      std::set<int> result = (*it_set)->gates();
      EXPECT_EQ(1, result.size());
      EXPECT_EQ(1, result.count(d_id));
      d_found = true;
    }
  }
  EXPECT_EQ(true, a_found && b_found && c_found && d_found);
  // Negative OR gate.
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));
  EXPECT_EQ(1, sets.size());
  result_set = (*sets.begin())->p_events();
  EXPECT_EQ(3, result_set.size());
  EXPECT_EQ(1, result_set.count(-1 * a_id));
  EXPECT_EQ(1, result_set.count(-1 * b_id));
  EXPECT_EQ(1, result_set.count(-1 * c_id));
  result_set = (*sets.begin())->gates();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(-1 * d_id));
}

TEST_F(FaultTreeAnalysisTest, AND_GATE) {
  std::vector<SupersetPtr> sets;
  std::vector<SupersetPtr>::iterator it_set;
  std::set<int> result_set;

  // Testing for AND gate.
  SetUpGate("and");
  inter->AddChild(A);
  inter->AddChild(B);
  inter->AddChild(C);
  inter->AddChild(D);
  GetIndices();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  EXPECT_EQ(1, sets.size());
  result_set = (*sets.begin())->p_events();
  EXPECT_EQ(3, result_set.size());
  EXPECT_EQ(1, result_set.count(a_id));
  EXPECT_EQ(1, result_set.count(b_id));
  EXPECT_EQ(1, result_set.count(c_id));
  result_set = (*sets.begin())->gates();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(d_id));
  // Negative AND gate.
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));
  EXPECT_EQ(4, sets.size());
  bool a_found = false;
  bool b_found = false;
  bool c_found = false;
  bool d_found = false;
  for (it_set = sets.begin(); it_set != sets.end(); ++it_set) {
    if (!(*it_set)->p_events().empty()) {
      std::set<int> result = (*it_set)->p_events();
      EXPECT_EQ(1, result.size());
      EXPECT_EQ(1, result.count(-1 * a_id) + result.count(-1 * b_id)
                + result.count(-1 * c_id));
      if (!a_found && result.count(-1 * a_id)) a_found = true;
      else if (!b_found && result.count(-1 * b_id)) b_found = true;
      else if (!c_found && result.count(-1 * c_id)) c_found = true;
    } else {
      std::set<int> result = (*it_set)->gates();
      EXPECT_EQ(1, result.size());
      EXPECT_EQ(1, result.count(-1 * d_id));
      d_found = true;
    }
  }
  EXPECT_EQ(true, a_found && b_found && c_found && d_found);
}

TEST_F(FaultTreeAnalysisTest, NOT_GATE) {
  std::vector<SupersetPtr> sets;
  std::set<int> result_set;

  // Testing for NOT gate with a primary event child.
  SetUpGate("not");
  inter->AddChild(A);
  GetIndices();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  result_set = (*sets.begin())->p_events();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(-1 * a_id));
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));  // Negative Gate.
  result_set = (*sets.begin())->p_events();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(a_id));

  // Testing for NOT gate with a intermediate event child.
  // delete fta();
  new_fta(new FaultTreeAnalysis());
  SetUpGate("not");
  inter->AddChild(D);
  GetIndices();
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  result_set = (*sets.begin())->gates();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(-1 * d_id));
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));  // Negative Gate.
  result_set = (*sets.begin())->gates();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(d_id));
}

TEST_F(FaultTreeAnalysisTest, NOR_GATE) {
  std::vector<SupersetPtr> sets;
  std::vector<SupersetPtr>::iterator it_set;
  std::set<int> result_set;

  // Testing for NOR gate.
  SetUpGate("nor");
  inter->AddChild(A);
  inter->AddChild(B);
  inter->AddChild(C);
  inter->AddChild(D);
  GetIndices();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  EXPECT_EQ(1, sets.size());
  result_set = (*sets.begin())->p_events();
  EXPECT_EQ(3, result_set.size());
  EXPECT_EQ(1, result_set.count(-1 * a_id));
  EXPECT_EQ(1, result_set.count(-1 * b_id));
  EXPECT_EQ(1, result_set.count(-1 * c_id));
  result_set = (*sets.begin())->gates();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(-1 * d_id));
  // Negative NOR gate.
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));  // Negative Gate.
  EXPECT_EQ(4, sets.size());
  bool a_found = false;
  bool b_found = false;
  bool c_found = false;
  bool d_found = false;
  for (it_set = sets.begin(); it_set != sets.end(); ++it_set) {
    if (!(*it_set)->p_events().empty()) {
      std::set<int> result = (*it_set)->p_events();
      EXPECT_EQ(1, result.size());
      EXPECT_EQ(1, result.count(a_id) + result.count(b_id)
                + result.count(c_id));
      if (!a_found && result.count(a_id)) a_found = true;
      else if (!b_found && result.count(b_id)) b_found = true;
      else if (!c_found && result.count(c_id)) c_found = true;
    } else {
      std::set<int> result = (*it_set)->gates();
      EXPECT_EQ(1, result.size());
      EXPECT_EQ(1, result.count(d_id));
      d_found = true;
    }
  }
  EXPECT_EQ(true, a_found && b_found && c_found && d_found);
}

TEST_F(FaultTreeAnalysisTest, NAND_GATE) {
  std::vector<SupersetPtr> sets;
  std::vector<SupersetPtr>::iterator it_set;
  std::set<int> result_set;

  // Testing for NAND gate.
  SetUpGate("nand");
  inter->AddChild(A);
  inter->AddChild(B);
  inter->AddChild(C);
  inter->AddChild(D);
  GetIndices();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  EXPECT_EQ(4, sets.size());
  bool a_found = false;
  bool b_found = false;
  bool c_found = false;
  bool d_found = false;
  for (it_set = sets.begin(); it_set != sets.end(); ++it_set) {
    if (!(*it_set)->p_events().empty()) {
      std::set<int> result = (*it_set)->p_events();
      EXPECT_EQ(1, result.size());
      EXPECT_EQ(1, result.count(-1 * a_id) + result.count(-1 * b_id)
                + result.count(-1 * c_id));
      if (!a_found && result.count(-1 * a_id)) a_found = true;
      else if (!b_found && result.count(-1 * b_id)) b_found = true;
      else if (!c_found && result.count(-1 * c_id)) c_found = true;
    } else {
      std::set<int> result = (*it_set)->gates();
      EXPECT_EQ(1, result.size());
      EXPECT_EQ(1, result.count(-1 * d_id));
      d_found = true;
    }
  }
  EXPECT_EQ(true, a_found && b_found && c_found && d_found);
  // Negative NAND gate.
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));
  EXPECT_EQ(1, sets.size());
  result_set = (*sets.begin())->p_events();
  EXPECT_EQ(3, result_set.size());
  EXPECT_EQ(1, result_set.count(a_id));
  EXPECT_EQ(1, result_set.count(b_id));
  EXPECT_EQ(1, result_set.count(c_id));
  result_set = (*sets.begin())->gates();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(d_id));
}

TEST_F(FaultTreeAnalysisTest, XOR_GATE) {
  std::vector<SupersetPtr> sets;
  std::vector<SupersetPtr>::iterator it_set;
  std::set<int> result_set;

  // Testing for XOR gate.
  SetUpGate("xor");
  inter->AddChild(A);
  inter->AddChild(D);
  GetIndices();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  EXPECT_EQ(2, sets.size());
  std::set<int> set_one;
  std::set<int> set_two;
  std::set<int> result_one;
  std::set<int> result_two;
  set_one.insert(*(*sets.begin())->p_events().begin());
  set_one.insert(*(*sets.begin())->gates().begin());
  set_two.insert(*(*++sets.begin())->p_events().begin());
  set_two.insert(*(*++sets.begin())->gates().begin());
  result_one.insert(a_id);
  result_one.insert(-1 * d_id);
  result_two.insert(-1 * a_id);
  result_two.insert(d_id);
  ASSERT_TRUE(set_one.count(a_id) || set_one.count(-1 * a_id));
  if (set_one.count(a_id)) {
    ASSERT_EQ(result_one, set_one);
    ASSERT_EQ(result_two, set_two);
  } else {
    ASSERT_EQ(result_two, set_one);
    ASSERT_EQ(result_one, set_two);
  }
  // Negative XOR gate.
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));
  set_one.clear();
  set_two.clear();
  result_one.clear();
  result_two.clear();
  set_one.insert(*(*sets.begin())->p_events().begin());
  set_one.insert(*(*sets.begin())->gates().begin());
  set_two.insert(*(*++sets.begin())->p_events().begin());
  set_two.insert(*(*++sets.begin())->gates().begin());
  result_one.insert(a_id);
  result_one.insert(d_id);
  result_two.insert(-1 * a_id);
  result_two.insert(-1 * d_id);
  ASSERT_TRUE(set_one.count(a_id) || set_one.count(-1 * a_id));
  if (set_one.count(a_id)) {
    EXPECT_EQ(result_one, set_one);
    EXPECT_EQ(result_two, set_two);
  } else {
    EXPECT_EQ(result_two, set_one);
    EXPECT_EQ(result_one, set_two);
  }
}

TEST_F(FaultTreeAnalysisTest, NULL_GATE) {
  std::vector<SupersetPtr> sets;
  std::set<int> result_set;

  // Testing for NULL gate with a primary event child.
  SetUpGate("null");
  inter->AddChild(A);
  GetIndices();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  result_set = (*sets.begin())->p_events();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(a_id));
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));  // Negative Gate.
  result_set = (*sets.begin())->p_events();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(-1 * a_id));

  // Testing for NULL gate with a intermediate event child.
  // delete fta();
  new_fta(new FaultTreeAnalysis());
  SetUpGate("null");
  inter->AddChild(D);
  GetIndices();
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  result_set = (*sets.begin())->gates();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(d_id));
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));  // Negative Gate.
  result_set = (*sets.begin())->gates();
  EXPECT_EQ(1, result_set.size());
  EXPECT_EQ(1, result_set.count(-1 * d_id));
}

TEST_F(FaultTreeAnalysisTest, ATLEAST_GATE) {
  std::vector<SupersetPtr> sets;
  std::vector<SupersetPtr>::iterator it_set;

  // Testing for ATLEAST gate.
  SetUpGate("atleast");
  inter->AddChild(A);
  inter->AddChild(B);
  inter->AddChild(C);
  inter->AddChild(D);
  inter->vote_number(3);
  GetIndices();
  ASSERT_NO_THROW(ExpandSets(inter_id, &sets));
  EXPECT_EQ(4, sets.size());
  std::set< std::set<int> > output;
  std::set<int> mcs;
  for (it_set = sets.begin(); it_set != sets.end(); ++it_set) {
    mcs.insert((*it_set)->p_events().begin(), (*it_set)->p_events().end());
    mcs.insert((*it_set)->gates().begin(), (*it_set)->gates().end());
    output.insert(mcs);
    mcs.clear();
  }
  std::set< std::set<int> > exp;
  mcs.clear();
  mcs.insert(a_id);
  mcs.insert(b_id);
  mcs.insert(c_id);
  exp.insert(mcs);
  mcs.clear();
  mcs.insert(a_id);
  mcs.insert(b_id);
  mcs.insert(d_id);
  exp.insert(mcs);
  mcs.clear();
  mcs.insert(a_id);
  mcs.insert(c_id);
  mcs.insert(d_id);
  exp.insert(mcs);
  mcs.clear();
  mcs.insert(b_id);
  mcs.insert(c_id);
  mcs.insert(d_id);
  exp.insert(mcs);
  mcs.clear();

  EXPECT_EQ(exp, output);

  // Negative VOTE gate.
  sets.clear();
  ASSERT_NO_THROW(ExpandSets(-1 * inter_id, &sets));
  EXPECT_EQ(6, sets.size());
  output.clear();
  mcs.clear();
  for (it_set = sets.begin(); it_set != sets.end(); ++it_set) {
    mcs.insert((*it_set)->p_events().begin(), (*it_set)->p_events().end());
    mcs.insert((*it_set)->gates().begin(), (*it_set)->gates().end());
    output.insert(mcs);
    mcs.clear();
  }
  exp.clear();
  mcs.clear();
  mcs.insert(a_id * -1);
  mcs.insert(b_id * -1);
  exp.insert(mcs);
  mcs.clear();
  mcs.insert(a_id * -1);
  mcs.insert(d_id * -1);
  exp.insert(mcs);
  mcs.clear();
  mcs.insert(a_id * -1);
  mcs.insert(c_id * -1);
  exp.insert(mcs);
  mcs.clear();
  mcs.insert(b_id * -1);
  mcs.insert(d_id * -1);
  exp.insert(mcs);
  mcs.clear();
  mcs.insert(b_id * -1);
  mcs.insert(c_id * -1);
  exp.insert(mcs);
  mcs.clear();
  mcs.insert(c_id * -1);
  mcs.insert(d_id * -1);
  exp.insert(mcs);
  mcs.clear();

  EXPECT_EQ(exp, output);
}

// ---------------------- Test Public Functions --------------------------
// Invalid options for the constructor.
TEST_F(FaultTreeAnalysisTest, Constructor) {
  // Incorrect limit order for minmal cut sets.
  ASSERT_THROW(FaultTreeAnalysis(-1), ValueError);
}
