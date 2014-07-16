#include "fault_tree_tests.h"
#include "superset.h"

using namespace scram;

// ---------------------- Test Private Functions -------------------------
// Test the function that gets arguments from a line in an input file.
TEST_F(FaultTreeTest, GetArgs) {
  std::string line = "";  // An empty line.
  std::string orig_line = "";
  std::vector<std::string> args;
  EXPECT_FALSE(GetArgs_(args, line, orig_line));  // Test for empty line.
  EXPECT_EQ(line, "");
  line = "# This is a comment";
  EXPECT_FALSE(GetArgs_(args, line, orig_line));  // Test for comments.
  line = "  Arg_1 Arg_2 ";  // Some valid arguments.
  EXPECT_TRUE(GetArgs_(args, line, orig_line));
  EXPECT_EQ("Arg_1 Arg_2", orig_line);
  EXPECT_EQ("arg_1 arg_2", line);
  EXPECT_EQ("arg_1", args[0]);
  EXPECT_EQ("arg_2", args[1]);
  line = "  Arg  # comments.";  // Inline comments.
  EXPECT_TRUE(GetArgs_(args, line, orig_line));
  EXPECT_EQ("Arg", orig_line);
  EXPECT_EQ("arg", line);
  EXPECT_EQ("arg", args[0]);
}

TEST_F(FaultTreeTest, CheckGate) {
  TopEventPtr top(new TopEvent("top", "and"));  // AND gate.
  PrimaryEventPtr A(new PrimaryEvent("a"));
  PrimaryEventPtr B(new PrimaryEvent("b"));
  PrimaryEventPtr C(new PrimaryEvent("c"));

  // AND Gate tests.
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));  // One child is not enough.
  top->AddChild(B);
  EXPECT_TRUE(CheckGate(top));  // Two children are enough.
  top->AddChild(C);
  EXPECT_TRUE(CheckGate(top));  // More children are OK.

  // OR Gate tests.
  top = TopEventPtr(new TopEvent("top", "or"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));  // One child is not enough.
  top->AddChild(B);
  EXPECT_TRUE(CheckGate(top));  // Two children are enough.
  top->AddChild(C);
  EXPECT_TRUE(CheckGate(top));  // More children are OK.

  // Some UNKNOWN gate tests.
  top = TopEventPtr(new TopEvent("top", "unknown_gate"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));
  top->AddChild(B);
  EXPECT_FALSE(CheckGate(top));
}

TEST_F(FaultTreeTest, ExpandSets) {
  InterEventPtr inter(new InterEvent("inter"));  // No gate is defined.
  inter_events().insert(std::make_pair("inter", inter));
  std::vector< SupersetPtr > sets;
  std::vector< SupersetPtr >::iterator it_set;
  EXPECT_THROW(ExpandSets(inter, sets), ValueError);
  PrimaryEventPtr A(new PrimaryEvent("a"));
  PrimaryEventPtr B(new PrimaryEvent("b"));
  PrimaryEventPtr C(new PrimaryEvent("c"));
  primary_events().insert(std::make_pair("a", A));
  primary_events().insert(std::make_pair("b", B));
  primary_events().insert(std::make_pair("c", C));

  // Testing for OR gate.
  inter->gate("or");
  inter->AddChild(A);
  inter->AddChild(B);
  inter->AddChild(C);
  ASSERT_NO_THROW(ExpandSets(inter, sets));
  EXPECT_EQ(3, sets.size());
  bool a_found = false;
  bool b_found = false;
  bool c_found = false;
  for (it_set = sets.begin(); it_set != sets.end(); ++it_set) {
    std::set<std::string> result = (*it_set)->primes();
    EXPECT_EQ(1, result.size());
    EXPECT_EQ(1, result.count("a") + result.count("b") + result.count("c"));
    if (!a_found && result.count("a")) a_found = true;
    else if (!b_found && result.count("b")) b_found = true;
    else if (!c_found && result.count("c")) c_found = true;
  }
  EXPECT_EQ(true, a_found && b_found && c_found);

  // Testing for AND gate.
  inter = InterEventPtr(new InterEvent("inter", "and"));
  sets.clear();
  inter->AddChild(A);
  inter->AddChild(B);
  inter->AddChild(C);
  ASSERT_NO_THROW(ExpandSets(inter, sets));
  EXPECT_EQ(1, sets.size());
  std::set<std::string> result = (*sets.begin())->primes();
  EXPECT_EQ(3, result.size());
  EXPECT_EQ(1, result.count("a"));
  EXPECT_EQ(1, result.count("b"));
  EXPECT_EQ(1, result.count("c"));

  // Testing for some UNKNOWN gate.
  inter = InterEventPtr(new InterEvent("inter", "unknown_gate"));
  sets.clear();
  inter->AddChild(A);
  inter->AddChild(B);
  inter->AddChild(C);
  ASSERT_THROW(ExpandSets(inter, sets), ValueError);
}

