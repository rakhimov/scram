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
namespace test {

class PerformanceTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    settings.algorithm("mocus");
    delta = 0.10;  // % variation of values.
  }

  virtual void TearDown() {
    delete ran;
  }

  // Convenient function to manage analysis of one model in input files.
  void Analyze(const std::vector<std::string>& input_files) {
    Initializer* init = new Initializer(settings);
    init->ProcessInputFiles(input_files);
    ran = new RiskAnalysis(init->model(), settings);
    delete init;
    ran->Analyze();
  }

  // Convenient function to manage analysis of one model in one input file.
  void Analyze(const std::string& input_file) {
    std::vector<std::string> input_files;
    input_files.push_back(input_file);
    Analyze(input_files);
  }

  // Total probability as a result of analysis.
  double p_total() {
    assert(!ran->probability_analyses().empty());
    return ran->probability_analyses().begin()->second->p_total();
  }

  // The number of MCS as a result of analysis.
  int NumOfMcs() {
    assert(!ran->fault_tree_analyses().empty());
    return ran->fault_tree_analyses().begin()->second->cut_sets().size();
  }

  // Time taken to find minimal cut sets.
  double McsGenerationTime() {
    assert(!ran->fault_tree_analyses().empty());
    return ran->fault_tree_analyses().begin()->second->analysis_time();
  }

  // Time taken to calculate total probability.
  double ProbCalcTime() {
    assert(!ran->probability_analyses().empty());
    return ran->probability_analyses().begin()->second->analysis_time();
  }

  RiskAnalysis* ran;
  Settings settings;
  double delta;  // The range indicator for values.
};

}  // namespace test
}  // namespace scram

#endif  // SCRAM_TESTS_PERFORMANCE_TESTS_H_
