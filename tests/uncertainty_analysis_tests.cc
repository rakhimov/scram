#include "uncertainty_analysis.h"

#include <gtest/gtest.h>

#include "error.h"

using namespace scram;

TEST(UncertaintyAnalysisTest, Constructor) {
  ASSERT_NO_THROW(UncertaintyAnalysis(1));
  // Incorrect number of series in the probability equation.
  ASSERT_THROW(UncertaintyAnalysis(-1), InvalidArgument);
  // Invalid cut-off probability.
  ASSERT_NO_THROW(UncertaintyAnalysis(1, 1));
  ASSERT_THROW(UncertaintyAnalysis(1, -1), InvalidArgument);
  // Invalid number of trials.
  ASSERT_NO_THROW(UncertaintyAnalysis(1, 1, 100));
  ASSERT_THROW(UncertaintyAnalysis(1, 1, -1), InvalidArgument);
}
