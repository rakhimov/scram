/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

namespace {

class TestElement : public Element {
 public:
  TestElement() : Element("", /*optional=*/true) {}
};

class NamedElement : public Element {
 public:
  explicit NamedElement(std::string name) : Element(std::move(name)) {}
};

}  // namespace

TEST(ElementTest, Name) {
  EXPECT_NO_THROW(TestElement());
  EXPECT_THROW(NamedElement(""), LogicError);

  EXPECT_THROW(NamedElement(".name"), InvalidArgument);
  EXPECT_THROW(NamedElement("na.me"), InvalidArgument);
  EXPECT_THROW(NamedElement("name."), InvalidArgument);

  EXPECT_NO_THROW(NamedElement("name"));
  NamedElement el("name");
  EXPECT_EQ("name", el.name());

  // Illegal names by MEF.
  // However, this names don't mess with class and reference invariants.
  EXPECT_NO_THROW(NamedElement("na me"));
  EXPECT_NO_THROW(NamedElement("na\nme"));
  EXPECT_NO_THROW(NamedElement("\tname"));
  EXPECT_NO_THROW(NamedElement("name?"));
}

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
  EXPECT_THROW(el.AddAttribute(attr), DuplicateArgumentError);
  ASSERT_TRUE(el.HasAttribute(attr.name));
  ASSERT_NO_THROW(el.GetAttribute(attr.name));
}

namespace {

class TestRole : public Role {
 public:
  using Role::Role;
};

}  // namespace

TEST(ElementTest, Role) {
  EXPECT_THROW(TestRole(RoleSpecifier::kPublic, ".ref"), InvalidArgument);
  EXPECT_THROW(TestRole(RoleSpecifier::kPublic, "ref."), InvalidArgument);
  EXPECT_NO_THROW(TestRole(RoleSpecifier::kPublic, "ref.name"));
}

namespace {

class NameId : public Element, public Role, public Id {
 public:
  NameId()
      : Element("", true),
        Role(RoleSpecifier::kPublic, "path"),
        Id(*this, *this) {}
  explicit NameId(std::string name, RoleSpecifier role = RoleSpecifier::kPublic,
                  std::string path = "")
      : Element(name), Role(role, path), Id(*this, *this) {}
};

}  // namespace

TEST(ElementTest, Id) {
  EXPECT_THROW(NameId(), LogicError);
  EXPECT_NO_THROW(NameId("name"));
  EXPECT_THROW(NameId("name", RoleSpecifier::kPrivate, ""), LogicError);
  NameId id_public("name");
  NameId id_private("name", RoleSpecifier::kPrivate, "path");
  EXPECT_NE(id_public.id(), id_private.id());
}

}  // namespace test
}  // namespace mef
}  // namespace scram
