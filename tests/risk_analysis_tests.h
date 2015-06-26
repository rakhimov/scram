#ifndef SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_
#define SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_

#include "initializer.h"
#include "risk_analysis.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace scram;

typedef boost::shared_ptr<Event> EventPtr;
typedef boost::shared_ptr<Gate> GatePtr;
typedef boost::shared_ptr<HouseEvent> HouseEventPtr;
typedef boost::shared_ptr<BasicEvent> BasicEventPtr;

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

  const boost::unordered_map<std::string, GatePtr>& gates() {
    return init->model()->gates();
  }

  const boost::unordered_map<std::string, HouseEventPtr>& house_events() {
    return init->model()->house_events();
  }

  const boost::unordered_map<std::string, BasicEventPtr>& basic_events() {
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
    std::set< std::set<std::string> >::const_iterator it;
    const std::set< std::set<std::string> >* mcs =
        &ran->fault_tree_analyses().begin()->second->min_cut_sets();
    for (it = mcs->begin(); it != mcs->end(); ++it) {
      int order = it->size();
      distr[order]++;
    }
    return distr;
  }

  double p_total() {
    assert(!ran->probability_analyses().empty());
    assert(ran->probability_analyses().size() == 1);
    return ran->probability_analyses().begin()->second->p_total();
  }

  const std::map< std::set<std::string>, double >& prob_of_min_sets() {
    assert(!ran->probability_analyses().empty());
    assert(ran->probability_analyses().size() == 1);
    return ran->probability_analyses().begin()->second->prob_of_min_sets();
  }

  const std::vector<double>& importance(std::string id) {
    assert(!ran->probability_analyses().empty());
    assert(ran->probability_analyses().size() == 1);
    return ran->probability_analyses().begin()->second
        ->importance().find(id)->second;
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

#endif  // SCRAM_TESTS_RISK_ANALYSIS_TESTS_H_
