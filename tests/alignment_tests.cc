/*
 * Copyright (C) 2017 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "alignment.h"

#include <gtest/gtest.h>

#include "error.h"

namespace scram {
namespace mef {
namespace test {

TEST(PhaseTest, TimeFraction) {
  EXPECT_NO_THROW(Phase("phase", 0.5));
  EXPECT_NO_THROW(Phase("phase", 0.1));
  EXPECT_NO_THROW(Phase("phase", 1));

  EXPECT_THROW(Phase("phase", 0), InvalidArgument);
  EXPECT_THROW(Phase("phase", 1.1), InvalidArgument);
  EXPECT_THROW(Phase("phase", -0.1), InvalidArgument);
}

TEST(AlignmentTest, AddPhase) {
  Alignment alignment("mission");
  auto phase_one = std::make_unique<Phase>("one", 0.5);
  auto phase_two = std::make_unique<Phase>("one", 0.1);  // Duplicate name.
  auto phase_three = std::make_unique<Phase>("three", 0.1);

  EXPECT_TRUE(alignment.phases().empty());
  auto* phase_one_address = phase_one.get();
  ASSERT_NO_THROW(alignment.Add(std::move(phase_one)));
  EXPECT_EQ(1, alignment.phases().size());
  EXPECT_EQ(phase_one_address, alignment.phases().begin()->get());

  EXPECT_THROW(alignment.Add(std::move(phase_two)), DuplicateArgumentError);
  EXPECT_EQ(1, alignment.phases().size());
  EXPECT_EQ(phase_one_address, alignment.phases().begin()->get());

  ASSERT_NO_THROW(alignment.Add(std::move(phase_three)));
  EXPECT_EQ(2, alignment.phases().size());
}

TEST(AlignmentTest, Validation) {
  Alignment alignment("mission");
  auto phase_one = std::make_unique<Phase>("one", 0.5);
  auto phase_two = std::make_unique<Phase>("two", 0.5);
  auto phase_three = std::make_unique<Phase>("three", 0.1);

  EXPECT_THROW(alignment.Validate(), ValidationError);

  ASSERT_NO_THROW(alignment.Add(std::move(phase_one)));
  EXPECT_THROW(alignment.Validate(), ValidationError);

  ASSERT_NO_THROW(alignment.Add(std::move(phase_two)));
  EXPECT_NO_THROW(alignment.Validate());

  ASSERT_NO_THROW(alignment.Add(std::move(phase_three)));
  EXPECT_THROW(alignment.Validate(), ValidationError);
}

}  // namespace test
}  // namespace mef
}  // namespace scram
