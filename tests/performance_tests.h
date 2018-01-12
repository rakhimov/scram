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

#include <string>
#include <vector>

#include <catch.hpp>

#include "fault_tree_analysis.h"
#include "initializer.h"
#include "probability_analysis.h"
#include "risk_analysis.h"

namespace scram::core::test {

class PerformanceTest {
 public:
  PerformanceTest() {
    settings.algorithm("mocus");
    delta = 0.10;  // % variation of values.
  }

 protected:
  // Convenient function to manage analysis of one model in input files.
  void Analyze(const std::vector<std::string>& input_files) {
    {
      model = mef::Initializer(input_files, settings).model();
      analysis = std::make_unique<RiskAnalysis>(model.get(), settings);
    }
    analysis->Analyze();
  }

  // Total probability as a result of analysis.
  double p_total() {
    assert(analysis->results().size() == 1);
    assert(analysis->results().front().probability_analysis);
    return analysis->results().front().probability_analysis->p_total();
  }

  // The number of products as a result of analysis.
  int NumOfProducts() {
    assert(analysis->results().size() == 1);
    return analysis->results().front().fault_tree_analysis->products().size();
  }

  // Time taken to find products.
  double ProductGenerationTime() {
    assert(analysis->results().size() == 1);
    return analysis->results().front().fault_tree_analysis->analysis_time();
  }

  // Time taken to calculate total probability.
  double ProbabilityCalculationTime() {
    assert(analysis->results().size() == 1);
    assert(analysis->results().front().probability_analysis);
    return analysis->results().front().probability_analysis->analysis_time();
  }

  std::shared_ptr<mef::Model> model;
  std::unique_ptr<RiskAnalysis> analysis;
  Settings settings;
  double delta;  // The range indicator for values.
};

}  // namespace scram::core::test
