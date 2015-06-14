#include "fault_tree_analysis.h"

#include <gtest/gtest.h>

#include "error.h"

using namespace scram;

// Invalid options for the constructor.
TEST(FaultTreeAnalysisTest, Constructor) {
  typedef boost::shared_ptr<Gate> GatePtr;
  // Incorrect limit order for minimal cut sets.
  ASSERT_THROW(FaultTreeAnalysis(GatePtr(new Gate("dummy")), -1),
               InvalidArgument);
}
