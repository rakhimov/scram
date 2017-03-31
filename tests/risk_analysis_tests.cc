/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

#include <sstream>
#include <utility>

#include <libxml++/libxml++.h>

#include "env.h"
#include "error.h"
#include "initializer.h"
#include "reporter.h"

namespace scram {
namespace core {
namespace test {

const std::set<std::set<std::string>> RiskAnalysisTest::kUnity = {
    std::set<std::string>{}};

void RiskAnalysisTest::SetUp() {
  if (HasParam()) {
    std::string param = GetParam();
    if (param == "pi") {
      settings.algorithm("bdd");
      settings.prime_implicants(true);
    } else {
      settings.algorithm(GetParam());
    }
  }
}

void RiskAnalysisTest::ProcessInputFiles(
    const std::vector<std::string>& input_files) {
  mef::Initializer init(input_files, settings);
  model = init.model();
  analysis = std::make_unique<RiskAnalysis>(model, settings);
  result_ = Result();
}

void RiskAnalysisTest::CheckReport(const std::string& tree_input) {
  static xmlpp::RelaxNGValidator validator(Env::report_schema());

  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  std::stringstream output;
  ASSERT_NO_THROW(Reporter().Report(*analysis, output));

  xmlpp::DomParser parser;
  ASSERT_NO_THROW(parser.parse_stream(output));
  ASSERT_NO_THROW(validator.validate(parser.get_document()));
}

const std::set<std::set<std::string>>& RiskAnalysisTest::products() {
  assert(!analysis->fault_tree_analyses().empty());
  assert(analysis->fault_tree_analyses().size() == 1);
  if (result_.products.empty()) {
    const FaultTreeAnalysis* fta =
        analysis->fault_tree_analyses().begin()->second.get();
    for (const Product& product : fta->products()) {
      result_.products.emplace(Convert(product));
    }
  }
  return result_.products;
}

std::vector<int> RiskAnalysisTest::ProductDistribution() {
  assert(!analysis->fault_tree_analyses().empty());
  assert(analysis->fault_tree_analyses().size() == 1);
  std::vector<int> distr(settings.limit_order(), 0);
  const FaultTreeAnalysis* fta =
      analysis->fault_tree_analyses().begin()->second.get();
  for (const Product& product : fta->products()) {
    distr[product.order() - 1]++;
  }
  while (!distr.empty() && !distr.back())
    distr.pop_back();
  return distr;
}

void RiskAnalysisTest::PrintProducts() {
  assert(!analysis->fault_tree_analyses().empty());
  assert(analysis->fault_tree_analyses().size() == 1);
  const FaultTreeAnalysis* fta =
      analysis->fault_tree_analyses().begin()->second.get();
  Print(fta->products());
}

const std::map<std::set<std::string>, double>&
RiskAnalysisTest::product_probability() {
  assert(!analysis->fault_tree_analyses().empty());
  assert(analysis->fault_tree_analyses().size() == 1);
  if (result_.product_probability.empty()) {
    const FaultTreeAnalysis* fta =
        analysis->fault_tree_analyses().begin()->second.get();
    for (const Product& product : fta->products()) {
      result_.product_probability.emplace(Convert(product), product.p());
    }
  }
  return result_.product_probability;
}

std::set<std::string> RiskAnalysisTest::Convert(const Product& product) {
  std::set<std::string> string_set;
  for (const Literal& literal : product) {
    string_set.insert((literal.complement ? "not " : "") + literal.event.id());
  }
  return string_set;
}

TEST_F(RiskAnalysisTest, ProcessInput) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  EXPECT_EQ(3, gates().size());
  EXPECT_EQ(1, gates().count("TrainOne"));
  EXPECT_EQ(1, gates().count("TrainTwo"));
  EXPECT_EQ(1, gates().count("TopEvent"));
  EXPECT_EQ(4, basic_events().size());
  EXPECT_EQ(1, basic_events().count("PumpOne"));
  EXPECT_EQ(1, basic_events().count("PumpTwo"));
  EXPECT_EQ(1, basic_events().count("ValveOne"));
  EXPECT_EQ(1, basic_events().count("ValveTwo"));

