#include "settings.h"

#include <gtest/gtest.h>

#include "error.h"

using namespace scram;

TEST(SettingsTest, IncorrectSetup) {
  Settings s;
  // Incorrect approximation argument.
  ASSERT_THROW(s.approx("approx"), InvalidArgument);
  // Incorrect limit order for minimal cut sets.
  ASSERT_THROW(s.limit_order(-1), InvalidArgument);
  // Incorrect number of series in the probability equation.
  ASSERT_THROW(s.num_sums(-1), InvalidArgument);
  ASSERT_THROW(s.num_sums(0), InvalidArgument);
  // Incorrect cut-off probability.
  ASSERT_THROW(s.cut_off(-1), InvalidArgument);
  ASSERT_THROW(s.cut_off(10), InvalidArgument);
  // Incorrect number of trials.
  ASSERT_THROW(s.num_trials(-10), InvalidArgument);
  ASSERT_THROW(s.num_trials(0), InvalidArgument);
  // Incorrect seed.
  ASSERT_THROW(s.seed(-1), InvalidArgument);
  // Incorrect mission time.
  ASSERT_THROW(s.mission_time(-10), InvalidArgument);
}
