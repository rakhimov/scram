#include "risk_analysis_tests.h"

#include <fstream>
#include <sstream>
#include <vector>

#include <boost/assign/std/vector.hpp>

#include "env.h"
#include "error.h"
#include "xml_parser.h"

using namespace scram;

// ---------------------- Test Private Functions -------------------------
// Test the function that gets arguments from a line in an input file.
TEST_F(RiskAnalysisTest, CheckGate) {
  GatePtr top(new Gate("top", "and"));  // AND gate.
  BasicEventPtr A(new BasicEvent("a"));
  BasicEventPtr B(new BasicEvent("b"));
  BasicEventPtr C(new BasicEvent("c"));

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

  // INHIBIT Gate tests.
  Attribute inh_attr;
  inh_attr.name="flavor";
  inh_attr.value="inhibit";
  top = GatePtr(new Gate("top", "and"));
  top->AddAttribute(inh_attr);
  EXPECT_FALSE(CheckGate(top));  // No child.
  primary_events().insert(std::make_pair("a", A));
  top->AddChild(A);
  EXPECT_FALSE(CheckGate(top));  // One child is not enough.
  primary_events().insert(std::make_pair("b", B));
  top->AddChild(B);
  EXPECT_FALSE(CheckGate(top));  // Nodes must be conditional.
  top->AddChild(C);
  EXPECT_FALSE(CheckGate(top));  // More than 2 is not allowed.

  top = GatePtr(new Gate("top", "and"));  // Re-initialize.
  top->AddAttribute(inh_attr);

  Attribute cond;
  cond.name = "flavor";
  cond.value = "conditional";
  C->AddAttribute(cond);
  primary_events().insert(std::make_pair("c", C));
  top->AddChild(A);  // Basic event.
  top->AddChild(C);  // Conditional event.
  EXPECT_TRUE(CheckGate(top));  // Two children with exact combination.
  A->AddAttribute(cond);
  EXPECT_FALSE(CheckGate(top));  // Wrong combination.

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
// Note that there are tests specifically for correct and incorrect inputs
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
  }
}

// Test Probability Assignment
TEST_F(RiskAnalysisTest, PopulateProbabilities) {
  // Input with probabilities
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_EQ(4, basic_events().size());
  ASSERT_EQ(1, basic_events().count("pumpone"));
  ASSERT_EQ(1, basic_events().count("pumptwo"));
  ASSERT_EQ(1, basic_events().count("valveone"));
  ASSERT_EQ(1, basic_events().count("valvetwo"));
  ASSERT_NO_THROW(basic_events().find("pumpone")->second->p());
  ASSERT_NO_THROW(basic_events().find("pumptwo")->second->p());
  ASSERT_NO_THROW(basic_events().find("valveone")->second->p());
  ASSERT_NO_THROW(basic_events().find("valvetwo")->second->p());
  EXPECT_EQ(0.6, basic_events().find("pumpone")->second->p());
  EXPECT_EQ(0.7, basic_events().find("pumptwo")->second->p());
  EXPECT_EQ(0.4, basic_events().find("valveone")->second->p());
  EXPECT_EQ(0.5, basic_events().find("valvetwo")->second->p());
}

// Test Graphing Intructions
TEST_F(RiskAnalysisTest, GraphingInstructions) {
  std::vector<std::string> tree_input;
  tree_input.push_back("./share/scram/input/fta/correct_tree_input.xml");
  tree_input.push_back("./share/scram/input/fta/graphing.xml");
  tree_input.push_back("./share/scram/input/fta/flavored_types.xml");

  std::vector<std::string>::iterator it;
  for (it = tree_input.begin(); it != tree_input.end(); ++it) {
    delete ran;
    ran = new RiskAnalysis();
    ASSERT_NO_THROW(ran->ProcessInput(*it));
    ASSERT_NO_THROW(ran->GraphingInstructions("/dev/null"));
  }

  // Messing up the input file.
  std::string output = "abracadabra.cadabraabra/graphing.dot";
  EXPECT_THROW(ran->GraphingInstructions(output), IOError);
}