  ASSERT_TRUE(gates().count("TopEvent"));
  mef::GatePtr top = *gates().find("TopEvent");
  EXPECT_EQ("TopEvent", top->id());
  ASSERT_NO_THROW(top->formula().type());
  EXPECT_EQ(mef::kAnd, top->formula().type());
  EXPECT_EQ(2, top->formula().event_args().size());

  ASSERT_TRUE(gates().count("TrainOne"));
  mef::GatePtr inter = *gates().find("TrainOne");
  EXPECT_EQ("TrainOne", inter->id());
  ASSERT_NO_THROW(inter->formula().type());
  EXPECT_EQ(mef::kOr, inter->formula().type());
  EXPECT_EQ(2, inter->formula().event_args().size());

  ASSERT_TRUE(basic_events().count("ValveOne"));
  mef::BasicEventPtr primary = *basic_events().find("ValveOne");
  EXPECT_EQ("ValveOne", primary->id());
}

// Test Probability Assignment
TEST_F(RiskAnalysisTest, PopulateProbabilities) {
  // Input with probabilities
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_EQ(4, basic_events().size());
  ASSERT_EQ(1, basic_events().count("PumpOne"));
  ASSERT_EQ(1, basic_events().count("PumpTwo"));
  ASSERT_EQ(1, basic_events().count("ValveOne"));
  ASSERT_EQ(1, basic_events().count("ValveTwo"));

  mef::BasicEventPtr p1 = *basic_events().find("PumpOne");
  mef::BasicEventPtr p2 = *basic_events().find("PumpTwo");
  mef::BasicEventPtr v1 = *basic_events().find("ValveOne");
  mef::BasicEventPtr v2 = *basic_events().find("ValveTwo");

  ASSERT_NO_THROW(p1->p());
  ASSERT_NO_THROW(p2->p());
  ASSERT_NO_THROW(v1->p());
  ASSERT_NO_THROW(v2->p());
  EXPECT_EQ(0.6, p1->p());
  EXPECT_EQ(0.7, p2->p());
  EXPECT_EQ(0.4, v1->p());
  EXPECT_EQ(0.5, v2->p());
}

// Test Analysis of Two train system.
TEST_P(RiskAnalysisTest, AnalyzeDefault) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  std::set<std::set<std::string>> mcs = {{"PumpOne", "PumpTwo"},
                                         {"PumpOne", "ValveTwo"},
                                         {"PumpTwo", "ValveOne"},
                                         {"ValveOne", "ValveTwo"}};
  EXPECT_EQ(mcs, products());
  PrintProducts();  // Quick visual verification.
}

TEST_P(RiskAnalysisTest, AnalyzeNonCoherentDefault) {
  std::string tree_input = "./share/scram/input/fta/correct_non_coherent.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"not PumpOne", "ValveOne"},
                                          {"PumpOne", "PumpTwo"},
                                          {"PumpOne", "ValveTwo"},
                                          {"PumpTwo", "ValveOne"},
                                          {"ValveOne", "ValveTwo"}};
    EXPECT_EQ(5, products().size());
    EXPECT_EQ(pi, products());
  } else {
    std::set<std::set<std::string>> mcs = {{"PumpOne", "PumpTwo"},
                                           {"PumpOne", "ValveTwo"},
                                           {"ValveOne"}};
    EXPECT_EQ(mcs, products());
  }
}

