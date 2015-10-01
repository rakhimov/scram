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

#ifndef SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_
#define SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_

#include "initializer.h"
#include "risk_analysis.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace scram {
namespace test {

using EventPtr = std::shared_ptr<Event>;
using GatePtr = std::shared_ptr<Gate>;
using HouseEventPtr = std::shared_ptr<HouseEvent>;
using BasicEventPtr = std::shared_ptr<BasicEvent>;
using FaultTreePtr = std::unique_ptr<FaultTree>;

class RiskAnalysisTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}

  virtual void TearDown() {
    delete init;
    delete ran;
  }

  // Parsing multiple input files.
  void ProcessInputFiles(const std::vector<std::string>& input_files) {
    init = new Initializer(settings);
    init->ProcessInputFiles(input_files);
    ran = new RiskAnalysis(init->model(), settings);
  }

  // Parsing an input file to get the model.
  void ProcessInputFile(const std::string& input_file) {
    std::vector<std::string> input_files;
    input_files.push_back(input_file);
    ProcessInputFiles(input_files);
  }

  // Returns a single fault tree, assuming one fault tree with single top gate.
  const FaultTreePtr& fault_tree() {
    return init->model()->fault_trees().begin()->second;
  }

  const std::unordered_map<std::string, GatePtr>& gates() {
    return init->model()->gates();
  }

  const std::unordered_map<std::string, HouseEventPtr>& house_events() {
    return init->model()->house_events();
  }

  const std::unordered_map<std::string, BasicEventPtr>& basic_events() {
    return init->model()->basic_events();
  }

  const std::set< std::set<std::string> >& min_cut_sets() {
    assert(!ran->fault_tree_analyses().empty());
    assert(ran->fault_tree_analyses().size() == 1);
    return ran->fault_tree_analyses().begin()->second->min_cut_sets();
  }

  // Provides the number of minimal cut sets per order of sets.
  // The order starts from 0.
  std::vector<int> McsDistribution() {
    assert(!ran->fault_tree_analyses().empty());
    assert(ran->fault_tree_analyses().size() == 1);
    std::vector<int> distr(
        ran->fault_tree_analyses().begin()->second->max_order() + 1, 0);
    const std::set< std::set<std::string> >& mcs =
        ran->fault_tree_analyses().begin()->second->min_cut_sets();
    for (const auto& cut_set : mcs) {
      int order = cut_set.size();
      distr[order]++;
    }
    return distr;
  }

  double p_total() {
    assert(!ran->probability_analyses().empty());
    assert(ran->probability_analyses().size() == 1);
    return ran->probability_analyses().begin()->second->p_total();
  }

  const std::map<std::set<std::string>, double>& mcs_probability() {
    assert(!ran->fault_tree_analyses().empty());
    assert(ran->fault_tree_analyses().size() == 1);
    return ran->fault_tree_analyses().begin()->second->mcs_probability();
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

  // Members
  RiskAnalysis* ran;
  Initializer* init;
  Settings settings;
};

}  // namespace test
}  // namespace scram

#endif  // SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_
