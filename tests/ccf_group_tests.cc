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

#include "ccf_group.h"

#include <catch.hpp>

#include "error.h"
#include "expression/constant.h"

namespace scram::mef::test {

TEST_CASE("CcfGroupTest.AddMemberRepeated", "[mef::ccf_group]") {
  BetaFactorModel ccf_group("general");
  BasicEvent member("id");
  REQUIRE_NOTHROW(ccf_group.AddMember(&member));
  CHECK_THROWS_AS(ccf_group.AddMember(&member), ValidityError);
}

TEST_CASE("CcfGroupTest.AddMemberAfterDistribution", "[mef::ccf_group]") {
  BetaFactorModel ccf_group("general");

  BasicEvent member_one("id");
  REQUIRE_NOTHROW(ccf_group.AddMember(&member_one));

  BasicEvent member_two("two");
  CHECK_NOTHROW(ccf_group.AddMember(&member_two));

  REQUIRE_NOTHROW(ccf_group.AddDistribution(&ConstantExpression::kOne));

  BasicEvent member_three("three");
  CHECK_THROWS_AS(ccf_group.AddMember(&member_three), LogicError);
}

}  // namespace scram::mef::test
