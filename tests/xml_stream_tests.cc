/*
 * Copyright (C) 2016-2018 Olzhas Rakhimov
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

#include <catch.hpp>

namespace scram::xml::test {

/// This fixture provides document stream for tests.
class XmlStreamTest {
 public:
  XmlStreamTest() : xml_stream_(stderr) {}
  ~XmlStreamTest() noexcept {}

 protected:
  Stream xml_stream_;  ///< The stream to add elements into per test case.
};

TEST_CASE_METHOD(XmlStreamTest, "xml::StreamElement ctor", "[xml_stream]") {
  CHECK_THROWS_AS(xml_stream_.root(""), StreamError);
  CHECK_NOTHROW(xml_stream_.root("element"));
}

TEST_CASE_METHOD(XmlStreamTest, "xml::Stream ctor", "[xml_stream]") {
  CHECK_NOTHROW(xml_stream_.root("root"));
  CHECK_THROWS_AS(xml_stream_.root("root"), StreamError);
}

TEST_CASE_METHOD(XmlStreamTest, "xml::StreamElement SetAttribute",
                 "[xml_stream]") {
  StreamElement el = xml_stream_.root("element");
  CHECK_THROWS_AS(el.SetAttribute("", "value"), StreamError);
  CHECK_NOTHROW(el.SetAttribute("attr1", "value"));
  CHECK_NOTHROW(el.SetAttribute("attr2", ""));
  CHECK_NOTHROW(el.SetAttribute("attr3", 7));
}

TEST_CASE_METHOD(XmlStreamTest, "xml::StreamElement AddText", "[xml_stream]") {
  StreamElement el = xml_stream_.root("element");
  CHECK_NOTHROW(el.AddText("text"));
  CHECK_NOTHROW(el.AddText(7));
}

TEST_CASE_METHOD(XmlStreamTest, "xml::StreamElement AddChild", "[xml_stream]") {
  StreamElement el = xml_stream_.root("element");
  CHECK_THROWS_AS(el.AddChild(""), StreamError);
  CHECK_NOTHROW(el.AddChild("child"));
}

TEST_CASE_METHOD(XmlStreamTest, "xml::StreamElement StateAfterSetAttribute",
                 "[xml_stream]") {
  StreamElement root = xml_stream_.root("root");
  {
    StreamElement el = root.AddChild("element");
    CHECK_NOTHROW(el.SetAttribute("attr", "value"));
    CHECK_NOTHROW(el.AddText("text"));
  }
  {
    StreamElement el = root.AddChild("element");
    CHECK_NOTHROW(el.SetAttribute("attr", "value"));
    CHECK_NOTHROW(el.AddChild("child"));
  }
}

TEST_CASE_METHOD(XmlStreamTest, "xml::StreamElement StateAfterAddText",
                 "[xml_stream]") {
  StreamElement el = xml_stream_.root("element");
  CHECK_NOTHROW(el.AddText("text"));  // Locks on text.
  CHECK_THROWS_AS(el.SetAttribute("attr", "value"), StreamError);
  CHECK_THROWS_AS(el.AddChild("another_child"), StreamError);
  CHECK_NOTHROW(el.AddText(" and continuation..."));
}

TEST_CASE_METHOD(XmlStreamTest, "xml::StreamElement StateAfterAddChild",
                 "[xml_stream]") {
  StreamElement el = xml_stream_.root("element");
  CHECK_NOTHROW(el.AddChild("child"));  // Locks on elements.
  CHECK_THROWS_AS(el.SetAttribute("attr", "value"), StreamError);
  CHECK_THROWS_AS(el.AddText("text"), StreamError);
  CHECK_NOTHROW(el.AddChild("another_child"));
}

TEST_CASE_METHOD(XmlStreamTest, "xml::StreamElement InactiveParent",
                 "[xml_stream]") {
  StreamElement el = xml_stream_.root("element");
  {
    StreamElement child = el.AddChild("child");  // Make the parent inactive.
    CHECK_THROWS_AS(el.SetAttribute("attr", "value"), StreamError);
    CHECK_THROWS_AS(el.AddText("text"), StreamError);
    CHECK_THROWS_AS(el.AddChild("another_child"), StreamError);
    // Child must be active without problems.
    CHECK_NOTHROW(child.SetAttribute("sub_attr", "value"));
    CHECK_NOTHROW(child.AddChild("sub_child"));
  }  // Lock is off at the scope exit.
  CHECK_NOTHROW(el.AddChild("another_child"));
}

}  // namespace scram::xml::test
