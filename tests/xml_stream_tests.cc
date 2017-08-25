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

/// This fixture provides document stream for tests.
class XmlStreamTest : public ::testing::Test {
 protected:
  XmlStreamTest() : xml_stream_(stderr) {}
  Stream xml_stream_;  ///< The stream to add elements into per test case.
};

TEST_F(XmlStreamTest, ElementConstructor) {
  EXPECT_THROW(xml_stream_.root(""), StreamError);
  EXPECT_NO_THROW(xml_stream_.root("element"));
}

TEST_F(XmlStreamTest, StreamConstructor) {
  EXPECT_NO_THROW(xml_stream_.root("root"));
  EXPECT_THROW(xml_stream_.root("root"), StreamError);
}

TEST_F(XmlStreamTest, SetAttribute) {
  StreamElement el = xml_stream_.root("element");
  EXPECT_THROW(el.SetAttribute("", "value"), StreamError);
  EXPECT_NO_THROW(el.SetAttribute("attr1", "value"));
  EXPECT_NO_THROW(el.SetAttribute("attr2", ""));
  EXPECT_NO_THROW(el.SetAttribute("attr3", 7));
}

TEST_F(XmlStreamTest, AddText) {
  StreamElement el = xml_stream_.root("element");
  EXPECT_NO_THROW(el.AddText("text"));
  EXPECT_NO_THROW(el.AddText(7));
}

TEST_F(XmlStreamTest, AddChild) {
  StreamElement el = xml_stream_.root("element");
  EXPECT_THROW(el.AddChild(""), StreamError);
  EXPECT_NO_THROW(el.AddChild("child"));
}

TEST_F(XmlStreamTest, StateAfterSetAttribute) {
  StreamElement root = xml_stream_.root("root");
  {
    StreamElement el = root.AddChild("element");
    EXPECT_NO_THROW(el.SetAttribute("attr", "value"));
    EXPECT_NO_THROW(el.AddText("text"));
  }
  {
    StreamElement el = root.AddChild("element");
    EXPECT_NO_THROW(el.SetAttribute("attr", "value"));
    EXPECT_NO_THROW(el.AddChild("child"));
  }
}

TEST_F(XmlStreamTest, StateAfterAddText) {
  StreamElement el = xml_stream_.root("element");
  EXPECT_NO_THROW(el.AddText("text"));  // Locks on text.
  EXPECT_THROW(el.SetAttribute("attr", "value"), StreamError);
  EXPECT_THROW(el.AddChild("another_child"), StreamError);
  EXPECT_NO_THROW(el.AddText(" and continuation..."));
}

TEST_F(XmlStreamTest, StateAfterAddChild) {
  StreamElement el = xml_stream_.root("element");
  EXPECT_NO_THROW(el.AddChild("child"));  // Locks on elements.
  EXPECT_THROW(el.SetAttribute("attr", "value"), StreamError);
  EXPECT_THROW(el.AddText("text"), StreamError);
  EXPECT_NO_THROW(el.AddChild("another_child"));
}

TEST_F(XmlStreamTest, InactiveParent) {
  StreamElement el = xml_stream_.root("element");
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