TEST_F(FaultTreeTest, ProbAndString) {
  std::set<std::string> min_cut_set;
  ASSERT_THROW(ProbAnd(min_cut_set), ValueError);  // Error for an empty set.

  PrimaryEventPtr A(new PrimaryEvent("a"));
  PrimaryEventPtr B(new PrimaryEvent("b"));
  PrimaryEventPtr C(new PrimaryEvent("c"));
  A->p(0.1);
  B->p(0.2);
  C->p(0.3);
  primary_events().insert(std::make_pair("a", A));
  primary_events().insert(std::make_pair("b", B));
  primary_events().insert(std::make_pair("c", C));

  min_cut_set.insert("a");
  EXPECT_DOUBLE_EQ(0.1, ProbAnd(min_cut_set));
  min_cut_set.insert("b");
  EXPECT_DOUBLE_EQ(0.02, ProbAnd(min_cut_set));
  min_cut_set.insert("c");
  EXPECT_DOUBLE_EQ(0.006, ProbAnd(min_cut_set));
}

TEST_F(FaultTreeTest, ProbAndInt) {
  std::set<int> min_cut_set;
  ASSERT_THROW(ProbAnd(min_cut_set), ValueError);  // Error for an empty set.

  min_cut_set.insert(0);
  AddPrimeIntProb(0.1);
  EXPECT_DOUBLE_EQ(0.1, ProbAnd(min_cut_set));
  min_cut_set.insert(1);
  AddPrimeIntProb(0.2);
  EXPECT_DOUBLE_EQ(0.02, ProbAnd(min_cut_set));
  min_cut_set.insert(2);
  AddPrimeIntProb(0.3);
  EXPECT_DOUBLE_EQ(0.006, ProbAnd(min_cut_set));
}

TEST_F(FaultTreeTest, CombineElAndSet) {
  std::set<int> el_one;
  std::set<int> el_two;
  std::set< std::set<int> > set_one;
  std::set< std::set<int> > set_two;
  std::set< std::set<int> > combo_set;

  // One element checks.
  el_one.insert(1);
  set_one.insert(el_one);  // Insert (1)
  ASSERT_NO_THROW(CombineElAndSet(el_one, set_one, combo_set));
  EXPECT_EQ(set_one, combo_set);  // Must be only (1)
  combo_set.clear();

  el_two.insert(3);
  ASSERT_NO_THROW(CombineElAndSet(el_two, set_one, combo_set));

  set_one.insert(el_two);  // Insert (3)

  EXPECT_EQ(1, combo_set.size());
  el_two.insert(1);
  set_two.insert(el_two);  // set_two is (1,3)
  EXPECT_EQ(set_two, combo_set);  // Must be only (1,3)
  combo_set.clear();

  // Two element checks.
  el_one.insert(2);  // el_one is (1, 2)
  ASSERT_NO_THROW(CombineElAndSet(el_one, set_two, combo_set));

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
  ASSERT_NO_THROW(CombineElAndSet(el_one, set_one, combo_set));
  EXPECT_EQ(2, combo_set.size());
  set_one.clear();  // To construct the expected output set_one.
  set_one.insert(el_one);
  el_one.insert(3);
  set_one.insert(el_one);
  EXPECT_EQ(set_one, combo_set);
}

