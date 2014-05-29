#include <gtest/gtest.h>

#include "error.h"
#include "risk_analysis.h"

using namespace scram;

// Test correct inputs
TEST(InputTest, CorrectInputs) {
  RiskAnalysis* ran = new FaultTree("fta-default", false);
  EXPECT_NO_THROW(ran->ProcessInput("./input/fta/correct_tree_input.scramf"));
}
