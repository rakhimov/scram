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

  std::vector< boost::container::flat_set<int> >& pos_terms() {
    return ua->pos_terms_;
  }

  std::vector< boost::container::flat_set<int> >& neg_terms() {
    return ua->neg_terms_;
  }

  // Members
  UncertaintyAnalysis* ua;
};

#endif  // SCRAM_TESTS_UNCERTAINTY_ANALYSIS_TESTS_H_
