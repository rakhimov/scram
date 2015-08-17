/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "risk_analysis_tests.h"

#include <fstream>
#include <sstream>
#include <vector>

#include "env.h"
#include "error.h"
#include "xml_parser.h"

namespace scram {
namespace test {

TEST_F(RiskAnalysisTest, ProcessInput) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  EXPECT_EQ(3, gates().size());
  EXPECT_EQ(1, gates().count("trainone"));
  EXPECT_EQ(1, gates().count("traintwo"));
  EXPECT_EQ(1, gates().count("topevent"));
  EXPECT_EQ(4, basic_events().size());
  EXPECT_EQ(1, basic_events().count("pumpone"));
  EXPECT_EQ(1, basic_events().count("pumptwo"));
  EXPECT_EQ(1, basic_events().count("valveone"));
  EXPECT_EQ(1, basic_events().count("valvetwo"));
  if (gates().count("topevent")) {
    GatePtr top = gates().find("topevent")->second;
    EXPECT_EQ("topevent", top->id());
    ASSERT_NO_THROW(top->formula()->type());
    EXPECT_EQ("and", top->formula()->type());
    EXPECT_EQ(2, top->formula()->event_args().size());
  }
  if (gates().count("trainone")) {
    GatePtr inter = gates().find("trainone")->second;
    EXPECT_EQ("trainone", inter->id());
    ASSERT_NO_THROW(inter->formula()->type());
    EXPECT_EQ("or", inter->formula()->type());
    EXPECT_EQ(2, inter->formula()->event_args().size());
  }
  if (basic_events().count("valveone")) {
    BasicEventPtr primary = basic_events().find("valveone")->second;
    EXPECT_EQ("valveone", primary->id());
  }
}

// Test Probability Assignment
TEST_F(RiskAnalysisTest, PopulateProbabilities) {
  // Input with probabilities
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
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

// Test Analysis of Two train system.
TEST_F(RiskAnalysisTest, AnalyzeDefault) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
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
}

TEST_F(RiskAnalysisTest, AnalyzeWithProbability) {
  std::string with_prob =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
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

  settings.probability_analysis(true).importance_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(ran->Analyze());

  EXPECT_EQ(4, min_cut_sets().size());
  EXPECT_EQ(1, min_cut_sets().count(mcs_1));
  EXPECT_EQ(1, min_cut_sets().count(mcs_2));
  EXPECT_EQ(1, min_cut_sets().count(mcs_3));
  EXPECT_EQ(1, min_cut_sets().count(mcs_4));

  EXPECT_DOUBLE_EQ(0.646, p_total());
  EXPECT_DOUBLE_EQ(0.42, prob_of_min_sets().find(mcs_1)->second);
  EXPECT_DOUBLE_EQ(0.3, prob_of_min_sets().find(mcs_2)->second);
  EXPECT_DOUBLE_EQ(0.28, prob_of_min_sets().find(mcs_3)->second);
  EXPECT_DOUBLE_EQ(0.2, prob_of_min_sets().find(mcs_4)->second);

  // Check importance values.
  std::vector<double> imp;
  std::map< std::string, std::vector<double> > importance;
  imp = {0.47368, 0.51, 0.9, 1.9, 1.315};
  importance.insert({"pumpone", imp});
  imp = {0.41176, 0.38, 0.7, 1.7, 1.1765};
  importance.insert({"pumptwo", imp});
  imp = {0.21053, 0.34, 0.26667, 1.2667, 1.3158};
  importance.insert({"valveone", imp});
  imp = {0.17647, 0.228, 0.21429, 1.2143, 1.1765};
  importance.insert({"valvetwo", imp});

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

TEST_F(RiskAnalysisTest, AnalyzeNestedFormula) {
  std::string nested_input = "./share/scram/input/fta/nested_formula.xml";
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

  ASSERT_NO_THROW(ProcessInputFile(nested_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_EQ(4, min_cut_sets().size());
  EXPECT_EQ(1, min_cut_sets().count(mcs_1));
  EXPECT_EQ(1, min_cut_sets().count(mcs_2));
  EXPECT_EQ(1, min_cut_sets().count(mcs_3));
  EXPECT_EQ(1, min_cut_sets().count(mcs_4));
}

TEST_F(RiskAnalysisTest, Importance) {
  std::string tree_input = "./share/scram/input/fta/importance_test.xml";
  settings.importance_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.67, p_total());
  // Check importance values with negative event.
  std::vector<double> imp;
  std::map< std::string, std::vector<double> > importance;
  imp = {0.40299, 0.45, 0.675, 1.675, 1.2687};
  importance.insert({"pumpone", imp});
  imp = {0.31343, 0.3, 0.45652, 1.4565, 1.1343};
  importance.insert({"pumptwo", imp});
  imp = {0.23881, 0.4, 0.31373, 1.3137, 1.3582};
  importance.insert({"valveone", imp});
  imp = {0.13433, 0.18, 0.15517, 1.1552, 1.1343};
  importance.insert({"valvetwo", imp});

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
  settings.approx("rare-event").probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(1.2, p_total());
}

// Apply the minimal cut set upper bound approximation.
TEST_F(RiskAnalysisTest, MCUB) {
  std::string with_prob =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  // Probability calculations with the MCUB approximation.
  settings.approx("mcub").probability_analysis(true).importance_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_DOUBLE_EQ(0.766144, p_total());
}

// Apply the minimal cut set upper bound approximation for non-coherent tree.
// This should be a warning.
TEST_F(RiskAnalysisTest, McubNonCoherent) {
  std::string with_prob = "./share/scram/input/core/a_and_not_b.xml";
  // Probability calculations with the MCUB approximation.
  settings.approx("mcub").probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_NEAR(0.08, p_total(), 1e-5);
}

// Test Monte Carlo Analysis
/// @todo Expand this test.
TEST_F(RiskAnalysisTest, AnalyzeMC) {
  settings.uncertainty_analysis(true);
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
}

// Test Reporting capabilities
// Tests the output against the schema. However the contents of the
// output are not verified or validated.
TEST_F(RiskAnalysisTest, ReportIOError) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  // Messing up the output file.
  std::string output = "abracadabra.cadabraabra/output.txt";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  EXPECT_THROW(ran->Report(output), IOError);
}

// Reporting of the default analysis for MCS only without probabilities.
TEST_F(RiskAnalysisTest, ReportDefaultMCS) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
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

  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
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

  settings.importance_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
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

  settings.uncertainty_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of CCF analysis.
TEST_F(RiskAnalysisTest, ReportCCF) {
  std::string tree_input = "./share/scram/input/core/mgl_ccf.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  settings.ccf_analysis(true).importance_analysis(true).num_sums(3);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of Negative events in MCS.
TEST_F(RiskAnalysisTest, ReportNegativeEvent) {
  std::string tree_input = "./share/scram/input/core/a_or_not_b.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
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

  settings.importance_analysis(true).uncertainty_analysis(true)
      .ccf_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting with public or private roles.
TEST_F(RiskAnalysisTest, ReportRoles) {
  std::string tree_input = "./share/scram/input/fta/mixed_roles.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of orphan primary events.
TEST_F(RiskAnalysisTest, ReportOrphanPrimaryEvents) {
  std::string tree_input = "./share/scram/input/fta/orphan_primary_event.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// Reporting of unused parameters.
TEST_F(RiskAnalysisTest, ReportUnusedParameters) {
  std::string tree_input = "./share/scram/input/fta/unused_parameter.xml";

  std::stringstream schema;
  std::string schema_path = Env::report_schema();
  std::ifstream schema_stream(schema_path.c_str());
  schema << schema_stream.rdbuf();
  schema_stream.close();

  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());

  std::stringstream output;
  ASSERT_NO_THROW(ran->Report(output));

  std::shared_ptr<XMLParser> parser;
  ASSERT_NO_THROW(parser = std::shared_ptr<XMLParser>(new XMLParser(output)));
  ASSERT_NO_THROW(parser->Validate(schema));
}

// NAND and NOR as a child cases.
TEST_F(RiskAnalysisTest, ChildNandNorGates) {
  std::string tree_input = "./share/scram/input/fta/children_nand_nor.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
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
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
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
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(ran->Analyze());
  std::set<std::string> mcs_1;
  EXPECT_EQ(1, min_cut_sets().size());
  EXPECT_EQ(1, min_cut_sets().count(mcs_1));
}

}  // namespace test
}  // namespace scram
