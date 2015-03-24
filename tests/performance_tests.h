#ifndef SCRAM_TESTS_PERFORMANCE_TESTS_H_
#define SCRAM_TESTS_PERFORMANCE_TESTS_H_

#include <gtest/gtest.h>

#include "fault_tree_analysis.h"
#include "risk_analysis.h"

using namespace scram;

class PerformanceTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    delta = 0.10;  // % variation of values.
    ran = new RiskAnalysis();
  }

  virtual void TearDown() {
    delete ran;
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
  double delta;  // the range indicator for values.
};

#endif  // SCRAM_TESTS_PERFORMANCE_TESTS_H_