TEST_P(RiskAnalysisTest, AnalyzeWithProbability) {
  std::string with_prob =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  std::set<std::string> mcs_1 = {"PumpOne", "PumpTwo"};
  std::set<std::string> mcs_2 = {"PumpOne", "ValveTwo"};
  std::set<std::string> mcs_3 = {"PumpTwo", "ValveOne"};
  std::set<std::string> mcs_4 = {"ValveOne", "ValveTwo"};
  std::set<std::set<std::string>> mcs = {mcs_1, mcs_2, mcs_3, mcs_4};
  settings.probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(analysis->Analyze());

  EXPECT_EQ(mcs, products());
  if (settings.approximation() == Approximation::kRareEvent) {
    EXPECT_DOUBLE_EQ(1, p_total());
  } else {
    EXPECT_DOUBLE_EQ(0.646, p_total());
  }
  EXPECT_DOUBLE_EQ(0.42, product_probability().at(mcs_1));
  EXPECT_DOUBLE_EQ(0.3, product_probability().at(mcs_2));
  EXPECT_DOUBLE_EQ(0.28, product_probability().at(mcs_3));
  EXPECT_DOUBLE_EQ(0.2, product_probability().at(mcs_4));
}

// Test for exact probability calculation
// regardless of the qualitative analysis algorithm.
TEST_P(RiskAnalysisTest, EnforceExactProbability) {
  std::string with_prob =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  settings.probability_analysis(true).approximation("none");
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(0.646, p_total());
}

