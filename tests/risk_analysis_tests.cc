/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

#include <utility>

#include <boost/filesystem.hpp>

#include "env.h"
#include "error.h"
#include "initializer.h"
#include "reporter.h"
#include "xml.h"

namespace fs = boost::filesystem;

namespace scram::core::test {

const char* scram::core::test::RiskAnalysisTest::parameter_ = nullptr;

const std::set<std::set<std::string>> RiskAnalysisTest::kUnity = {
    std::set<std::string>{}};

RiskAnalysisTest::RiskAnalysisTest() {
  if (HasParam()) {
    std::string_view param = GetParam();
    if (param == "pi") {
      settings.algorithm("bdd");
      settings.prime_implicants(true);
    } else {
      settings.algorithm(param);
    }
  }
}

void RiskAnalysisTest::ProcessInputFiles(
    const std::vector<std::string>& input_files, bool allow_extern) {
  mef::Initializer init(input_files, settings, allow_extern);
  model = init.model();
  analysis = std::make_unique<RiskAnalysis>(model.get(), settings);
  result_ = Result();
}

void RiskAnalysisTest::CheckReport(const std::vector<std::string>& tree_input) {
  static xml::Validator validator(env::report_schema());

  REQUIRE_NOTHROW(ProcessInputFiles(tree_input));
  REQUIRE_NOTHROW(analysis->Analyze());
  fs::path unique_name = "scram_report_test-" + fs::unique_path().string();
  fs::path temp_file = fs::temp_directory_path() / unique_name;
  INFO("input: " + tree_input.front());
  INFO("output: " + temp_file.string());
  REQUIRE_NOTHROW(Reporter().Report(*analysis, temp_file.string()));
  REQUIRE_NOTHROW(xml::Document(temp_file.string(), &validator));
  fs::remove(temp_file);
}

const std::set<std::set<std::string>>& RiskAnalysisTest::products() {
  assert(analysis->results().size() == 1);
  if (result_.products.empty()) {
    for (const Product& product :
         analysis->results().front().fault_tree_analysis->products()) {
      result_.products.emplace(Convert(product));
    }
  }
  return result_.products;
}

std::vector<int> RiskAnalysisTest::ProductDistribution() {
  assert(analysis->results().size() == 1);
  return analysis->results()
      .front()
      .fault_tree_analysis->products()
      .Distribution();
}

void RiskAnalysisTest::PrintProducts() {
  assert(analysis->results().size() == 1);
  Print(analysis->results().front().fault_tree_analysis->products());
}

const std::map<std::set<std::string>, double>&
RiskAnalysisTest::product_probability() {
  assert(analysis->results().size() == 1);
  if (result_.product_probability.empty()) {
    for (const Product& product :
         analysis->results().front().fault_tree_analysis->products()) {
      result_.product_probability.emplace(Convert(product), product.p());
    }
  }
  return result_.product_probability;
}

std::map<std::string, double> RiskAnalysisTest::sequences() {
  assert(model->alignments().empty());
  assert(analysis->event_tree_results().size() == 1);
  std::map<std::string, double> results;
  for (const core::EventTreeAnalysis::Result& result :
       analysis->event_tree_results()
           .front()
           .event_tree_analysis->sequences()) {
    results.emplace(result.sequence.name(), result.p_sequence);
  }
  return results;
}

std::set<std::string> RiskAnalysisTest::Convert(const Product& product) {
  std::set<std::string> string_set;
  for (const Literal& literal : product) {
    string_set.insert((literal.complement ? "not " : "") + literal.event.id());
  }
  return string_set;
}

TEST_F(RiskAnalysisTest, ProcessInput) {
  std::string tree_input = "tests/input/fta/correct_tree_input.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  CHECK(gates().size() == 3);
  CHECK(gates().count("TrainOne") == 1);
  CHECK(gates().count("TrainTwo") == 1);
  CHECK(gates().count("TopEvent") == 1);
  CHECK(basic_events().size() == 4);
  CHECK(basic_events().count("PumpOne") == 1);
  CHECK(basic_events().count("PumpTwo") == 1);
  CHECK(basic_events().count("ValveOne") == 1);
  CHECK(basic_events().count("ValveTwo") == 1);

  REQUIRE(gates().count("TopEvent"));
  mef::Gate* top = gates().find("TopEvent")->get();
  CHECK(top->id() == "TopEvent");
  CHECK(top->formula().connective() == mef::kAnd);
  CHECK(top->formula().args().size() == 2);

  REQUIRE(gates().count("TrainOne"));
  mef::Gate* inter = gates().find("TrainOne")->get();
  CHECK(inter->id() == "TrainOne");
  CHECK(inter->formula().connective() == mef::kOr);
  CHECK(inter->formula().args().size() == 2);

  REQUIRE(basic_events().count("ValveOne"));
  mef::BasicEvent* primary = basic_events().find("ValveOne")->get();
  CHECK(primary->id() == "ValveOne");
}

// Test Probability Assignment
TEST_F(RiskAnalysisTest, PopulateProbabilities) {
  // Input with probabilities
  std::string tree_input = "tests/input/fta/correct_tree_input_with_probs.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE(basic_events().size() == 4);
  REQUIRE(basic_events().count("PumpOne") == 1);
  REQUIRE(basic_events().count("PumpTwo") == 1);
  REQUIRE(basic_events().count("ValveOne") == 1);
  REQUIRE(basic_events().count("ValveTwo") == 1);

  mef::BasicEvent* p1 = basic_events().find("PumpOne")->get();
  mef::BasicEvent* p2 = basic_events().find("PumpTwo")->get();
  mef::BasicEvent* v1 = basic_events().find("ValveOne")->get();
  mef::BasicEvent* v2 = basic_events().find("ValveTwo")->get();

  REQUIRE_NOTHROW(p1->p());
  REQUIRE_NOTHROW(p2->p());
  REQUIRE_NOTHROW(v1->p());
  REQUIRE_NOTHROW(v2->p());
  CHECK(p1->p() == 0.6);
  CHECK(p2->p() == 0.7);
  CHECK(v1->p() == 0.4);
  CHECK(v2->p() == 0.5);
}

// Test Analysis of Two train system.
TEST_P(RiskAnalysisTest, AnalyzeDefault) {
  std::string tree_input = "tests/input/fta/correct_tree_input.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  std::set<std::set<std::string>> mcs = {{"PumpOne", "PumpTwo"},
                                         {"PumpOne", "ValveTwo"},
                                         {"PumpTwo", "ValveOne"},
                                         {"ValveOne", "ValveTwo"}};
  CHECK(products() == mcs);
  PrintProducts();  // Quick visual verification.
}

