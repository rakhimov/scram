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
    assert(!ran->ftas_.empty());
    return ran->ftas_[0]->p_total();
  }

  // The number of MCS as a result of analysis.
  int NumOfMcs() {
    assert(!ran->ftas_.empty());
    return ran->ftas_[0]->min_cut_sets().size();
  }

  // Returns the time that took to preprocess and expand the fault tree.
  double TreeExpansionTime() {
    assert(!ran->ftas_.empty());
    return ran->ftas_[0]->exp_time_;
  }

  // Time taken to find minimal cut sets.
  double McsGenerationTime() {
    assert(!ran->ftas_.empty());
    return ran->ftas_[0]->mcs_time_ - ran->ftas_[0]->exp_time_;
  }

  // Time taken to calculate total probability.
  double ProbCalcTime() {
    assert(!ran->ftas_.empty());
    return ran->ftas_[0]->p_time_ - ran->ftas_[0]->mcs_time_;
  }

  RiskAnalysis* ran;
  Settings settings;
  double delta;  // the range indicator for values.
};

#endif  // SCRAM_TESTS_PERFORMANCE_TESTS_H_
