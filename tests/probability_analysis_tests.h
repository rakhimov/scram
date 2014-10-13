#ifndef SCRAM_TESTS_PROBABILITY_ANALYSIS_TESTS_H_
#define SCRAM_TESTS_PROBABILITY_ANALYSIS_TESTS_H_

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "probability_analysis.h"

using namespace scram;

typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

class ProbabilityAnalysisTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    prob_analysis = new ProbabilityAnalysis();
  }

  virtual void TearDown() {
    delete prob_analysis;
  }

  double ProbAnd(const std::set<int>& min_cut_set) {
    return prob_analysis->ProbAnd(min_cut_set);
  }

  double ProbOr(std::set< std::set<int> >* min_cut_sets) {
    return prob_analysis->ProbOr(1000, min_cut_sets);
  }

  double ProbOr(int nsums, std::set< std::set<int> >* min_cut_sets) {
    return prob_analysis->ProbOr(nsums, min_cut_sets);
  }

  void CombineElAndSet(const std::set<int>& el,
                       const std::set< std::set<int> >& set,
                       std::set< std::set<int> >* combo_set) {
    return prob_analysis->CombineElAndSet(el, set, combo_set);
  }

  void AddPrimaryIntProb(double prob) {
    prob_analysis->iprobs_.push_back(prob);
  }

  // Members
  ProbabilityAnalysis* prob_analysis;
};

#endif  // SCRAM_TESTS_PROBABILITY_ANALYSIS_TESTS_H_