TEST_P(RiskAnalysisTest, AnalyzeNonCoherentDefault) {
  std::string tree_input = "tests/input/fta/correct_non_coherent.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {{"not PumpOne", "ValveOne"},
                                          {"PumpOne", "PumpTwo"},
                                          {"PumpOne", "ValveTwo"},
                                          {"PumpTwo", "ValveOne"},
                                          {"ValveOne", "ValveTwo"}};
    CHECK(products().size() == 5);
    CHECK(products() == pi);
  } else {
    std::set<std::set<std::string>> mcs = {
        {"PumpOne", "PumpTwo"}, {"PumpOne", "ValveTwo"}, {"ValveOne"}};
    CHECK(products() == mcs);
  }
}

TEST_P(RiskAnalysisTest, AnalyzeWithProbability) {
  std::string with_prob = "tests/input/fta/correct_tree_input_with_probs.xml";
  std::set<std::string> mcs_1 = {"PumpOne", "PumpTwo"};
  std::set<std::string> mcs_2 = {"PumpOne", "ValveTwo"};
  std::set<std::string> mcs_3 = {"PumpTwo", "ValveOne"};
  std::set<std::string> mcs_4 = {"ValveOne", "ValveTwo"};
  std::set<std::set<std::string>> mcs = {mcs_1, mcs_2, mcs_3, mcs_4};
  settings.probability_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({with_prob}));
  REQUIRE_NOTHROW(analysis->Analyze());

  CHECK(products() == mcs);
  if (settings.approximation() == Approximation::kRareEvent) {
    CHECK(p_total() == Approx(1));
  } else {
    CHECK(p_total() == Approx(0.646));
  }
  CHECK(product_probability().at(mcs_1) == Approx(0.42));
  CHECK(product_probability().at(mcs_2) == Approx(0.3));
  CHECK(product_probability().at(mcs_3) == Approx(0.28));
  CHECK(product_probability().at(mcs_4) == Approx(0.2));
}

