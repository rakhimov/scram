#ifndef SCRAM_TESTS_PERFORMANCE_TESTS_H_
#define SCRAM_TESTS_PERFORMANCE_TESTS_H_

#include <gtest/gtest.h>

#include "fault_tree_analysis.h"
#include "initializer.h"
#include "probability_analysis.h"
#include "risk_analysis.h"

using namespace scram;

class PerformanceTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
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
    assert(!ran->prob_analyses_.empty());
    return ran->prob_analyses_.begin()->second->p_total();
  }

  // The number of MCS as a result of analysis.
  int NumOfMcs() {
    assert(!ran->ftas_.empty());
    return ran->ftas_.begin()->second->min_cut_sets().size();
  }

  // Time taken to find minimal cut sets.
  double McsGenerationTime() {
    assert(!ran->ftas_.empty());
    return ran->ftas_.begin()->second->analysis_time_;
  }

  // Time taken to calculate total probability.
  double ProbCalcTime() {
    assert(!ran->prob_analyses_.empty());
    return ran->prob_analyses_.begin()->second->p_time_;
  }

  RiskAnalysis* ran;
  Settings settings;
  double delta;  // The range indicator for values.
};

#endif  // SCRAM_TESTS_PERFORMANCE_TESTS_H_
