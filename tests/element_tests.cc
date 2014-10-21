#include "element.h"

#include <gtest/gtest.h>

#include "error.h"

using namespace scram;

TEST(ElementTest, Label) {
  Element el = Element();
  EXPECT_EQ("", el.label());
  EXPECT_THROW(el.label(""), LogicError);
  ASSERT_NO_THROW(el.label("label"));
  EXPECT_THROW(el.label("new_label"), LogicError);
}

TEST(ElementTest, Attribute) {
  Element el = Element();
  Attribute attr = Attribute();
  attr.name = "impact";
  attr.value = "0.1";
  attr.type = "float";
  EXPECT_THROW(el.GetAttribute(attr.name), LogicError);
  ASSERT_NO_THROW(el.AddAttribute(attr));
  EXPECT_THROW(el.AddAttribute(attr), LogicError);
  ASSERT_TRUE(el.HasAttribute(attr.name));
  ASSERT_NO_THROW(el.GetAttribute(attr.name));
}