// Test for exact probability calculation
// regardless of the qualitative analysis algorithm.
TEST_P(RiskAnalysisTest, EnforceExactProbability) {
  std::string with_prob = "tests/input/fta/correct_tree_input_with_probs.xml";
  settings.probability_analysis(true).approximation("none");
  REQUIRE_NOTHROW(ProcessInputFiles({with_prob}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(p_total() == Approx(0.646));
}

TEST_P(RiskAnalysisTest, AnalyzeNestedFormula) {
  std::string nested_input = "tests/input/fta/nested_not.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({nested_input}));
  REQUIRE_NOTHROW(analysis->Analyze());

  auto sets = [this]() -> std::set<std::set<std::string>> {
    if (settings.prime_implicants())
      return {{"not PumpOne", "ValveTwo", "PumpTwo", "not ValveOne"}};
    return {{"ValveTwo", "PumpTwo"}};
  }();
  CHECK(products() == sets);
}

TEST_F(RiskAnalysisTest, ImportanceDefault) {
  std::string with_prob = "tests/input/fta/correct_tree_input_with_probs.xml";
  settings.importance_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({with_prob}));
  REQUIRE_NOTHROW(analysis->Analyze());
  TestImportance({{"PumpOne", {2, 0.51, 0.4737, 0.7895, 1.316, 1.9}},
                  {"PumpTwo", {2, 0.38, 0.4118, 0.8235, 1.176, 1.7}},
                  {"ValveOne", {2, 0.34, 0.2105, 0.5263, 1.316, 1.267}},
                  {"ValveTwo", {2, 0.228, 0.1765, 0.5882, 1.176, 1.214}}});
}

TEST_F(RiskAnalysisTest, ImportanceNeg) {
  std::string tree_input = "tests/input/fta/importance_neg_test.xml";
  settings.prime_implicants(true).importance_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(p_total() == Approx(0.04459));
  // Check importance values with negative event.
  TestImportance({{"PumpOne", {3, 0.0765, 0.1029, 0.1568, 2.613, 1.115}},
                  {"PumpTwo", {2, 0.057, 0.08948, 0.1532, 2.189, 1.098}},
                  {"ValveOne", {3, 0.94, 0.8432, 0.8495, 21.237, 6.379}},
                  {"ValveTwo", {2, 0.0558, 0.06257, 0.1094, 2.189, 1.067}}});
}

TEST_P(RiskAnalysisTest, ImportanceSingleEvent) {
  std::string tree_input = "tests/input/core/null_a.xml";
  settings.importance_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  TestImportance({{"OnlyChild", {1, 1, 1, 1, 2, 0}}});
}

TEST_P(RiskAnalysisTest, ImportanceZeroProbability) {
  std::string tree_input = "tests/input/core/zero_prob.xml";
  settings.importance_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  TestImportance({{"A", {1, 1, 0, 0, 0, 0}}});
}

TEST_P(RiskAnalysisTest, ImportanceOneProbability) {
  std::string tree_input = "tests/input/core/one_prob.xml";
  settings.importance_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  TestImportance({{"A", {1, 1, 1, 1, 1, 0}}});
}

// Apply the rare event approximation.
TEST_F(RiskAnalysisTest, ImportanceRareEvent) {
  std::string with_prob = "tests/input/fta/importance_test.xml";
  // Probability calculations with the rare event approximation.
  settings.approximation("rare-event").importance_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({with_prob}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(p_total() == Approx(0.012));  // Adjusted probability.
  TestImportance({{"PumpOne", {2, 0.12, 0.6, 0.624, 10.4, 2.5}},
                  {"PumpTwo", {2, 0.1, 0.5833, 0.6125, 8.75, 2.4}},
                  {"ValveOne", {2, 0.12, 0.4, 0.424, 10.6, 1.667}},
                  {"ValveTwo", {2, 0.1, 0.4167, 0.4458, 8.917, 1.714}}});
}

// Apply the minimal cut set upper bound approximation.
TEST_F(RiskAnalysisTest, Mcub) {
  std::string with_prob = "tests/input/fta/correct_tree_input_with_probs.xml";
  // Probability calculations with the MCUB approximation.
  settings.approximation("mcub").importance_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({with_prob}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(p_total() == Approx(0.766144));
}

// Apply the minimal cut set upper bound approximation for non-coherent tree.
// This should be a warning.
TEST_F(RiskAnalysisTest, McubNonCoherent) {
  std::string with_prob = "tests/input/core/a_and_not_b.xml";
  // Probability calculations with the MCUB approximation.
  settings.approximation("mcub").probability_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({with_prob}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(p_total() == Approx(0.10));
}

// Test Monte Carlo Analysis
/// @todo Expand this test.
TEST_P(RiskAnalysisTest, AnalyzeMC) {
  settings.uncertainty_analysis(true);
  std::string tree_input = "tests/input/fta/correct_tree_input_with_probs.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
}

TEST_P(RiskAnalysisTest, AnalyzeProbabilityOverTime) {
  std::string tree_input = "tests/input/core/single_exponential.xml";
  settings.probability_analysis(true).time_step(24).mission_time(120);
  std::vector<double> curve = {0,        2.399e-4, 4.7989e-4,
                               7.197e-4, 9.595e-4, 1.199e-3};
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  REQUIRE_FALSE(analysis->results().empty());
  REQUIRE(analysis->results().front().probability_analysis);
  auto it = curve.begin();
  double time = 0;
  for (const std::pair<double, double>& p_vs_time :
       analysis->results().front().probability_analysis->p_time()) {
    REQUIRE_FALSE(it == curve.end());
    if (time >= settings.mission_time()) {
      CHECK(p_vs_time.second == settings.mission_time());
    } else {
      CHECK(p_vs_time.second == time);
    }
    CHECK(p_vs_time.first == Approx(*it).epsilon(1e-3));
    time += settings.time_step();
    ++it;
  }
  REQUIRE(time);
}

TEST_P(RiskAnalysisTest, AnalyzeSil) {
  std::string tree_input = "tests/input/core/single_exponential.xml";
  settings.time_step(24).safety_integrity_levels(true);
  double pfd_fractions[] = {1.142e-4, 1.0275e-3, 1.02796e-2,
                            0.1033,   0.88527,   0};
  double pfh_fractions[] = {2.74e-7, 2.466e-6, 2.466e-5, 2.466e-4, 0.999726, 0};
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  REQUIRE_FALSE(analysis->results().empty());
  REQUIRE(analysis->results().front().probability_analysis);
  const auto& prob_an = *analysis->results().front().probability_analysis;
  CHECK(prob_an.sil().pfd_avg == Approx(0.04255).epsilon(1e-3));
  CHECK(prob_an.sil().pfh_avg == Approx(9.77e-6).epsilon(1e-3));
  auto compare_fractions = [](const auto& sil_fractions, const auto& result,
                              const char* type) {
    auto it = std::begin(sil_fractions);
    for (const std::pair<const double, double>& result_bucket : result) {
      CAPTURE(type);
      INFO("bucket: " + std::to_string(result_bucket.first));
      REQUIRE_FALSE(it == std::end(sil_fractions));
      CHECK(result_bucket.second == Approx(*it).epsilon(1e-3));
      ++it;
    }
    REQUIRE(it == std::end(sil_fractions));
  };
  compare_fractions(pfd_fractions, prob_an.sil().pfd_fractions, "PFD");
  compare_fractions(pfh_fractions, prob_an.sil().pfh_fractions, "PFH");
}

TEST_F(RiskAnalysisTest, EventTreeCollectAtleastFormula) {
  const char* tree_input = "tests/input/eta/collect_atleast_formula.xml";
  settings.probability_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(analysis->event_tree_results().size() == 1);
}

TEST_F(RiskAnalysisTest, EventTreeCollectCardinalityFormula) {
  const char* tree_input = "tests/input/eta/collect_cardinality_formula.xml";
  settings.probability_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(analysis->event_tree_results().size() == 1);
}

TEST_P(RiskAnalysisTest, AnalyzeEventTree) {
  const char* tree_input = "input/EventTrees/bcd.xml";
  settings.probability_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(analysis->event_tree_results().size() == 1);
  const auto& results = sequences();
  REQUIRE(results.size() == 2);
  std::map<std::string, double> expected = {{"Success", 0.594},
                                            {"Failure", 0.406}};
  for (const auto& result : expected) {
    INFO("state: " + result.first);
    REQUIRE(results.count(result.first));
    CHECK(results.at(result.first) == Approx(result.second));
  }
}

TEST_P(RiskAnalysisTest, AnalyzeTestEventDefault) {
  const char* tree_input = "tests/input/eta/test_event_default.xml";
  settings.probability_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(analysis->event_tree_results().size() == 1);
  const auto& results = sequences();
  REQUIRE(results.size() == 1);
  CHECK(results.begin()->first == "S");
  CHECK(results.begin()->second == Approx(0.5));
}

TEST_P(RiskAnalysisTest, AnalyzeTestInitatingEvent) {
  const char* tree_input = "tests/input/eta/test_initiating_event.xml";
  settings.probability_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(analysis->event_tree_results().size() == 1);
  const auto& results = sequences();
  REQUIRE(results.size() == 1);
  CHECK(results.begin()->first == "S");
  CHECK(results.begin()->second == Approx(0.5));
}

TEST_P(RiskAnalysisTest, AnalyzeTestFunctionalEvent) {
  const char* tree_input[] = {"tests/input/eta/test_functional_event.xml",
                              "tests/input/eta/test_functional_event_link.xml"};
  settings.probability_analysis(true);
  for (auto input : tree_input) {
    REQUIRE_NOTHROW(ProcessInputFiles({input}));
    CAPTURE(input);
    REQUIRE_NOTHROW(analysis->Analyze());
    CHECK(analysis->event_tree_results().size() == 1);
    const auto& results = sequences();
    CAPTURE(input);
    REQUIRE(results.size() == 1);
    CHECK(results.begin()->first == "S");
    CHECK(results.begin()->second == Approx(0.5));
  }
}

// Test Reporting capabilities
// Tests the output against the schema. However the contents of the
// output are not verified or validated.
TEST_F(RiskAnalysisTest, ReportIOError) {
  std::string tree_input = "tests/input/fta/correct_tree_input.xml";
  // Messing up the output file.
  std::string output = "abracadabra.cadabraabra/output.txt";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK_THROWS_AS(Reporter().Report(*analysis, output), IOError);
}

TEST_F(RiskAnalysisTest, ReportEmpty) {
  std::string tree_input = "tests/input/empty_model.xml";
  CheckReport({tree_input});
}

// Reporting of the default analysis for MCS only without probabilities.
TEST_P(RiskAnalysisTest, ReportDefaultMCS) {
  CheckReport({"tests/input/fta/correct_tree_input.xml"});
}

// Reporting of analysis for MCS with probability results.
TEST_F(RiskAnalysisTest, ReportProbability) {
  std::string tree_input = "tests/input/fta/correct_tree_input_with_probs.xml";
  settings.probability_analysis(true);
  CheckReport({tree_input});
}

TEST_F(RiskAnalysisTest, ReportProbabilityCurve) {
  std::string tree_input = "tests/input/core/single_exponential.xml";
  settings.probability_analysis(true).time_step(24).mission_time(720);
  CheckReport({tree_input});
}

TEST_F(RiskAnalysisTest, ReportSil) {
  std::string tree_input = "tests/input/core/single_exponential.xml";
  settings.time_step(24).safety_integrity_levels(true).mission_time(720);
  CheckReport({tree_input});
}

// Reporting of importance analysis.
TEST_F(RiskAnalysisTest, ReportImportanceFactors) {
  std::string tree_input = "tests/input/fta/correct_tree_input_with_probs.xml";
  settings.importance_analysis(true);
  CheckReport({tree_input});
}

// Reporting of uncertainty analysis.
TEST_F(RiskAnalysisTest, ReportUncertaintyResults) {
  std::string tree_input = "tests/input/fta/correct_tree_input_with_probs.xml";
  settings.uncertainty_analysis(true);
  CheckReport({tree_input});
}

// Reporting event tree analysis with an initiating event.
TEST_F(RiskAnalysisTest, ReportInitiatingEventAnalysis) {
  const char* tree_input = "input/EventTrees/bcd.xml";
  settings.probability_analysis(true);
  CheckReport({tree_input});
}

// Reporting of CCF analysis.
TEST_F(RiskAnalysisTest, ReportCCF) {
  std::string tree_input = "tests/input/core/mgl_ccf.xml";
  settings.ccf_analysis(true).importance_analysis(true);
  CheckReport({tree_input});
}

// Reporting of Negative events in MCS.
TEST_F(RiskAnalysisTest, ReportNegativeEvent) {
  std::string tree_input = "tests/input/core/a_or_not_b.xml";
  settings.probability_analysis(true);
  CheckReport({tree_input});
}

// Reporting of all possible analyses.
TEST_F(RiskAnalysisTest, ReportAll) {
  std::string tree_input = "tests/input/fta/correct_tree_input_with_probs.xml";
  settings.importance_analysis(true).uncertainty_analysis(true).ccf_analysis(
      true);
  CheckReport({tree_input});
}

// Reporting with public or private roles.
TEST_F(RiskAnalysisTest, ReportRoles) {
  std::string tree_input = "tests/input/fta/mixed_roles.xml";
  CheckReport({tree_input});
}

// Reporting of orphan primary events.
TEST_F(RiskAnalysisTest, ReportOrphanPrimaryEvents) {
  std::string tree_input = "tests/input/fta/orphan_primary_event.xml";
  CheckReport({tree_input});
}

// Reporting of unused parameters.
TEST_F(RiskAnalysisTest, ReportUnusedParameters) {
  std::string tree_input = "tests/input/fta/unused_parameter.xml";
  CheckReport({tree_input});
}

TEST_F(RiskAnalysisTest, ReportUnusedEventTreeElements) {
  std::string tree_input = "tests/input/eta/unused_elements.xml";
  CheckReport({tree_input});
}

TEST_F(RiskAnalysisTest, ReportAlignment) {
  std::string tree_input = "input/TwoTrain/two_train_alignment.xml";
  CheckReport({tree_input});
}

TEST_F(RiskAnalysisTest, ReportAlignmentEventTree) {
  std::string dir = "input/EventTrees/";
  settings.probability_analysis(true);
  CheckReport({dir + "attack_alignment.xml", dir + "attack.xml"});
}

// NAND and NOR as a child cases.
TEST_P(RiskAnalysisTest, ChildNandNorGates) {
  std::string tree_input = "tests/input/fta/children_nand_nor.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  if (settings.prime_implicants()) {
    std::set<std::set<std::string>> pi = {
        {"not PumpOne", "not PumpTwo", "not ValveOne"},
        {"not PumpOne", "not ValveTwo", "not ValveOne"}};
    CHECK(products() == pi);
  } else {
    CHECK(products() == kUnity);
  }
}

// Simple test for several house event propagation.
TEST_P(RiskAnalysisTest, ManyHouseEvents) {
  std::string tree_input = "tests/input/fta/constant_propagation.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  std::set<std::set<std::string>> mcs = {{"A", "B"}};
  CHECK(products() == mcs);
}

// Simple test for several constant gate propagation.
TEST_P(RiskAnalysisTest, ConstantGates) {
  std::string tree_input = "tests/input/fta/constant_gates.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  CHECK(products() == kUnity);
}

// Mixed roles with undefined event types
TEST_F(RiskAnalysisTest, UndefinedEventsMixedRoles) {
  std::string tree_input = "tests/input/fta/ambiguous_events_with_roles.xml";
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}));
  REQUIRE_NOTHROW(analysis->Analyze());
  std::set<std::set<std::string>> mcs = {
      {"C", "Ambiguous.Private.A", "Ambiguous.Private.B"},
      {"G", "Ambiguous.Private.A", "Ambiguous.Private.B"}};
  CHECK(products() == mcs);
}

// Extern function call check.
TEST_P(RiskAnalysisTest, ExternFunctionProbability) {
  std::string tree_input = "tests/input/model/extern_full_check.xml";
  settings.probability_analysis(true);
  REQUIRE_NOTHROW(ProcessInputFiles({tree_input}, true));
  REQUIRE_NOTHROW(analysis->Analyze());
  std::set<std::set<std::string>> mcs = {{"e1"}};
  CHECK(products() == mcs);
  CHECK(p_total() == Approx(0.1));
}

}  // namespace scram::core::test
