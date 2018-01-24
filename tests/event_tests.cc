/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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

#include "event.h"

#include <catch.hpp>

#include "cycle.h"
#include "error.h"
#include "expression/constant.h"

namespace scram::mef::test {

// Test for Event base class.
TEST_CASE("EventTest.Id", "[mef::event]") {
  BasicEvent event("event_name");
  CHECK("event_name" == event.id());
}

TEST_CASE("BasicEventTest.ExpressionReset", "[mef::event]") {
  BasicEvent event("event");
  CHECK_FALSE(event.HasExpression());
  ConstantExpression p_init(0.5);
  event.expression(&p_init);
  REQUIRE(event.HasExpression());
  CHECK(event.p() == 0.5);

  ConstantExpression p_change(0.1);
  event.expression(&p_change);
  REQUIRE(event.HasExpression());
  CHECK(event.p() == 0.1);
}

TEST_CASE("BasicEventTest.Validate", "[mef::event]") {
  BasicEvent event("event");
  CHECK_FALSE(event.HasExpression());
  ConstantExpression p_valid(0.5);
  event.expression(&p_valid);
  REQUIRE(event.HasExpression());
  CHECK_NOTHROW(event.Validate());

  ConstantExpression p_negative(-0.1);
  event.expression(&p_negative);
  REQUIRE(event.HasExpression());
  CHECK_THROWS_AS(event.Validate(), ValidityError);

  ConstantExpression p_large(1.1);
  event.expression(&p_large);
  REQUIRE(event.HasExpression());
  CHECK_THROWS_AS(event.Validate(), ValidityError);
}

TEST_CASE("FormulaTest.MinNumber", "[mef::event]") {
  SECTION("Invalid connective") {
    Formula top(kAnd);
    CHECK(top.connective() == kAnd);
    CHECK_THROWS_AS(top.min_number(2), LogicError);
  }

  SECTION("Atleast") {
    Formula top(kAtleast);
    CHECK(top.connective() == kAtleast);
    // No min number.
    CHECK_THROWS_AS(top.min_number(), LogicError);
    // Illegal min number.
    CHECK_THROWS_AS(top.min_number(-2), ValidityError);
    // Legal min number.
    CHECK_NOTHROW(top.min_number(2));
    // Trying to reset the min number.
    CHECK_THROWS_AS(top.min_number(2), LogicError);
    // Requesting the min number should succeed.
    REQUIRE_NOTHROW(top.min_number());
    CHECK(top.min_number() == 2);
  }
}

TEST_CASE("FormulaTest.EventArguments", "[mef::event]") {
  Formula top(kAnd);
  BasicEvent first_child("first");
  BasicEvent second_child("second");
  CHECK(top.args().size() == 0);
  // Adding first child.
  CHECK_NOTHROW(top.Add(&first_child));
  // Re-adding a child must cause an error.
  CHECK_THROWS_AS(top.Add(&first_child), ValidityError);
  // Check the contents of the children container.
  CHECK(std::get<BasicEvent*>(top.args().front().event) == &first_child);
  // Adding another child.
  CHECK_NOTHROW(top.Add(&second_child));
  CHECK(top.args().size() == 2);
  CHECK(std::get<BasicEvent*>(top.args().back().event) == &second_child);

  CHECK_NOTHROW(top.Remove(&first_child));
  CHECK(top.args().size() == 1);
  CHECK_THROWS_AS(top.Remove(&first_child), LogicError);
}

TEST_CASE("FormulaTest.InvalidComplementArguments", "[mef::event]") {
  BasicEvent arg_event("first");
  SECTION("NULL connective with complement") {
    Formula top(kNull);
    REQUIRE_THROWS_AS(top.Add(&arg_event, true), LogicError);
    CHECK(top.args().empty());
  }
  SECTION("NOT connective with complement") {
    Formula top(kNot);
    REQUIRE_THROWS_AS(top.Add(&arg_event, true), LogicError);
    CHECK(top.args().empty());
  }
}

TEST_CASE("FormulaTest.DuplicateViaComplement", "[mef::event]") {
  Formula top(kAnd);
  BasicEvent arg_event("first");
  SECTION("Complement first") {
    REQUIRE_NOTHROW(top.Add(&arg_event, true));
    CHECK(top.args().size() == 1);
    REQUIRE_THROWS_AS(top.Add(&arg_event), DuplicateArgumentError);
  }
  SECTION("Complement second") {
    REQUIRE_NOTHROW(top.Add(&arg_event));
    CHECK(top.args().size() == 1);
    REQUIRE_THROWS_AS(top.Add(&arg_event, true), DuplicateArgumentError);
  }
}

TEST_CASE("FormulaTest.InvalidConstantArguments", "[mef::event]") {
  Formula top(kNot);
  SECTION("Constant True event") {
    REQUIRE_THROWS_AS(top.Add(&HouseEvent::kTrue), LogicError);
  }
  SECTION("Constant False event") {
    REQUIRE_THROWS_AS(top.Add(&HouseEvent::kFalse), LogicError);
  }
}

TEST_CASE("MEFGateTest.Cycle", "[mef::event]") {
  Gate root("root");  // Should not appear in the cycle.
  Gate top("Top");
  Gate middle("Middle");
  Gate bottom("Bottom");

  FormulaPtr formula_root(new Formula(kNot));
  formula_root->Add(&top);
  root.formula(std::move(formula_root));

  FormulaPtr formula_one(new Formula(kNot));
  formula_one->Add(&middle);
  FormulaPtr formula_two(new Formula(kNot));
  formula_two->Add(&bottom);
  FormulaPtr formula_three(new Formula(kNot));
  formula_three->Add(&top);  // Looping here.
  top.formula(std::move(formula_one));
  middle.formula(std::move(formula_two));
  bottom.formula(std::move(formula_three));

  std::vector<Gate*> cycle;
  bool ret = cycle::DetectCycle(&root, &cycle);
  CHECK(ret);
  std::vector<Gate*> print_cycle = {&top, &bottom, &middle, &top};
  CHECK(cycle == print_cycle);
  CHECK(cycle::PrintCycle(cycle) == "Top->Middle->Bottom->Top");
}

// Test gate connective validation.
TEST_CASE("FormulaTest.Validate", "[mef::event]") {
  BasicEvent arg_one("a");
  BasicEvent arg_two("b");
  BasicEvent arg_three("c");

  for (Connective nary : {kAnd, kOr, kNand, kNor}) {
    CAPTURE(nary);
    INFO(kConnectiveToString[nary]);
    Formula top(nary);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
    top.Add(&arg_one);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
    top.Add(&arg_two);
    CHECK_NOTHROW(top.Validate());
    top.Add(&arg_three);
    CHECK_NOTHROW(top.Validate());
  }

  for (Connective unary : {kNot, kNull}) {
    CAPTURE(unary);
    INFO(kConnectiveToString[unary]);
    Formula top(unary);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
    top.Add(&arg_one);
    CHECK_NOTHROW(top.Validate());
    top.Add(&arg_two);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
  }

  for (Connective binary : {kXor, kImply, kIff}) {
    CAPTURE(binary);
    INFO(kConnectiveToString[binary]);
    Formula top(binary);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
    top.Add(&arg_one);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
    top.Add(&arg_two);
    CHECK_NOTHROW(top.Validate());
    top.Add(&arg_three);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
  }

  SECTION("ATLEAST connective") {
    Formula top(kAtleast);
    top.min_number(2);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
    top.Add(&arg_one);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
    top.Add(&arg_two);
    CHECK_THROWS_AS(top.Validate(), ValidityError);
    top.Add(&arg_three);
    CHECK_NOTHROW(top.Validate());
  }
}

TEST_CASE("MEFGateTest.Inhibit", "[mef::event]") {
  BasicEvent arg_one("a");
  BasicEvent arg_two("b");
  BasicEvent arg_three("c");
  // INHIBIT Gate tests.
  Attribute inh_attr{"flavor", "inhibit"};
  Gate top("top");
  top.formula(FormulaPtr(new Formula(kAnd)));
  top.AddAttribute(inh_attr);
  CHECK_THROWS_AS(top.Validate(), ValidityError);
  top.formula().Add(&arg_one);
  CHECK_THROWS_AS(top.Validate(), ValidityError);
  top.formula().Add(&arg_two);
  CHECK_THROWS_AS(top.Validate(), ValidityError);

  top.formula().Add(&arg_three);
  CHECK_THROWS_AS(top.Validate(), ValidityError);

  top.formula(FormulaPtr(new Formula(kAnd)));

  Attribute cond{"flavor", "conditional"};
  arg_three.AddAttribute(cond);
  top.formula().Add(&arg_one);  // Basic event.
  top.formula().Add(&arg_three);  // Conditional event.
  CHECK_NOTHROW(top.Validate());
  arg_one.AddAttribute(cond);
  CHECK_THROWS_AS(top.Validate(), ValidityError);
}

TEST_CASE("PrimaryEventTest.HouseProbability", "[mef::event]") {
  // House primary event.
  HouseEvent primary("valve");
  CHECK_FALSE(primary.state());  // Default state.
  // Setting with a valid values.
  CHECK_NOTHROW(primary.state(true));
  CHECK(primary.state());
  CHECK_NOTHROW(primary.state(false));
  CHECK_FALSE(primary.state());
}

}  // namespace scram::mef::test
