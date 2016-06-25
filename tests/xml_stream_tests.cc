/*
 * Copyright (C) 2016 Olzhas Rakhimov
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

#include "xml_stream.h"

#include <gtest/gtest.h>

namespace scram {
namespace test {

TEST(XmlStreamTest, Constructor) {
  EXPECT_THROW(XmlStreamElement("", std::cerr), XmlStreamError);
  EXPECT_NO_THROW(XmlStreamElement("element", std::cerr));
}

TEST(XmlStreamTest, SetAttribute) {
  XmlStreamElement el("element", std::cerr);
  EXPECT_THROW(el.SetAttribute("", "value"), XmlStreamError);
  EXPECT_NO_THROW(el.SetAttribute("attr1", "value"));
  EXPECT_NO_THROW(el.SetAttribute("attr2", ""));
  EXPECT_NO_THROW(el.SetAttribute("attr3", 7));
}

TEST(XmlStreamTest, AddText) {
  XmlStreamElement el("element", std::cerr);
  EXPECT_NO_THROW(el.AddText("text"));
  EXPECT_NO_THROW(el.AddText(7));
}

TEST(XmlStreamTest, AddChild) {
  XmlStreamElement el("element", std::cerr);
  EXPECT_THROW(el.AddChild(""), XmlStreamError);
  EXPECT_NO_THROW(el.AddChild("child"));
}

TEST(XmlStreamTest, StateAfterSetAttribute) {
  {
    XmlStreamElement el("element", std::cerr);
    EXPECT_NO_THROW(el.SetAttribute("attr", "value"));
    EXPECT_NO_THROW(el.AddText("text"));
  }
  {
    XmlStreamElement el("element", std::cerr);
    EXPECT_NO_THROW(el.SetAttribute("attr", "value"));
    EXPECT_NO_THROW(el.AddChild("child"));
  }
}

TEST(XmlStreamTest, StateAfterAddText) {
  XmlStreamElement el("element", std::cerr);
  EXPECT_NO_THROW(el.AddText("text"));  // Locks on text.
  EXPECT_THROW(el.SetAttribute("attr", "value"), XmlStreamError);
  EXPECT_THROW(el.AddChild("another_child"), XmlStreamError);
  EXPECT_NO_THROW(el.AddText(" and continuation..."));
}

TEST(XmlStreamTest, StateAfterAddChild) {
  XmlStreamElement el("element", std::cerr);
  EXPECT_NO_THROW(el.AddChild("child"));  // Locks on elements.
  EXPECT_THROW(el.SetAttribute("attr", "value"), XmlStreamError);
  EXPECT_THROW(el.AddText("text"), XmlStreamError);
  EXPECT_NO_THROW(el.AddChild("another_child"));
}

TEST(XmlStreamTest, InactiveParent) {
  XmlStreamElement el("element", std::cerr);
  {
    XmlStreamElement child = el.AddChild("child");  // Make the parent inactive.
    EXPECT_THROW(el.SetAttribute("attr", "value"), XmlStreamError);
    EXPECT_THROW(el.AddText("text"), XmlStreamError);
    EXPECT_THROW(el.AddChild("another_child"), XmlStreamError);
    // Child must be active without problems.
    EXPECT_NO_THROW(child.SetAttribute("sub_attr", "value"));
    EXPECT_NO_THROW(child.AddChild("sub_child"));
  }  // Lock is off at the scope exit.
  EXPECT_NO_THROW(el.AddChild("another_child"));
}

}  // namespace test
}  // namespace scram
