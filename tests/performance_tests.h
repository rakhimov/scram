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

#ifndef SCRAM_TESTS_PERFORMANCE_TESTS_H_
#define SCRAM_TESTS_PERFORMANCE_TESTS_H_

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "fault_tree_analysis.h"
#include "initializer.h"
#include "probability_analysis.h"
#include "risk_analysis.h"

namespace scram {
namespace core {
namespace test {

class PerformanceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    settings.algorithm("mocus");
    delta = 0.10;  // % variation of values.
  }

  void TearDown() override {}

  // Convenient function to manage analysis of one model in input files.
  void Analyze(const std::vector<std::string>& input_files) {
    {
      mef::Initializer init(input_files, settings);
      analysis = std::make_unique<RiskAnalysis>(init.model(), settings);
    }
    analysis->Analyze();
  }

  // Total probability as a result of analysis.
  double p_total() {
    assert(!analysis->probability_analyses().empty());
    return analysis->probability_analyses().begin()->second->p_total();
  }

  // The number of products as a result of analysis.
  int NumOfProducts() {
    assert(!analysis->fault_tree_analyses().empty());
    return analysis->fault_tree_analyses().begin()->second->products().size();
  }

  // Time taken to find products.
  double ProductGenerationTime() {
    assert(!analysis->fault_tree_analyses().empty());
    return analysis->fault_tree_analyses().begin()->second->analysis_time();
  }

  // Time taken to calculate total probability.
  double ProbabilityCalculationTime() {
    assert(!analysis->probability_analyses().empty());
    return analysis->probability_analyses().begin()->second->analysis_time();
  }

  std::unique_ptr<RiskAnalysis> analysis;
  Settings settings;
  double delta;  // The range indicator for values.
};

}  // namespace test
}  // namespace core
}  // namespace scram

#endif  // SCRAM_TESTS_PERFORMANCE_TESTS_H_
