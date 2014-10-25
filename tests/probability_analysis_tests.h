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
    using boost::container::flat_set;
    flat_set<int> mcs(min_cut_set.begin(), min_cut_set.end());
    return prob_analysis->ProbAnd(mcs);
  }

  double ProbOr(std::set< std::set<int> >* min_cut_sets) {
    return ProbabilityAnalysisTest::ProbOr(1000, min_cut_sets);
  }

  double ProbOr(int nsums, std::set< std::set<int> >* min_cut_sets) {
    using boost::container::flat_set;
    std::set< flat_set<int> > mcs;
    std::set< std::set<int> >::iterator it;
    for (it = min_cut_sets->begin(); it != min_cut_sets->end(); ++it) {
      flat_set<int> fs(it->begin(), it->end());
      mcs.insert(mcs.end(), fs);
    }
    prob_analysis->pos_terms_.clear();
    prob_analysis->neg_terms_.clear();
    prob_analysis->ProbOr(1, nsums, &mcs);
    return prob_analysis->CalculateTotalProbability();
  }

  void CombineElAndSet(const std::set<int>& el,
                       const std::set< std::set<int> >& set,
                       std::set< std::set<int> >* combo_set) {
    prob_analysis->coherent_ = false;  // General case.
    using boost::container::flat_set;
    std::set< flat_set<int> > mcs;
    std::set< std::set<int> >::const_iterator it;
    for (it = set.begin(); it != set.end(); ++it) {
      flat_set<int> fs(it->begin(), it->end());
      mcs.insert(mcs.end(), fs);
    }
    std::set< flat_set<int> > f_comb;
    flat_set<int> f_el(el.begin(), el.end());
    prob_analysis->CombineElAndSet(f_el, mcs, &f_comb);
    std::set< flat_set<int> >::iterator fit;
    for (fit = f_comb.begin(); fit != f_comb.end(); ++fit) {
      std::set<int> st(fit->begin(), fit->end());
      combo_set->insert(st);
    }
  }

  void AddPrimaryIntProb(double prob) {
    prob_analysis->iprobs_.push_back(prob);
  }

  // Members
  ProbabilityAnalysis* prob_analysis;
};

#endif  // SCRAM_TESTS_PROBABILITY_ANALYSIS_TESTS_H_