TEST_F(FaultTreeTest, ProbOrInt) {
  std::set<int> mcs;  // Minimal cut set.
  std::set<std::set<int> > min_cut_sets;  // A set of minimal cut sets.
  ASSERT_THROW(ProbOr(min_cut_sets), ValueError);  // Error for an empty set.
  AddPrimeIntProb(0.1);  // A is element 0.
  AddPrimeIntProb(0.2);  // B is element 1.
  AddPrimeIntProb(0.3);  // C is element 2.

  // Check for one element calculation for A.
  mcs.insert(0);
  min_cut_sets.insert(mcs);
  EXPECT_DOUBLE_EQ(0.1, ProbOr(min_cut_sets));

  // Check that recursive nsums=0 returns without changing anything.
  mcs.insert(0);
  min_cut_sets.insert(mcs);
  EXPECT_EQ(0, ProbOr(min_cut_sets, 0));

  // Check for [A or B]
  min_cut_sets.clear();
  mcs.clear();
  mcs.insert(0);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(1);
  min_cut_sets.insert(mcs);
  EXPECT_DOUBLE_EQ(0.28, ProbOr(min_cut_sets));

  // Check for [A or B or C]
  min_cut_sets.clear();
  mcs.clear();
  mcs.insert(0);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(1);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(2);
  min_cut_sets.insert(mcs);
  EXPECT_DOUBLE_EQ(0.496, ProbOr(min_cut_sets));

  // Check for [(A,B) or (B,C)]
  mcs.clear();
  min_cut_sets.clear();
  mcs.insert(0);
  mcs.insert(1);
  min_cut_sets.insert(mcs);
  mcs.clear();
  mcs.insert(1);
  mcs.insert(2);
  min_cut_sets.insert(mcs);
  EXPECT_DOUBLE_EQ(0.074, ProbOr(min_cut_sets));
}

// ------------------------ Monte Carlo -----------------------------
TEST_F(FaultTreeTest, MCombineElAndSet) {
  std::set<int> el_one;
  std::set<int> el_two;
  std::set< std::set<int> > set_one;
  std::set< std::set<int> > set_two;
  std::set< std::set<int> > combo_set;

  // One element checks.
  el_one.insert(1);
  set_one.insert(el_one);  // Insert (1)
  ASSERT_NO_THROW(MCombineElAndSet(el_one, set_one, combo_set));
  EXPECT_EQ(set_one, combo_set);  // Must be only (1)
  combo_set.clear();

  el_two.insert(3);
  ASSERT_NO_THROW(MCombineElAndSet(el_two, set_one, combo_set));

  set_one.insert(el_two);  // Insert (3)

  EXPECT_EQ(1, combo_set.size());
  el_two.insert(1);
  set_two.insert(el_two);  // set_two is (1,3)
  EXPECT_EQ(set_two, combo_set);  // Must be only (1,3)
  combo_set.clear();

  // Two element checks.
  el_one.insert(2);  // el_one is (1, 2)
  ASSERT_NO_THROW(MCombineElAndSet(el_one, set_two, combo_set));

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
  ASSERT_NO_THROW(MCombineElAndSet(el_one, set_one, combo_set));
  EXPECT_EQ(2, combo_set.size());
  set_one.clear();  // To construct the expected output set_one.
  set_one.insert(el_one);
  el_one.insert(3);
  set_one.insert(el_one);
  EXPECT_EQ(set_one, combo_set);
}

