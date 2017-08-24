/*
 * Copyright (C) 2016-2017 Olzhas Rakhimov
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
namespace xml {
namespace test {

TEST(XmlStreamTest, Constructor) {
  EXPECT_THROW(StreamElement("", std::cerr), StreamError);
  EXPECT_NO_THROW(StreamElement("element", std::cerr));
}

TEST(XmlStreamTest, SetAttribute) {
  StreamElement el("element", std::cerr);
  EXPECT_THROW(el.SetAttribute("", "value"), StreamError);
  EXPECT_NO_THROW(el.SetAttribute("attr1", "value"));
  EXPECT_NO_THROW(el.SetAttribute("attr2", ""));
  EXPECT_NO_THROW(el.SetAttribute("attr3", 7));
}

TEST(XmlStreamTest, AddText) {
  StreamElement el("element", std::cerr);
  EXPECT_NO_THROW(el.AddText("text"));
  EXPECT_NO_THROW(el.AddText(7));
}

TEST(XmlStreamTest, AddChild) {
  StreamElement el("element", std::cerr);
  EXPECT_THROW(el.AddChild(""), StreamError);
  EXPECT_NO_THROW(el.AddChild("child"));
}

TEST(XmlStreamTest, StateAfterSetAttribute) {
  {
    StreamElement el("element", std::cerr);
    EXPECT_NO_THROW(el.SetAttribute("attr", "value"));
    EXPECT_NO_THROW(el.AddText("text"));
  }
  {
    StreamElement el("element", std::cerr);
    EXPECT_NO_THROW(el.SetAttribute("attr", "value"));
    EXPECT_NO_THROW(el.AddChild("child"));
  }
}

TEST(XmlStreamTest, StateAfterAddText) {
  StreamElement el("element", std::cerr);
  EXPECT_NO_THROW(el.AddText("text"));  // Locks on text.
  EXPECT_THROW(el.SetAttribute("attr", "value"), StreamError);
  EXPECT_THROW(el.AddChild("another_child"), StreamError);
  EXPECT_NO_THROW(el.AddText(" and continuation..."));
}

TEST(XmlStreamTest, StateAfterAddChild) {
  StreamElement el("element", std::cerr);
  EXPECT_NO_THROW(el.AddChild("child"));  // Locks on elements.
  EXPECT_THROW(el.SetAttribute("attr", "value"), StreamError);
  EXPECT_THROW(el.AddText("text"), StreamError);
  EXPECT_NO_THROW(el.AddChild("another_child"));
}

TEST(XmlStreamTest, InactiveParent) {
  StreamElement el("element", std::cerr);
  {
    StreamElement child = el.AddChild("child");  // Make the parent inactive.
    EXPECT_THROW(el.SetAttribute("attr", "value"), StreamError);
    EXPECT_THROW(el.AddText("text"), StreamError);
    EXPECT_THROW(el.AddChild("another_child"), StreamError);
    // Child must be active without problems.
    EXPECT_NO_THROW(child.SetAttribute("sub_attr", "value"));
    EXPECT_NO_THROW(child.AddChild("sub_child"));
  }  // Lock is off at the scope exit.
  EXPECT_NO_THROW(el.AddChild("another_child"));
}

}  // namespace test
}  // namespace xml
}  // namespace scram