TEST_P(RiskAnalysisTest, AnalyzeNestedFormula) {
  std::string nested_input = "./share/scram/input/fta/nested_formula.xml";
  std::set<std::set<std::string>> mcs = {{"PumpOne", "PumpTwo"},
                                         {"PumpOne", "ValveTwo"},
                                         {"PumpTwo", "ValveOne"},
                                         {"ValveOne", "ValveTwo"}};
  ASSERT_NO_THROW(ProcessInputFile(nested_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_EQ(mcs, products());
}

TEST_F(RiskAnalysisTest, ImportanceDefault) {
  std::string with_prob =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  settings.importance_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(analysis->Analyze());
  TestImportance({{"PumpOne", {2, 0.51, 0.4737, 0.7895, 1.316, 1.9}},
                  {"PumpTwo", {2, 0.38, 0.4118, 0.8235, 1.176, 1.7}},
                  {"ValveOne", {2, 0.34, 0.2105, 0.5263, 1.316, 1.267}},
                  {"ValveTwo", {2, 0.228, 0.1765, 0.5882, 1.176, 1.214}}});
}

TEST_F(RiskAnalysisTest, ImportanceNeg) {
  std::string tree_input = "./share/scram/input/fta/importance_neg_test.xml";
  settings.prime_implicants(true).importance_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_NEAR(0.04459, p_total(), 1e-3);
  // Check importance values with negative event.
  TestImportance({{"PumpOne", {3, 0.0765, 0.1029, 0.1568, 2.613, 1.115}},
                  {"PumpTwo", {2, 0.057, 0.08948, 0.1532, 2.189, 1.098}},
                  {"ValveOne", {3, 0.94, 0.8432, 0.8495, 21.237, 6.379}},
                  {"ValveTwo", {2, 0.0558, 0.06257, 0.1094, 2.189, 1.067}}});
}

// Apply the rare event approximation.
TEST_F(RiskAnalysisTest, ImportanceRareEvent) {
  std::string with_prob = "./share/scram/input/fta/importance_test.xml";
  // Probability calculations with the rare event approximation.
  settings.approximation("rare-event").importance_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(0.012, p_total());  // Adjusted probability.
  TestImportance({{"PumpOne", {2, 0.12, 0.6, 0.624, 10.4, 2.5}},
                  {"PumpTwo", {2, 0.1, 0.5833, 0.6125, 8.75, 2.4}},
                  {"ValveOne", {2, 0.12, 0.4, 0.424, 10.6, 1.667}},
                  {"ValveTwo", {2, 0.1, 0.4167, 0.4458, 8.917, 1.714}}});
}

// Apply the minimal cut set upper bound approximation.
TEST_F(RiskAnalysisTest, Mcub) {
  std::string with_prob =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  // Probability calculations with the MCUB approximation.
  settings.approximation("mcub").importance_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_DOUBLE_EQ(0.766144, p_total());
}

// Apply the minimal cut set upper bound approximation for non-coherent tree.
// This should be a warning.
TEST_F(RiskAnalysisTest, McubNonCoherent) {
  std::string with_prob = "./share/scram/input/core/a_and_not_b.xml";
  // Probability calculations with the MCUB approximation.
  settings.approximation("mcub").probability_analysis(true);
  ASSERT_NO_THROW(ProcessInputFile(with_prob));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_NEAR(0.10, p_total(), 1e-5);
}

// Test Monte Carlo Analysis
/// @todo Expand this test.
TEST_P(RiskAnalysisTest, AnalyzeMC) {
  settings.uncertainty_analysis(true);
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
}

TEST_P(RiskAnalysisTest, AnalyzeProbabilityOverTime) {
  std::string tree_input = "./share/scram/input/core/single_exponential.xml";
  settings.probability_analysis(true).time_step(24).mission_time(120);
  std::vector<double> curve = {0,        2.399e-4, 4.7989e-4,
                               7.197e-4, 9.595e-4, 1.199e-3};
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  ASSERT_FALSE(analysis->probability_analyses().empty());
  auto it = curve.begin();
  double time = 0;
  for (const std::pair<double, double>& p_vs_time :
       analysis->probability_analyses().begin()->second->p_time()) {
    ASSERT_NE(curve.end(), it);
    if (time >= settings.mission_time()) {
      EXPECT_EQ(settings.mission_time(), p_vs_time.second);
    } else {
      EXPECT_EQ(time, p_vs_time.second);
    }
    EXPECT_NEAR(*it, p_vs_time.first, *it * 0.001);
    time += settings.time_step();
    ++it;
  }
  ASSERT_TRUE(time);
}

TEST_P(RiskAnalysisTest, AnalyzeSil) {
  std::string tree_input = "./share/scram/input/core/single_exponential.xml";
  settings.time_step(24).safety_integrity_levels(true);
  double pfd_fractions[] = {1.142e-4, 1.0275e-3, 1.02796e-2,
                            0.1033,   0.88527,   0};
  double pfh_fractions[] = {2.74e-7, 2.466e-6, 2.466e-5, 2.466e-4, 0.999726, 0};
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  ASSERT_FALSE(analysis->probability_analyses().empty());
  const auto& prob_an = *analysis->probability_analyses().begin()->second;
  EXPECT_NEAR(0.04255, prob_an.sil().pfd_avg, 0.00001);
  EXPECT_NEAR(9.77e-6, prob_an.sil().pfh_avg, 1e-8);
  auto compare_fractions = [](const auto& sil_fractions, const auto& result,
                              const char* type) {
    auto it = std::begin(sil_fractions);
    for (const std::pair<const double, double>& result_bucket : result) {
      ASSERT_NE(std::end(sil_fractions), it);
      EXPECT_NEAR(*it, result_bucket.second, *it * 0.001)
          << "The " << type << " bucket for " << result_bucket.first;
      ++it;
    }
    ASSERT_EQ(std::end(sil_fractions), it);
  };
  compare_fractions(pfd_fractions, prob_an.sil().pfd_fractions, "PFD");
  compare_fractions(pfh_fractions, prob_an.sil().pfh_fractions, "PFH");
}

// Test Reporting capabilities
// Tests the output against the schema. However the contents of the
// output are not verified or validated.
TEST_F(RiskAnalysisTest, ReportIOError) {
  std::string tree_input = "./share/scram/input/fta/correct_tree_input.xml";
  // Messing up the output file.
  std::string output = "abracadabra.cadabraabra/output.txt";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_THROW(Reporter().Report(*analysis, output), IOError);
}

// Reporting of the default analysis for MCS only without probabilities.
TEST_P(RiskAnalysisTest, ReportDefaultMCS) {
  CheckReport("./share/scram/input/fta/correct_tree_input.xml");
}

// Reporting of analysis for MCS with probability results.
TEST_F(RiskAnalysisTest, ReportProbability) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  settings.probability_analysis(true);
  CheckReport(tree_input);
}

TEST_F(RiskAnalysisTest, ReportProbabilityCurve) {
  std::string tree_input = "./share/scram/input/core/single_exponential.xml";
  settings.probability_analysis(true).time_step(24).mission_time(720);
  CheckReport(tree_input);
}

TEST_F(RiskAnalysisTest, ReportSil) {
  std::string tree_input = "./share/scram/input/core/single_exponential.xml";
  settings.time_step(24).safety_integrity_levels(true).mission_time(720);
  CheckReport(tree_input);
}

// Reporting of importance analysis.
TEST_F(RiskAnalysisTest, ReportImportanceFactors) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  settings.importance_analysis(true);
  CheckReport(tree_input);
}