TEST_F(FaultTreeTest, MProbOr) {
  std::set<int> mcs;  // Minimal cut set.
  std::set< std::set<int> > p_terms;  // Positive terms of the equation.
  std::set< std::set<int> > n_terms;  // Negative terms of the equation.
  std::set< std::set<int> > temp_set;  // Temp set for dumping the output.
  std::set< std::set<int> > min_cut_sets;  // A set of minimal cut sets.
  ASSERT_THROW(MProbOr(min_cut_sets), ValueError);  // Error for an empty set.

  // Check for one element calculation for A.
  mcs.insert(0);
  p_terms.insert(mcs);
  min_cut_sets.insert(mcs);
  ASSERT_NO_THROW(MProbOr(min_cut_sets));
  EXPECT_EQ(0, min_cut_sets.size());  // This set is emptied recursively.
  temp_set.insert(pos_terms().begin(), pos_terms().end());
  EXPECT_EQ(p_terms, temp_set);

  // Check that recursive nsums=0 returns without changing anything.
  min_cut_sets.insert(mcs);
  pos_terms().clear();
  ASSERT_NO_THROW(MProbOr(min_cut_sets, 1, 0));
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
  ASSERT_NO_THROW(MProbOr(min_cut_sets));
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
  ASSERT_NO_THROW(MProbOr(min_cut_sets));
  temp_set.clear();
  temp_set.insert(pos_terms().begin(), pos_terms().end());
  EXPECT_EQ(p_terms, temp_set);
  temp_set.clear();
  temp_set.insert(neg_terms().begin(), neg_terms().end());
  EXPECT_EQ(n_terms, temp_set);
}
// ----------------------------------------------------------------------
// ---------------------- Test Public Functions --------------------------
// Test Input Processing
// Note that there are tests specificly for correct and incorrect inputs
// in fault_tree_input_tests.cc, so this test only concerned with actual changes
// after processing the input.
TEST_F(FaultTreeTest, ProcessInput) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  EXPECT_EQ(7, orig_ids().size());
  EXPECT_EQ("topevent", top_event_id());
  EXPECT_EQ(2, inter_events().size());
  EXPECT_EQ(1, inter_events().count("trainone"));
  EXPECT_EQ(1, inter_events().count("traintwo"));
  EXPECT_EQ(4, primary_events().size());
  EXPECT_EQ(1, primary_events().count("pumpone"));
  EXPECT_EQ(1, primary_events().count("pumptwo"));
  EXPECT_EQ(1, primary_events().count("valveone"));
  EXPECT_EQ(1, primary_events().count("valvetwo"));
  if (inter_events().count("trainone")) {
    InterEventPtr inter = inter_events()["trainone"];
    EXPECT_EQ("trainone", inter->id());
    ASSERT_NO_THROW(inter->gate());
    EXPECT_EQ("or", inter->gate());
    ASSERT_NO_THROW(inter->parent());
    EXPECT_EQ("topevent", inter->parent()->id());
  }
  if (primary_events().count("valveone")) {
    PrimaryEventPtr primary = primary_events()["valveone"];
    EXPECT_EQ("valveone", primary->id());
    ASSERT_NO_THROW(primary->parents());
    EXPECT_EQ(1, primary->parents().size());
    EXPECT_EQ(1, primary->parents().count("trainone"));
    ASSERT_NO_THROW(primary->type());
    EXPECT_EQ("basic", primary->type());
    EXPECT_THROW(primary->p(), Error);
  }
}

// Test Probability Assignment
TEST_F(FaultTreeTest, PopulateProbabilities) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  std::string prob_input = "./input/fta/correct_prob_input.scramp";
  ASSERT_THROW(fta->PopulateProbabilities(prob_input), Error);  // No tree.
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_EQ(4, primary_events().size());
  ASSERT_EQ(1, primary_events().count("pumpone"));
  ASSERT_EQ(1, primary_events().count("pumptwo"));
  ASSERT_EQ(1, primary_events().count("valveone"));
  ASSERT_EQ(1, primary_events().count("valvetwo"));
  ASSERT_NO_THROW(primary_events()["pumpone"]->p());
  ASSERT_NO_THROW(primary_events()["pumptwo"]->p());
  ASSERT_NO_THROW(primary_events()["valveone"]->p());
  ASSERT_NO_THROW(primary_events()["valvetwo"]->p());
  EXPECT_EQ(0.6, primary_events()["pumpone"]->p());
  EXPECT_EQ(0.7, primary_events()["pumptwo"]->p());
  EXPECT_EQ(0.4, primary_events()["valveone"]->p());
  EXPECT_EQ(0.5, primary_events()["valvetwo"]->p());
}

