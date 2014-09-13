#include "risk_analysis_tests.h"

#include <vector>

using namespace scram;

// ---------------------- Test Private Functions -------------------------
// Test the function that gets arguments from a line in an input file.
TEST_F(RiskAnalysisTest, CheckGate) {
  GatePtr top(new Gate("top", "and"));  // AND gate.
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
  top = GatePtr(new Gate("top", "or"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));  // One child is not enough.
  top->AddChild(B);
  EXPECT_TRUE(CheckGate(top));  // Two children are enough.
  top->AddChild(C);
  EXPECT_TRUE(CheckGate(top));  // More children are OK.

  // NOT Gate tests.
  top = GatePtr(new Gate("top", "not"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_TRUE(CheckGate(top));  // Exactly one child is required.
  top->AddChild(B);
  EXPECT_FALSE(CheckGate(top));  // Two children are too much.

  // NULL Gate tests.
  top = GatePtr(new Gate("top", "null"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_TRUE(CheckGate(top));  // Exactly one child is required.
  top->AddChild(B);
  EXPECT_FALSE(CheckGate(top));  // Two children are too much.

  // NOR Gate tests.
  top = GatePtr(new Gate("top", "nor"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));  // One child is not enough.
  top->AddChild(B);
  EXPECT_TRUE(CheckGate(top));  // Two children are enough.
  top->AddChild(C);
  EXPECT_TRUE(CheckGate(top));  // More children are OK.

  // NAND Gate tests.
  top = GatePtr(new Gate("top", "nand"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));  // One child is not enough.
  top->AddChild(B);
  EXPECT_TRUE(CheckGate(top));  // Two children are enough.
  top->AddChild(C);
  EXPECT_TRUE(CheckGate(top));  // More children are OK.

  // XOR Gate tests.
  top = GatePtr(new Gate("top", "xor"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));  // One child is not enough.
  top->AddChild(B);
  EXPECT_TRUE(CheckGate(top));  // Two children are enough.
  top->AddChild(C);
  EXPECT_FALSE(CheckGate(top));  // More than 2 is not allowed.

  // INHIBIT Gate tests.
  top = GatePtr(new Gate("top", "inhibit"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  A->type("basic");
  primary_events().insert(std::make_pair("a", A));
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));  // One child is not enough.
  B->type("basic");
  primary_events().insert(std::make_pair("b", B));
  top->AddChild(B);
  EXPECT_FALSE(CheckGate(top));  // Nodes must be conditional.
  top->AddChild(C);
  EXPECT_FALSE(CheckGate(top));  // More than 2 is not allowed.
  top = GatePtr(new Gate("top", "inhibit"));  // Re-initialize.
  C->type("conditional");
  primary_events().insert(std::make_pair("c", C));
  top->AddChild(A);  // Basic event.
  top->AddChild(C);  // Conditional event.
  EXPECT_TRUE(CheckGate(top));  // Two children with exact combination.
  A = PrimaryEventPtr(new PrimaryEvent("a", "conditional"));
  primary_events().clear();
  primary_events().insert(std::make_pair("a", A));
  primary_events().insert(std::make_pair("c", C));
  EXPECT_FALSE(CheckGate(top));  // Wrong combination.

  // VOTE/ATLEAST gate tests.
  top = GatePtr(new Gate("top", "atleast"));
  top->vote_number(2);
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));  // One child is not enough.
  top->AddChild(B);
  EXPECT_FALSE(CheckGate(top));  // Two children are not enough.
  top->AddChild(C);
  EXPECT_TRUE(CheckGate(top));  // More than 2 is needed.

  // Some UNKNOWN gate tests.
  top = GatePtr(new Gate("top", "unknown_gate"));
  EXPECT_FALSE(CheckGate(top));  // No child.
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));
  top->AddChild(B);
  EXPECT_FALSE(CheckGate(top));
}

