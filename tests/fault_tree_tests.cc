/*
 * Copyright (C) 2014-2015, 2017-2018 Olzhas Rakhimov
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

#include "fault_tree.h"

#include <catch.hpp>

#include "error.h"

namespace scram::mef::test {

TEST_CASE("FaultTreeTest.AddGate", "[mef::fault_tree]") {
  FaultTree ft("never_fail");
  Gate gate("Golden");
  CHECK_NOTHROW(ft.Add(&gate));
  CHECK_THROWS_AS(ft.Add(&gate), ValidityError);  // Trying to re-add.

  Gate gate_two("Iron");
  CHECK_NOTHROW(ft.Add(&gate_two));  // No parent.
}

TEST_CASE("FaultTreeTest.AddBasicEvent", "[mef::fault_tree]") {
  FaultTree ft("never_fail");
  BasicEvent event("Golden");
  CHECK_NOTHROW(ft.Add(&event));
  CHECK_THROWS_AS(ft.Add(&event), ValidityError);  // Trying to re-add.

  BasicEvent event_two("Iron");
  CHECK_NOTHROW(ft.Add(&event_two));  // No parent.
}

TEST_CASE("FaultTreeTest.AddHouseEvent", "[mef::fault_tree]") {
  FaultTree ft("never_fail");
  HouseEvent event("Golden");
  CHECK_NOTHROW(ft.Add(&event));
  CHECK_THROWS_AS(ft.Add(&event), ValidityError);  // Trying to re-add.

  HouseEvent event_two("Iron");
  CHECK_NOTHROW(ft.Add(&event_two));  // No parent.
}

TEST_CASE("FaultTreeTest.AddCcfGroup", "[mef::fault_tree]") {
  FaultTree ft("never_fail");
  BetaFactorModel group("Golden");
  CHECK_NOTHROW(ft.Add(&group));
  CHECK_THROWS_AS(ft.Add(&group), DuplicateElementError);  // Trying to re-add.

  BetaFactorModel group_two("Iron");
  CHECK_NOTHROW(ft.Add(&group_two));
}

TEST_CASE("FaultTreeTest.AddParameter", "[mef::fault_tree]") {
  FaultTree ft("never_fail");
  Parameter parameter("Golden");
  CHECK_NOTHROW(ft.Add(&parameter));
  CHECK_THROWS_AS(ft.Add(&parameter), ValidityError);

  Parameter parameter_two("Iron");
  CHECK_NOTHROW(ft.Add(&parameter_two));
}

}  // namespace scram::mef::test
