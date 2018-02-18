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

#pragma once

#include "risk_analysis.h"

#include <set>
#include <vector>

#include <catch.hpp>

#include <boost/range/algorithm.hpp>

/// Transitional macros from GoogleTest.
#define TEST_F(Fixture, Name) \
  TEST_CASE_METHOD(Fixture, #Fixture "." #Name, "[risk]")
/// Parametrized tests by the by analysis algorithms.
#define TEST_P(Fixture, Name) \
  TEST_CASE_METHOD(Fixture, #Fixture "." #Name, "[risk][bdd][pi][mocus][zbdd]")
#define ASSERT_NO_THROW REQUIRE_NOTHROW
#define EXPECT_NEAR(expected, value, delta) \
  CHECK((value) == Approx(expected).margin(delta))
#define EXPECT_EQ(expected, value) CHECK((value) == (expected))
#define EXPECT_DOUBLE_EQ(expected, value) CHECK((value) == Approx(expected))
#define ASSERT_EQ(expected, value) REQUIRE((value) == (expected))
#define ASSERT_DOUBLE_EQ(expected, value) REQUIRE((value) == Approx(expected))
#define EXPECT_TRUE CHECK
#define ASSERT_TRUE REQUIRE

int main(int argc, char* argv[]);  ///< Sets the parameter.

namespace scram::core::test {

class RiskAnalysisTest {
  friend int ::main(int argc, char* argv[]);  ///< Sets the parameter.
  static const char* parameter_;  ///< Algorithm parameter.

 public:
  using ImportanceContainer =
      std::vector<std::pair<std::string, ImportanceFactors>>;

  static const std::set<std::set<std::string>> kUnity;  ///< Special unity set.

  RiskAnalysisTest();

 protected:
  // Parsing multiple input files.
  void ProcessInputFiles(const std::vector<std::string>& input_files,
                         bool allow_extern = false);

  // Collection of assertions on the reporting after running analysis.
  // Note that the analysis is run by this function.
  void CheckReport(const std::vector<std::string>& tree_input);

  // Returns a single fault tree, assuming one fault tree with single top gate.
  const mef::FaultTree& fault_tree() { return *model->fault_trees().begin(); }
  auto gates() { return model->gates(); }
  auto house_events() { return model->house_events(); }
  auto basic_events() { return model->basic_events(); }

  /// @returns The resultant products of the fault tree analysis.
  const std::set<std::set<std::string>>& products();

  // Provides the number of products per order of sets.
  // The order starts from 1.
  const std::vector<int>& ProductDistribution();

  /// Prints products to the standard error.
  void PrintProducts();

  double p_total() {
    assert(analysis->results().size() == 1);
    assert(analysis->results().front().probability_analysis);
    return analysis->results().front().probability_analysis->p_total();
  }

  /// @returns Products and their probabilities.
  const std::map<std::set<std::string>, double>& product_probability();

  const ImportanceFactors& importance(const std::string& id) {
    assert(analysis->results().size() == 1);
    assert(analysis->results().front().importance_analysis);
    const auto& importance =
        analysis->results().front().importance_analysis->importance();
    auto it = boost::find_if(importance, [&id](const ImportanceRecord& record) {
      return record.event.id() == id;
    });
    assert(it != importance.end());
    return it->factors;
  }

  void TestImportance(const ImportanceContainer& expected) {
#define IMP_EQ(field) CHECK(result.field == Approx(test.field).epsilon(1e-3))
    for (const auto& entry : expected) {
      INFO("event: " + entry.first);
      const ImportanceFactors& result = importance(entry.first);
      const ImportanceFactors& test = entry.second;
      CHECK(result.occurrence == test.occurrence);
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
    assert(analysis->results().size() == 1);
    assert(analysis->results().front().uncertainty_analysis);
    return analysis->results().front().uncertainty_analysis->mean();
  }

  double sigma() {
    assert(analysis->results().size() == 1);
    assert(analysis->results().front().uncertainty_analysis);
    return analysis->results().front().uncertainty_analysis->sigma();
  }

  /// @returns The event-tree analysis sequence results.
  std::map<std::string, double> sequences();

  // Members
  std::unique_ptr<RiskAnalysis> analysis;
  std::unique_ptr<mef::Model> model;
  Settings settings;

 private:
  /// Converts a set of pointers to events with complement flags
  /// into readable and testable strings.
  /// Complements are communicated with "not" prefix.
  std::set<std::string> Convert(const Product& product);

  /// @todo Provide parametrized tests.
  /// @{
  bool HasParam() { return parameter_; }
  const char* GetParam() { return parameter_; }
  /// @}

  struct Result {
    std::map<std::set<std::string>, double> product_probability;
    std::set<std::set<std::string>> products;
  } result_;
};

}  // namespace scram::core::test
