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

#include <set>
#include <vector>

#include <boost/range/algorithm.hpp>
#include <gtest/gtest.h>

namespace scram {
namespace core {
namespace test {

class RiskAnalysisTest : public ::testing::TestWithParam<const char*> {
 protected:
  using ImportanceContainer =
      std::vector<std::pair<std::string, ImportanceFactors>>;

  static const std::set<std::set<std::string>> kUnity;  ///< Special unity set.

  void SetUp() override;

  // Parsing multiple input files.
  void ProcessInputFiles(const std::vector<std::string>& input_files);

  // Parsing an input file to get the model.
  void ProcessInputFile(const std::string& input_file) {
    ProcessInputFiles({input_file});
  }

  // Collection of assertions on the reporting after running analysis.
  // Note that the analysis is run by this function.
  void CheckReport(const std::string& tree_input);

  // Returns a single fault tree, assuming one fault tree with single top gate.
  const mef::FaultTreePtr& fault_tree() {
    return *model->fault_trees().begin();
  }

  const mef::IdTable<mef::GatePtr>& gates() { return model->gates(); }

  const mef::IdTable<mef::HouseEventPtr>& house_events() {
    return model->house_events();
  }

  const mef::IdTable<mef::BasicEventPtr>& basic_events() {
    return model->basic_events();
  }

  /// @returns The resultant products of the fault tree analysis.
  const std::set<std::set<std::string>>& products();

  // Provides the number of products per order of sets.
  // The order starts from 1.
  std::vector<int> ProductDistribution();

  /// Prints products to the standard error.
  void PrintProducts();

  double p_total() {
    assert(!analysis->probability_analyses().empty());
    assert(analysis->probability_analyses().size() == 1);
    return analysis->probability_analyses().begin()->second->p_total();
  }

  /// @returns Products and their probabilities.
  const std::map<std::set<std::string>, double>& product_probability();

  const ImportanceFactors& importance(const std::string& id) {
    assert(!analysis->importance_analyses().empty());
    assert(analysis->importance_analyses().size() == 1);
    const auto& importance =
        analysis->importance_analyses().begin()->second->importance();
    auto it = boost::find_if(importance, [&id](const ImportanceRecord& record) {
      return record.event.id() == id;
    });
    assert(it != importance.end());
    return it->factors;
  }

  void TestImportance(const ImportanceContainer& expected) {
#define IMP_EQ(field) \
  EXPECT_NEAR(test.field, result.field, (1e-3 * result.field)) << entry.first
    for (const auto& entry : expected) {
      const ImportanceFactors& result = importance(entry.first);
      const ImportanceFactors& test = entry.second;
      EXPECT_EQ(test.occurrence, result.occurrence) << entry.first;
      IMP_EQ(mif);
      IMP_EQ(cif);
      IMP_EQ(dif);
      IMP_EQ(raw);
      IMP_EQ(rrw);
    }
#undef IMP_EQ
  }

  // Uncertainty analysis.
  double mean() {
    assert(!analysis->uncertainty_analyses().empty());
    assert(analysis->uncertainty_analyses().size() == 1);
    return analysis->uncertainty_analyses().begin()->second->mean();
  }

  double sigma() {
    assert(!analysis->uncertainty_analyses().empty());
    assert(analysis->uncertainty_analyses().size() == 1);
    return analysis->uncertainty_analyses().begin()->second->sigma();
  }

  // Members
  std::unique_ptr<RiskAnalysis> analysis;
  std::shared_ptr<mef::Model> model;
  Settings settings;

 private:
  /// Converts a set of pointers to events with complement flags
  /// into readable and testable strings.
  /// Complements are communicated with "not" prefix.
  std::set<std::string> Convert(const Product& product);

  struct Result {
    std::map<std::set<std::string>, double> product_probability;
    std::set<std::set<std::string>> products;
  } result_;
};

}  // namespace test
}  // namespace core
}  // namespace scram

#endif  // SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_
