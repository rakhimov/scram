#include "fault_tree_analysis.h"

#include <gtest/gtest.h>

#include "error.h"

using namespace scram;

// Invalid options for the constructor.
TEST(FaultTreeAnalysisTest, Constructor) {
  ASSERT_NO_THROW(FaultTreeAnalysis(1));
  // Incorrect limit order for minimal cut sets.
  ASSERT_THROW(FaultTreeAnalysis(-1), InvalidArgument);
}
