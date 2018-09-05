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

#include <fstream>
#include <memory>
#include <sstream>

#include <boost/filesystem.hpp>

#include <catch2/catch.hpp>

namespace fs = boost::filesystem;

namespace scram::xml::test {

TEST_CASE("XmlStreamTest.ctor", "[xml_stream]") {
  CHECK_NOTHROW(Stream(stderr));
  CHECK_NOTHROW(Stream(stdout));
  CHECK_THROWS_AS(Stream(stderr).root(""), StreamError);
  CHECK_NOTHROW(Stream(stderr).root("element"));
  Stream xml_stream(stderr);
  CHECK_NOTHROW(xml_stream.root("element"));
  CHECK_THROWS_AS(xml_stream.root("root"), StreamError);
}

TEST_CASE("XmlStreamTest.Element", "[xml_stream]") {
  Stream xml_stream(stderr);
  StreamElement root = xml_stream.root("root");

  SECTION("Duplicate root") {
    CHECK_THROWS_AS(xml_stream.root("root"), StreamError);
  }

  SECTION("Set attribute") {
    CHECK_THROWS_AS(root.SetAttribute("", "value"), StreamError);
    CHECK_NOTHROW(root.SetAttribute("attr1", "value"));
    CHECK_NOTHROW(root.SetAttribute("attr2", ""));
    CHECK_NOTHROW(root.SetAttribute("attr3", 7));
  }

  SECTION("Add text") {
    CHECK_NOTHROW(root.AddText("text"));
    CHECK_NOTHROW(root.AddText(7));
  }

  SECTION("Add child") {
    CHECK_THROWS_AS(root.AddChild(""), StreamError);
    CHECK_NOTHROW(root.AddChild("child"));
  }

  SECTION("State after add attribute") {
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

  SECTION("State after add text") {
    CHECK_NOTHROW(root.AddText("text"));  // Locks on text.
    CHECK_THROWS_AS(root.SetAttribute("attr", "value"), StreamError);
    CHECK_THROWS_AS(root.AddChild("another_child"), StreamError);
    CHECK_NOTHROW(root.AddText(" and continuation..."));
  }

  SECTION("State after add child") {
    CHECK_NOTHROW(root.AddChild("child"));  // Locks on elements.
    CHECK_THROWS_AS(root.SetAttribute("attr", "value"), StreamError);
    CHECK_THROWS_AS(root.AddText("text"), StreamError);
    CHECK_NOTHROW(root.AddChild("another_child"));
  }

  SECTION("Inactive parent") {
    {
      StreamElement child = root.AddChild("child");  // Make root inactive.
      CHECK_THROWS_AS(root.SetAttribute("attr", "value"), StreamError);
      CHECK_THROWS_AS(root.AddText("text"), StreamError);
      CHECK_THROWS_AS(root.AddChild("another_child"), StreamError);
      // Child must be active without problems.
      CHECK_NOTHROW(child.SetAttribute("sub_attr", "value"));
      CHECK_NOTHROW(child.AddChild("sub_child"));
    }  // Lock is off at the scope exit.
    CHECK_NOTHROW(root.AddChild("another_child"));
  }
}

TEST_CASE("XmlStreamTest.Full", "[xml_stream]") {
  fs::path unique_name = "scram_xml_test-" + fs::unique_path().string();
  fs::path temp_file = fs::temp_directory_path() / unique_name;
  INFO("XML temp file: " + temp_file.string());
  const char content[] =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<root name=\"master\" age=\"42\" stamina=\"0.42\" empty=\"\">\n"
      "  <empty/>\n"
      "  <student new=\"true\" old=\"false\">\n"
      "    <label>newbie</label>\n"
      "  </student>\n"
      "  <student name=\"brut'\" motto=\"less &lt; more\">\n"
      "    <label>brut' less &lt; more</label>\n"
      "  </student>\n"
      "  <student name=\"brut&quot;\" motto=\"less > more\">\n"
      "    <label>brut&quot; less > more</label>\n"
      "  </student>\n"
      "  <student name=\"brut&amp;\" motto=\"less &amp; more\">\n"
      "    <label>brut&amp; less &amp; more</label>\n"
      "  </student>\n"
      "</root>\n";
  {
    std::unique_ptr<std::FILE, decltype(&std::fclose)> fp(
        std::fopen(temp_file.string().c_str(), "w"), &std::fclose);
    Stream xml_stream(fp.get());
    StreamElement root = xml_stream.root("root");
    root.SetAttribute("name", "master")
        .SetAttribute("age", 42)
        .SetAttribute("stamina", "0.42")
        .SetAttribute("empty", "");
    root.AddChild("empty");
    {
      StreamElement student = root.AddChild("student");
      student.SetAttribute("new", true).SetAttribute("old", false);
      student.AddChild("label").AddText("newbie");
    }
    auto add_student = [&root](const char* name, const char* motto) {
      StreamElement student = root.AddChild("student");
      student.SetAttribute("name", name).SetAttribute("motto", motto);
      student.AddChild("label").AddText(name).AddText(" ").AddText(motto);
    };
    add_student("brut'", "less < more");
    add_student("brut\"", "less > more");
    add_student("brut&", "less & more");
  }
  std::stringstream str_stream;
  str_stream << std::fstream(temp_file.string()).rdbuf();
  CHECK(str_stream.str() == content);
  fs::remove(temp_file);
}

}  // namespace scram::xml::test