// Test Analysis of Two train system.
TEST_F(RiskAnalysisTest, AnalyzeDefault) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  std::string with_prob =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
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
  delete ran;
  ran = new RiskAnalysis();
  ran->AddSettings(settings.probability_analysis(true)
                           .importance_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.646, p_total());
  EXPECT_DOUBLE_EQ(0.42, prob_of_min_sets().find(mcs_1)->second);
  EXPECT_DOUBLE_EQ(0.3, prob_of_min_sets().find(mcs_2)->second);
  EXPECT_DOUBLE_EQ(0.28, prob_of_min_sets().find(mcs_3)->second);
  EXPECT_DOUBLE_EQ(0.2, prob_of_min_sets().find(mcs_4)->second);

  // Check importance values.
  using namespace boost::assign;
  std::vector<double> imp;
  std::map< std::string, std::vector<double> > importance;
  imp += 0.47368, 0.51, 0.9, 1.9, 1.315;
  importance.insert(std::make_pair("pumpone", imp));
  imp.clear();
  imp += 0.41176, 0.38, 0.7, 1.7, 1.1765;
  importance.insert(std::make_pair("pumptwo", imp));
  imp.clear();
  imp += 0.21053, 0.34, 0.26667, 1.2667, 1.3158;
  importance.insert(std::make_pair("valveone", imp));
  imp.clear();
  imp += 0.17647, 0.228, 0.21429, 1.2143, 1.1765;
  importance.insert(std::make_pair("valvetwo", imp));

  std::map< std::string, std::vector<double> >::iterator it;
  for (it = importance.begin(); it != importance.end(); ++it) {
    std::vector<double> results = RiskAnalysisTest::importance(it->first);
    ASSERT_EQ(5, results.size());
    for (int i = 0; i < 5; ++i) {
      EXPECT_NEAR(it->second[i], results[i], 1e-3)
          << it->first << ": Importance " << i;
    }
  }
}

TEST_F(RiskAnalysisTest, Importance) {
  std::string tree_input = "./share/scram/input/fta/importance_test.xml";
  ran->AddSettings(settings.importance_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.67, p_total());
  // Check importance values with negative event.
  using namespace boost::assign;
  std::vector<double> imp;
  std::map< std::string, std::vector<double> > importance;
  imp += 0.40299, 0.45, 0.675, 1.675, 1.2687;
  importance.insert(std::make_pair("pumpone", imp));
  imp.clear();
  imp += 0.31343, 0.3, 0.45652, 1.4565, 1.1343;
  importance.insert(std::make_pair("pumptwo", imp));
  imp.clear();
  imp += 0.23881, 0.4, 0.31373, 1.3137, 1.3582;
  importance.insert(std::make_pair("valveone", imp));
  imp.clear();
  imp += 0.13433, 0.18, 0.15517, 1.1552, 1.1343;
  importance.insert(std::make_pair("valvetwo", imp));

  std::map< std::string, std::vector<double> >::iterator it;
  for (it = importance.begin(); it != importance.end(); ++it) {
    std::vector<double> results = RiskAnalysisTest::importance(it->first);
    assert(results.size() == 5);
    for (int i = 0; i < 5; ++i) {
      EXPECT_NEAR(it->second[i], results[i], 1e-3)
          << it->first << ": Importance " << i;
    }
  }
}

