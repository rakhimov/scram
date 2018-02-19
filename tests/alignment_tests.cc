/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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

#include <catch.hpp>

#include "error.h"

namespace scram::mef::test {

TEST_CASE("PhaseTest.TimeFraction", "[mef::alignment]") {
  CHECK_NOTHROW(Phase("phase", 0.5));
  CHECK_NOTHROW(Phase("phase", 0.1));
  CHECK_NOTHROW(Phase("phase", 1));

  CHECK_THROWS_AS(Phase("phase", 0), DomainError);
  CHECK_THROWS_AS(Phase("phase", 1.1), DomainError);
  CHECK_THROWS_AS(Phase("phase", -0.1), DomainError);
}

TEST_CASE("AlignmentTest.AddPhase", "[mef::alignment]") {
  Alignment alignment("mission");
  auto phase_one = std::make_unique<Phase>("one", 0.5);
  auto phase_two = std::make_unique<Phase>("one", 0.1);  // Duplicate name.
  auto phase_three = std::make_unique<Phase>("three", 0.1);

  CHECK(alignment.phases().empty());
  auto* phase_one_address = phase_one.get();
  REQUIRE_NOTHROW(alignment.Add(std::move(phase_one)));
  CHECK(alignment.phases().size() == 1);
  CHECK(&*alignment.phases().begin() == phase_one_address);

  CHECK_THROWS_AS(alignment.Add(std::move(phase_two)), DuplicateElementError);
  CHECK(alignment.phases().size() == 1);
  CHECK(&*alignment.phases().begin() == phase_one_address);

  REQUIRE_NOTHROW(alignment.Add(std::move(phase_three)));
  CHECK(alignment.phases().size() == 2);
}

TEST_CASE("AlignmentTest.Validation", "[mef::alignment]") {
  Alignment alignment("mission");
  auto phase_one = std::make_unique<Phase>("one", 0.5);
  auto phase_two = std::make_unique<Phase>("two", 0.5);
  auto phase_three = std::make_unique<Phase>("three", 0.1);

  CHECK_THROWS_AS(alignment.Validate(), ValidityError);

  REQUIRE_NOTHROW(alignment.Add(std::move(phase_one)));
  CHECK_THROWS_AS(alignment.Validate(), ValidityError);

  REQUIRE_NOTHROW(alignment.Add(std::move(phase_two)));
  CHECK_NOTHROW(alignment.Validate());

  REQUIRE_NOTHROW(alignment.Add(std::move(phase_three)));
  CHECK_THROWS_AS(alignment.Validate(), ValidityError);
}

}  // namespace scram::mef::test
