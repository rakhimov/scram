/*
 * Copyright (C) 2014-2015, 2017 Olzhas Rakhimov
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

namespace scram {
namespace mef {
namespace test {

TEST(FaultTreeTest, AddGate) {
  FaultTree ft("never_fail");
  GatePtr gate(new Gate("Golden"));
  EXPECT_NO_THROW(ft.AddGate(gate));
  EXPECT_THROW(ft.AddGate(gate), ValidationError);  // Trying to re-add.

  GatePtr gate_two(new Gate("Iron"));
  EXPECT_NO_THROW(ft.AddGate(gate_two));  // No parent.
}

TEST(FaultTreeTest, AddBasicEvent) {
  FaultTree ft("never_fail");
  BasicEventPtr event(new BasicEvent("Golden"));
  EXPECT_NO_THROW(ft.AddBasicEvent(event));
  EXPECT_THROW(ft.AddBasicEvent(event), ValidationError);  // Trying to re-add.

  BasicEventPtr event_two(new BasicEvent("Iron"));
  EXPECT_NO_THROW(ft.AddBasicEvent(event_two));  // No parent.
}

TEST(FaultTreeTest, AddHouseEvent) {
  FaultTree ft("never_fail");
  HouseEventPtr event(new HouseEvent("Golden"));
  EXPECT_NO_THROW(ft.AddHouseEvent(event));
  EXPECT_THROW(ft.AddHouseEvent(event), ValidationError);  // Trying to re-add.

  HouseEventPtr event_two(new HouseEvent("Iron"));
  EXPECT_NO_THROW(ft.AddHouseEvent(event_two));  // No parent.
}

TEST(FaultTreeTest, AddCcfGroup) {
  FaultTree ft("never_fail");
  CcfGroupPtr group(new BetaFactorModel("Golden"));
  EXPECT_NO_THROW(ft.AddCcfGroup(group));
  EXPECT_THROW(ft.AddCcfGroup(group), ValidationError);  // Trying to re-add.

  CcfGroupPtr group_two(new BetaFactorModel("Iron"));
  EXPECT_NO_THROW(ft.AddCcfGroup(group_two));
}

TEST(FaultTreeTest, AddParameter) {
  FaultTree ft("never_fail");
  ParameterPtr parameter(new Parameter("Golden"));
  EXPECT_NO_THROW(ft.AddParameter(parameter));
  EXPECT_THROW(ft.AddParameter(parameter), ValidationError);

  ParameterPtr parameter_two(new Parameter("Iron"));
  EXPECT_NO_THROW(ft.AddParameter(parameter_two));
}

}  // namespace test
}  // namespace mef
}  // namespace scram