// Reporting of uncertainty analysis.
TEST_F(RiskAnalysisTest, ReportUncertaintyResults) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  settings.uncertainty_analysis(true);
  CheckReport(tree_input);
}

// Reporting of CCF analysis.
TEST_F(RiskAnalysisTest, ReportCCF) {
  std::string tree_input = "./share/scram/input/core/mgl_ccf.xml";
  settings.ccf_analysis(true).importance_analysis(true);
  CheckReport(tree_input);
}

// Reporting of Negative events in MCS.
TEST_F(RiskAnalysisTest, ReportNegativeEvent) {
  std::string tree_input = "./share/scram/input/core/a_or_not_b.xml";
  settings.probability_analysis(true);
  CheckReport(tree_input);
}

// Reporting of all possible analyses.
TEST_F(RiskAnalysisTest, ReportAll) {
  std::string tree_input =
      "./share/scram/input/fta/correct_tree_input_with_probs.xml";
  settings.importance_analysis(true).uncertainty_analysis(true)
      .ccf_analysis(true);
  CheckReport(tree_input);
}

// Reporting with public or private roles.
TEST_F(RiskAnalysisTest, ReportRoles) {
  std::string tree_input = "./share/scram/input/fta/mixed_roles.xml";
  CheckReport(tree_input);
}

// Reporting of orphan primary events.
TEST_F(RiskAnalysisTest, ReportOrphanPrimaryEvents) {
  std::string tree_input = "./share/scram/input/fta/orphan_primary_event.xml";
  CheckReport(tree_input);
}

// Reporting of unused parameters.
TEST_F(RiskAnalysisTest, ReportUnusedParameters) {
  std::string tree_input = "./share/scram/input/fta/unused_parameter.xml";
  CheckReport(tree_input);
}

// NAND and NOR as a child cases.
TEST_P(RiskAnalysisTest, ChildNandNorGates) {
  std::string tree_input = "./share/scram/input/fta/children_nand_nor.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {
        {"not PumpOne", "not PumpTwo", "not ValveOne"},
        {"not PumpOne", "not ValveTwo", "not ValveOne"}};
    EXPECT_EQ(pi, products());
  } else {
    EXPECT_EQ(kUnity, products());
  }
}

// Simple test for several house event propagation.
TEST_P(RiskAnalysisTest, ManyHouseEvents) {
  std::string tree_input = "./share/scram/input/fta/constant_propagation.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  std::set<std::set<std::string>> mcs = {{"A", "B"}};
  EXPECT_EQ(mcs, products());
}

// Simple test for several constant gate propagation.
TEST_P(RiskAnalysisTest, ConstantGates) {
  std::string tree_input = "./share/scram/input/fta/constant_gates.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  EXPECT_EQ(kUnity, products());
}

// Mixed roles with undefined event types
TEST_F(RiskAnalysisTest, UndefinedEventsMixedRoles) {
  std::string tree_input =
      "./share/scram/input/fta/ambiguous_events_with_roles.xml";
  ASSERT_NO_THROW(ProcessInputFile(tree_input));
  ASSERT_NO_THROW(analysis->Analyze());
  std::set<std::set<std::string>> mcs = {
      {"C", "Ambiguous.Private.A", "Ambiguous.Private.B"},
      {"G", "Ambiguous.Private.A", "Ambiguous.Private.B"}};
  EXPECT_EQ(mcs, products());
}

}  // namespace test
}  // namespace core
}  // namespace scram
