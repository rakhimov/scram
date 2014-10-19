#ifndef SCRAM_TESTS_UNCERTAINTY_ANALYSIS_TESTS_H_
#define SCRAM_TESTS_UNCERTAINTY_ANALYSIS_TESTS_H_

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uncertainty_analysis.h"

using namespace scram;

typedef boost::shared_ptr<scram::PrimaryEvent> PrimaryEventPtr;

class UncertaintyAnalysisTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    ua = new UncertaintyAnalysis();
  }

  virtual void TearDown() {
    delete ua;
  }

  void CombineElAndSet(const std::set<int>& el,
                       const std::set< std::set<int> >& set,
                       std::set< std::set<int> >* combo_set) {
    return ua->CombineElAndSet(el, set, combo_set);
  }

  void MProbOr(std::set< std::set<int> >* min_cut_sets) {
    return ua->ProbOr(1, 1000, min_cut_sets);
  }

  void MProbOr(int sign, int nsums, std::set< std::set<int> >* min_cut_sets) {
    return ua->ProbOr(sign, nsums, min_cut_sets);
  }

  std::vector< std::set<int> >& pos_terms() {
    return ua->pos_terms_;
  }

  std::vector< std::set<int> >& neg_terms() {
    return ua->neg_terms_;
  }

  // Members
  UncertaintyAnalysis* ua;
};

#endif  // SCRAM_TESTS_UNCERTAINTY_ANALYSIS_TESTS_H_
