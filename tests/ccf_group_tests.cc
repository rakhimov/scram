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

#include "ccf_group.h"

#include <gtest/gtest.h>

#include "error.h"
#include "expression/constant.h"

namespace scram {
namespace mef {
namespace test {

TEST(CcfGroupTest, AddMemberRepeated) {
  BetaFactorModel ccf_group("general");
  BasicEventPtr member(new BasicEvent("id"));
  ASSERT_NO_THROW(ccf_group.AddMember(member));
  EXPECT_THROW(ccf_group.AddMember(member), ValidationError);
}

TEST(CcfGroupTest, AddMemberAfterDistribution) {
  BetaFactorModel ccf_group("general");

  BasicEventPtr member(new BasicEvent("id"));
  ASSERT_NO_THROW(ccf_group.AddMember(member));

  ExpressionPtr distr(new ConstantExpression(1));
  ASSERT_NO_THROW(ccf_group.AddDistribution(distr));

  BasicEventPtr member_two(new BasicEvent("two"));
  EXPECT_THROW(ccf_group.AddMember(member), IllegalOperation);
}

}  // namespace test
}  // namespace mef
}  // namespace scram
