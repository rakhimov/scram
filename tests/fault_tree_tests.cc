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

#include <gtest/gtest.h>

#include "error.h"

namespace scram::mef::test {

TEST(FaultTreeTest, AddGate) {
  FaultTree ft("never_fail");
  Gate gate("Golden");
  EXPECT_NO_THROW(ft.Add(&gate));
  EXPECT_THROW(ft.Add(&gate), ValidityError);  // Trying to re-add.

  Gate gate_two("Iron");
  EXPECT_NO_THROW(ft.Add(&gate_two));  // No parent.
}

TEST(FaultTreeTest, AddBasicEvent) {
  FaultTree ft("never_fail");
  BasicEvent event("Golden");
  EXPECT_NO_THROW(ft.Add(&event));
  EXPECT_THROW(ft.Add(&event), ValidityError);  // Trying to re-add.

  BasicEvent event_two("Iron");
  EXPECT_NO_THROW(ft.Add(&event_two));  // No parent.
}

TEST(FaultTreeTest, AddHouseEvent) {
  FaultTree ft("never_fail");
  HouseEvent event("Golden");
  EXPECT_NO_THROW(ft.Add(&event));
  EXPECT_THROW(ft.Add(&event), ValidityError);  // Trying to re-add.

  HouseEvent event_two("Iron");
  EXPECT_NO_THROW(ft.Add(&event_two));  // No parent.
}

TEST(FaultTreeTest, AddCcfGroup) {
  FaultTree ft("never_fail");
  BetaFactorModel group("Golden");
  EXPECT_NO_THROW(ft.Add(&group));
  EXPECT_THROW(ft.Add(&group), ValidityError);  // Trying to re-add.

  BetaFactorModel group_two("Iron");
  EXPECT_NO_THROW(ft.Add(&group_two));
}

TEST(FaultTreeTest, AddParameter) {
  FaultTree ft("never_fail");
  Parameter parameter("Golden");
  EXPECT_NO_THROW(ft.Add(&parameter));
  EXPECT_THROW(ft.Add(&parameter), ValidityError);

  Parameter parameter_two("Iron");
  EXPECT_NO_THROW(ft.Add(&parameter_two));
}

}  // namespace scram::mef::test