// Test Graphing Intructions
TEST_F(FaultTreeTest, GraphingInstructions) {
  std::vector<std::string> tree_input;
  tree_input.push_back("./input/fta/correct_tree_input.scramf");
  tree_input.push_back("./input/fta/doubly_defined_basic.scramf");
  tree_input.push_back("./input/fta/transfer_correct_top.scramf");
  tree_input.push_back("./input/fta/transfer_correct_sub.scramf");

  std::vector<std::string>::iterator it;
  for (it = tree_input.begin(); it != tree_input.end(); ++it) {
    delete fta;
    fta = new FaultTree("fta-default", true);
    ASSERT_THROW(fta->GraphingInstructions(), Error);
    ASSERT_NO_THROW(fta->ProcessInput(*it));
    ASSERT_NO_THROW(fta->GraphingInstructions());
  }

  // Handle an exception graphing case with one TransferIn only.
  std::string special_case = "./input/fta/transfer_graphing_exception.scramf";
  fta = new FaultTree("fta-default", true);
  ASSERT_NO_THROW(fta->ProcessInput(special_case));
  ASSERT_THROW(fta->GraphingInstructions(), ValidationError);
}

// Test Analysis
TEST_F(FaultTreeTest, AnalyzeDefault) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  std::string prob_input = "./input/fta/correct_prob_input.scramp";
  ASSERT_THROW(fta->Analyze(), Error);  // Calling without a tree initialized.
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->Analyze());
  std::set<std::string> mcs_1;
  std::set<std::string> mcs_2;
  std::set<std::string> mcs_3;
  std::set<std::string> mcs_4;
  mcs_1.insert("pumpone");
  mcs_1.insert("pumptwo");
  mcs_2.insert("pumpone");
  mcs_2.insert("valvetwo");
  mcs_3.insert("pumptwo");
  mcs_3.insert("valveone");
  mcs_4.insert("valveone");
  mcs_4.insert("valvetwo");
  EXPECT_EQ(4, min_cut_sets().size());
  EXPECT_EQ(1, min_cut_sets().count(mcs_1));
  EXPECT_EQ(1, min_cut_sets().count(mcs_2));
  EXPECT_EQ(1, min_cut_sets().count(mcs_3));
  EXPECT_EQ(1, min_cut_sets().count(mcs_4));

  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  EXPECT_DOUBLE_EQ(0.646, p_total());
  EXPECT_DOUBLE_EQ(0.42, prob_of_min_sets()[mcs_1]);
  EXPECT_DOUBLE_EQ(0.3, prob_of_min_sets()[mcs_2]);
  EXPECT_DOUBLE_EQ(0.28, prob_of_min_sets()[mcs_3]);
  EXPECT_DOUBLE_EQ(0.2, prob_of_min_sets()[mcs_4]);

  EXPECT_DOUBLE_EQ(0.72, imp_of_primaries()["pumpone"]);
  EXPECT_DOUBLE_EQ(0.7, imp_of_primaries()["pumptwo"]);
  EXPECT_DOUBLE_EQ(0.48, imp_of_primaries()["valveone"]);
  EXPECT_DOUBLE_EQ(0.5, imp_of_primaries()["valvetwo"]);

  // Probability calculations with the rare event approximation.
  delete fta;  // Re-initializing.
  fta = new FaultTree("fta-default", false, true);
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_NO_THROW(fta->Analyze());
  EXPECT_DOUBLE_EQ(1.2, p_total());
}

// Test Monte Carlo Analysis
TEST_F(FaultTreeTest, AnalyzeMC) {
  delete fta;  // Re-initializing.
  fta = new FaultTree("fta-mc", false);
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  ASSERT_THROW(fta->Analyze(), Error);  // Calling without a tree initialized.
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->Analyze());
}

// Test Reporting capabilities
TEST_F(FaultTreeTest, Report) {
  std::string tree_input = "./input/fta/correct_tree_input.scramf";
  std::string prob_input = "./input/fta/correct_prob_input.scramp";
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_THROW(fta->Report("/dev/null"), Error);  // Calling before analysis.
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));

  // Generate warning due to rare event approximation.
  delete fta;
  fta = new FaultTree(tree_input, false, true);
  ASSERT_NO_THROW(fta->ProcessInput(tree_input));
  ASSERT_NO_THROW(fta->PopulateProbabilities(prob_input));
  ASSERT_THROW(fta->Report("/dev/null"), Error);  // Calling before analysis.
  ASSERT_NO_THROW(fta->Analyze());
  ASSERT_NO_THROW(fta->Report("/dev/null"));
}