// ---------------------- Test Public Functions --------------------------
// Test Input Processing for Risk Analysis.
// Note that there are tests specificly for correct and incorrect inputs
// in risk_analysis_input_tests.cc, so this test is only concerned with
// actual changes after processing the input.
TEST_F(RiskAnalysisTest, ProcessInput) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  EXPECT_EQ(3, gates().size());
  EXPECT_EQ(1, gates().count("trainone"));
  EXPECT_EQ(1, gates().count("traintwo"));
  EXPECT_EQ(1, gates().count("topevent"));
  EXPECT_EQ(4, primary_events().size());
  EXPECT_EQ(1, primary_events().count("pumpone"));
  EXPECT_EQ(1, primary_events().count("pumptwo"));
  EXPECT_EQ(1, primary_events().count("valveone"));
  EXPECT_EQ(1, primary_events().count("valvetwo"));
  if (gates().count("trainone")) {
    GatePtr inter = gates().find("trainone")->second;
    EXPECT_EQ("trainone", inter->id());
    ASSERT_NO_THROW(inter->type());
    EXPECT_EQ("or", inter->type());
    ASSERT_NO_THROW(inter->parents());
    EXPECT_EQ("topevent", inter->parents().begin()->first);
  }
  if (primary_events().count("valveone")) {
    PrimaryEventPtr primary = primary_events().find("valveone")->second;
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
TEST_F(RiskAnalysisTest, PopulateProbabilities) {
  // Input with probabilities
  std::string tree_input = "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_EQ(4, primary_events().size());
  ASSERT_EQ(1, primary_events().count("pumpone"));
  ASSERT_EQ(1, primary_events().count("pumptwo"));
  ASSERT_EQ(1, primary_events().count("valveone"));
  ASSERT_EQ(1, primary_events().count("valvetwo"));
  ASSERT_NO_THROW(primary_events().find("pumpone")->second->p());
  ASSERT_NO_THROW(primary_events().find("pumptwo")->second->p());
  ASSERT_NO_THROW(primary_events().find("valveone")->second->p());
  ASSERT_NO_THROW(primary_events().find("valvetwo")->second->p());
  EXPECT_EQ(0.6, primary_events().find("pumpone")->second->p());
  EXPECT_EQ(0.7, primary_events().find("pumptwo")->second->p());
  EXPECT_EQ(0.4, primary_events().find("valveone")->second->p());
  EXPECT_EQ(0.5, primary_events().find("valvetwo")->second->p());
}

// Test Graphing Intructions
TEST_F(RiskAnalysisTest, GraphingInstructions) {
  std::vector<std::string> tree_input;
  tree_input.push_back("./share/scram/input/fta/correct_tree_input.xml");
  tree_input.push_back("./share/scram/input/fta/graphing.xml");

  std::vector<std::string>::iterator it;
  for (it = tree_input.begin(); it != tree_input.end(); ++it) {
    fta(new FaultTreeAnalysis("default"));
    ASSERT_NO_THROW(ran->ProcessInput(*it));
    ASSERT_NO_THROW(ran->GraphingInstructions());
  }
}

// Test Analysis
TEST_F(RiskAnalysisTest, AnalyzeDefault) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  std::string with_prob = "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
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

  // Probability calculations.
  fta(new FaultTreeAnalysis("default"));
  ASSERT_NO_THROW(ran->ProcessInput(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.646, p_total());
  EXPECT_DOUBLE_EQ(0.42, prob_of_min_sets().find(mcs_1)->second);
  EXPECT_DOUBLE_EQ(0.3, prob_of_min_sets().find(mcs_2)->second);
  EXPECT_DOUBLE_EQ(0.28, prob_of_min_sets().find(mcs_3)->second);
  EXPECT_DOUBLE_EQ(0.2, prob_of_min_sets().find(mcs_4)->second);

  EXPECT_DOUBLE_EQ(0.72, imp_of_primaries().find("pumpone")->second);
  EXPECT_DOUBLE_EQ(0.7, imp_of_primaries().find("pumptwo")->second);
  EXPECT_DOUBLE_EQ(0.48, imp_of_primaries().find("valveone")->second);
  EXPECT_DOUBLE_EQ(0.5, imp_of_primaries().find("valvetwo")->second);

  // Probability calculations with the rare event approximation.
  fta(new FaultTreeAnalysis("default", "rare"));
  ASSERT_NO_THROW(ran->ProcessInput(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(1.2, p_total());

  // Probability calculations with the MCUB approximation.
  fta(new FaultTreeAnalysis("default", "mcub"));
  ASSERT_NO_THROW(ran->ProcessInput(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.766144, p_total());
}

// Test Monte Carlo Analysis
TEST_F(RiskAnalysisTest, AnalyzeMC) {
  fta(new FaultTreeAnalysis("mc"));
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
}

// Test Reporting capabilities
TEST_F(RiskAnalysisTest, Report) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_NO_THROW(ran->Report("/dev/null"));

  // Generate warning due to rare event approximation.
  fta(new FaultTreeAnalysis("default", "rare"));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  ASSERT_NO_THROW(ran->Report("/dev/null"));
}
