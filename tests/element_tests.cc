/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

#include "element.h"

#include <gtest/gtest.h>

#include "error.h"

namespace scram {
namespace mef {
namespace test {

class TestElement : public Element {};

TEST(ElementTest, Label) {
  TestElement el;
  EXPECT_EQ("", el.label());
  EXPECT_THROW(el.label(""), LogicError);
  ASSERT_NO_THROW(el.label("label"));
  EXPECT_THROW(el.label("new_label"), LogicError);
}

TEST(ElementTest, Attribute) {
  TestElement el;
  Attribute attr;
  attr.name = "impact";
  attr.value = "0.1";
  attr.type = "float";
  EXPECT_THROW(el.GetAttribute(attr.name), LogicError);
  ASSERT_NO_THROW(el.AddAttribute(attr));
  EXPECT_THROW(el.AddAttribute(attr), LogicError);
  ASSERT_TRUE(el.HasAttribute(attr.name));
  ASSERT_NO_THROW(el.GetAttribute(attr.name));
}

}  // namespace test
}  // namespace mef
}  // namespace scram
