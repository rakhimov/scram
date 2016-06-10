/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

#ifndef SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_
#define SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_

#include "risk_analysis.h"

#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "env.h"
#include "initializer.h"
#include "xml_parser.h"

namespace scram {
namespace core {
namespace test {

class RiskAnalysisTest : public ::testing::TestWithParam<const char*> {
 protected:
  void SetUp() override {
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

  void TearDown() override {}

  // Parsing multiple input files.
  void ProcessInputFiles(const std::vector<std::string>& input_files) {
    ResetInitializer();
    init->ProcessInputFiles(input_files);
    ResetRiskAnalysis();
  }

  // Resets the initializer with the settings of the object.
  void ResetInitializer() {
    init = std::unique_ptr<mef::Initializer>(new mef::Initializer(settings));
  }

  // Resets the risk analysis with the initialized model and settings.
  void ResetRiskAnalysis() {
    assert(init && "Missing initializer");
    ran = std::unique_ptr<RiskAnalysis>(
        new RiskAnalysis(init->model(), settings));
  }

  // Parsing an input file to get the model.
  void ProcessInputFile(const std::string& input_file) {
    std::vector<std::string> input_files;
    input_files.push_back(input_file);
    ProcessInputFiles(input_files);
  }

  // Collection of assertions on the reporting after running analysis.
  // Note that the analysis is run by this function.
  void CheckReport(const std::string& tree_input) {
    std::stringstream schema;
    std::string schema_path = Env::report_schema();
    std::ifstream schema_stream(schema_path.c_str());
    schema << schema_stream.rdbuf();
    schema_stream.close();

    ASSERT_NO_THROW(ProcessInputFile(tree_input));
    ASSERT_NO_THROW(ran->Analyze());
    std::stringstream output;
    ASSERT_NO_THROW(ran->Report(output));

    std::unique_ptr<XmlParser> parser;
    ASSERT_NO_THROW(parser = std::unique_ptr<XmlParser>(new XmlParser(output)));
    ASSERT_NO_THROW(parser->Validate(schema));
  }

  // Returns a single fault tree, assuming one fault tree with single top gate.
  const mef::FaultTreePtr& fault_tree() {
    return init->model()->fault_trees().begin()->second;
  }

  const std::unordered_map<std::string, mef::GatePtr>& gates() {
    return init->model()->gates();
  }

  const std::unordered_map<std::string, mef::HouseEventPtr>& house_events() {
    return init->model()->house_events();
  }

  const std::unordered_map<std::string, mef::BasicEventPtr>& basic_events() {
    return init->model()->basic_events();
  }

  const std::set<std::set<std::string>>& products() {
    assert(!ran->fault_tree_analyses().empty());
    assert(ran->fault_tree_analyses().size() == 1);
    if (products_.empty()) {
      const FaultTreeAnalysis* fta =
          ran->fault_tree_analyses().begin()->second.get();
      for (const Product& product : fta->products()) {
        products_.emplace(Convert(product));
      }
    }
    return products_;
  }

  // Provides the number of products per order of sets.
  // The order starts from 1.
  std::vector<int> ProductDistribution() {
    assert(!ran->fault_tree_analyses().empty());
    assert(ran->fault_tree_analyses().size() == 1);
    std::vector<int> distr(settings.limit_order(), 0);
    const FaultTreeAnalysis* fta =
        ran->fault_tree_analyses().begin()->second.get();
    for (const Product& product : fta->products()) {
      distr[GetOrder(product) - 1]++;
    }
    while (!distr.empty() && !distr.back()) distr.pop_back();
    return distr;
  }

  /// Prints products to the standard error.
  void PrintProducts() {
    assert(!ran->fault_tree_analyses().empty());
    assert(ran->fault_tree_analyses().size() == 1);
    const FaultTreeAnalysis* fta =
        ran->fault_tree_analyses().begin()->second.get();
    Print(fta->products());
  }

  double p_total() {
    assert(!ran->probability_analyses().empty());
    assert(ran->probability_analyses().size() == 1);
    return ran->probability_analyses().begin()->second->p_total();
  }

  const std::map<std::set<std::string>, double>& product_probability() {
    assert(!ran->fault_tree_analyses().empty());
    assert(ran->fault_tree_analyses().size() == 1);
    if (product_probability_.empty()) {
      const FaultTreeAnalysis* fta =
          ran->fault_tree_analyses().begin()->second.get();
      for (const Product& product : fta->products()) {
        product_probability_.emplace(Convert(product),
                                     CalculateProbability(product));
      }
    }
    return product_probability_;
  }

  const ImportanceFactors& importance(std::string id) {
    assert(!ran->importance_analyses().empty());
    assert(ran->importance_analyses().size() == 1);
    return ran->importance_analyses().begin()->second->importance().at(id);
  }

  // Uncertainty analysis.
  double mean() {
    assert(!ran->uncertainty_analyses().empty());
    assert(ran->uncertainty_analyses().size() == 1);
    return ran->uncertainty_analyses().begin()->second->mean();
  }

  double sigma() {
    assert(!ran->uncertainty_analyses().empty());
    assert(ran->uncertainty_analyses().size() == 1);
    return ran->uncertainty_analyses().begin()->second->sigma();
  }

  /// Converts a set of pointers to events with complement flags
  /// into readable and testable strings.
  /// Complements are communicated with "not" prefix.
  std::set<std::string> Convert(const Product& product) {
    std::set<std::string> string_set;
    for (const Literal& literal : product) {
      string_set.insert((literal.complement ? "not " : "") +
                        literal.event.id());
    }
    return string_set;
  }

  // Members
  std::unique_ptr<RiskAnalysis> ran;
  std::unique_ptr<mef::Initializer> init;
  Settings settings;

 private:
  std::map<std::set<std::string>, double> product_probability_;
  std::set<std::set<std::string>> products_;
};

}  // namespace test
}  // namespace core
}  // namespace scram

#endif  // SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_
