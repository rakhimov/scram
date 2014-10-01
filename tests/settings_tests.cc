#include <gtest/gtest.h>

#include "settings.h"
#include "error.h"

using namespace scram;

TEST(SettingsTest, IncorrectSetup) {
  Settings s;
  // Incorrect analysis type.
  ASSERT_THROW(s.fta_type("analysis"), ValueError);
  // Incorrect approximation argument.
  ASSERT_THROW(s.approx("approx"), ValueError);
  // Incorrect limit order for minmal cut sets.
  ASSERT_THROW(s.limit_order(-1), ValueError);
  // Incorrect number of series in the probability equation.
  ASSERT_THROW(s.num_sums(-1), ValueError);
  // Incorrect cut-off probability.
  ASSERT_THROW(s.cut_off(-1), ValueError);
  ASSERT_THROW(s.cut_off(10), ValueError);
}
