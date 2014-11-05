#include <gtest/gtest.h>

#include "error.h"
#include "fault_tree_analysis.h"

using namespace scram;

// Invalid options for the constructor.
TEST(FaultTreeAnalysisTest, Constructor) {
  ASSERT_NO_THROW(FaultTreeAnalysis(1));
  // Incorrect limit order for minmal cut sets.
  ASSERT_THROW(FaultTreeAnalysis(-1), InvalidArgument);
}