// Apply the rare event approximation.
TEST_F(RiskAnalysisTest, RareEvent) {
  std::string with_prob =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  // Probability calculations with the rare event approximation.
  ran->AddSettings(settings.approx("rare-event").probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(1.2, p_total());
}

// Apply the minimal cut set upper bound approximation.
TEST_F(RiskAnalysisTest, MCUB) {
  std::string with_prob =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  // Probability calculations with the MCUB approximation.
  ran->AddSettings(settings.approx("mcub").probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.766144, p_total());
}

// Apply the minimal cut set upper bound approximation for non-coherent tree.
// This should be a warning.
TEST_F(RiskAnalysisTest, McubNonCoherent) {
  std::string with_prob = "./share/scram/input/benchmark/a_and_not_b.xml";
  // Probability calculations with the MCUB approximation.
  ran->AddSettings(settings.approx("mcub").probability_analysis(true));
  ASSERT_NO_THROW(ran->ProcessInput(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(0.08, p_total(), 1e-5);
}

// Test Monte Carlo Analysis
/// @todo Expand this test.
TEST_F(RiskAnalysisTest, AnalyzeMC) {
  ran->AddSettings(settings.uncertainty_analysis(true));
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
}

// Test Reporting capabilities
// Tests the output against the schema. However the contents of the
// output are not verified or validated.
TEST_F(RiskAnalysisTest, ReportIOError) {
  // Messing up the output file.
  std::string output = "abracadabra.cadabraabra/output.txt";
  EXPECT_THROW(ran->Report(output), IOError);
}

// Reporting of the default analysis for MCS only without probabilities.
TEST_F(RiskAnalysisTest, ReportDefaultMCS) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  boost::shared_ptr<XMLParser> parser(new XMLParser());
  ASSERT_NO_THROW(parser->Init(output));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of analysis for MCS with probability results.
TEST_F(RiskAnalysisTest, ReportProbability) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ran->AddSettings(settings.probability_analysis(true)));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  boost::shared_ptr<XMLParser> parser(new XMLParser());
  ASSERT_NO_THROW(parser->Init(output));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of importance analysis.
TEST_F(RiskAnalysisTest, ReportImportanceFactors) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ran->AddSettings(settings.importance_analysis(true)));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  boost::shared_ptr<XMLParser> parser(new XMLParser());
  ASSERT_NO_THROW(parser->Init(output));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of uncertainty analysis.
TEST_F(RiskAnalysisTest, ReportUncertaintyResults) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ran->AddSettings(settings.uncertainty_analysis(true)));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  boost::shared_ptr<XMLParser> parser(new XMLParser());
  ASSERT_NO_THROW(parser->Init(output));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of CCF analysis.
TEST_F(RiskAnalysisTest, ReportCCF) {
  std::string tree_input =
      "./share/scram/input/benchmark/mgl_ccf.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ran->AddSettings(settings.ccf_analysis(true)
                                           .importance_analysis(true)
                                           .num_sums(3)));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  boost::shared_ptr<XMLParser> parser(new XMLParser());
  ASSERT_NO_THROW(parser->Init(output));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of Negative events in MCS.
TEST_F(RiskAnalysisTest, ReportNegativeEvent) {
  std::string tree_input =
      "./share/scram/input/benchmark/a_or_not_b.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ran->AddSettings(settings.probability_analysis(true)));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  boost::shared_ptr<XMLParser> parser(new XMLParser());
  ASSERT_NO_THROW(parser->Init(output));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of all possible analyses.
TEST_F(RiskAnalysisTest, ReportAll) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ran->AddSettings(settings.importance_analysis(true)
                                           .uncertainty_analysis(true)
                                           .ccf_analysis(true)));
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  boost::shared_ptr<XMLParser> parser(new XMLParser());
  ASSERT_NO_THROW(parser->Init(output));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// NAND and NOR as a child cases.
TEST_F(RiskAnalysisTest, ChildNandNorGates) {
  std::string tree_input = "./share/scram/input/fta/children_nand_nor.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  std::set<std::string> mcs_1;
  std::set<std::string> mcs_2;
  mcs_1.insert("not pumpone");
  mcs_1.insert("not pumptwo");
  mcs_1.insert("not valveone");
  mcs_2.insert("not pumpone");
  mcs_2.insert("not valvetwo");
  mcs_2.insert("not valveone");
  EXPECT_EQ(2, min_cut_sets().size());
  EXPECT_EQ(1, min_cut_sets().count(mcs_1));
  EXPECT_EQ(1, min_cut_sets().count(mcs_2));
}

// Simple test for several house event propagation.
TEST_F(RiskAnalysisTest, ManyHouseEvents) {
  std::string tree_input = "./share/scram/input/fta/constant_propagation.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  std::set<std::string> mcs_1;
  mcs_1.insert("a");
  mcs_1.insert("b");
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(1, min_cut_sets().count(mcs_1));
}

// Simple test for several constant gate propagation.
TEST_F(RiskAnalysisTest, ConstantGates) {
  std::string tree_input = "./share/scram/input/fta/constant_gates.xml";
  ASSERT_NO_THROW(ran->ProcessInput(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  std::set<std::string> mcs_1;
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(1, min_cut_sets().count(mcs_1));
}
