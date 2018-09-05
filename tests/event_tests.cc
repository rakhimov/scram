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

#include <catch2/catch.hpp>

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

TEST_CASE("FormulaTest.MinMaxNumber", "[mef::event]") {
  BasicEvent first_child("first");
  BasicEvent second_child("second");

  SECTION("Invalid connective") {
    Formula::ArgSet arg_set = {&first_child, &second_child};
    CHECK_THROWS_AS(Formula(kAnd, arg_set, 2), LogicError);
    CHECK_THROWS_AS(Formula(kAnd, arg_set, 2, 3), LogicError);

    Formula top(kAnd, arg_set);
    CHECK(top.connective() == kAnd);
    CHECK(top.min_number() == std::nullopt);
    CHECK(top.max_number() == std::nullopt);
  }

  SECTION("Atleast") {
    BasicEvent third_child("third");
    Formula::ArgSet arg_set = {&first_child, &second_child, &third_child};
    Formula top(kAtleast, arg_set, 2);
    CHECK(top.connective() == kAtleast);
    CHECK(top.min_number() == 2);
    // No min number.
    CHECK_THROWS_AS(Formula(kAtleast, arg_set), ValidityError);
    // Redundant max number.
    CHECK_THROWS_AS(Formula(kAtleast, arg_set, 2, 3), LogicError);
    CHECK_THROWS_AS(Formula(kAtleast, arg_set, {}, 3), LogicError);
    // Illegal min number.
    CHECK_THROWS_AS(Formula(kAtleast, arg_set, -2), LogicError);
    CHECK_THROWS_AS(Formula(kAtleast, arg_set, 1), ValidityError);
    CHECK_THROWS_AS(Formula(kAtleast, arg_set, 0), ValidityError);
    CHECK_THROWS_AS(Formula(kAtleast, arg_set, 3), ValidityError);
    CHECK_THROWS_AS(Formula(kAtleast, arg_set, 4), ValidityError);
  }

  SECTION("Cardinality") {
    BasicEvent third_child("third");
    Formula::ArgSet arg_set = {&first_child, &second_child, &third_child};
    Formula top(kCardinality, arg_set, 2, 3);
    CHECK(top.connective() == kCardinality);
    CHECK(top.min_number() == 2);
    CHECK(top.max_number() == 3);

    CHECK_THROWS_AS(Formula(kCardinality, arg_set), ValidityError);
    CHECK_THROWS_AS(Formula(kCardinality, arg_set, 2), ValidityError);
    CHECK_THROWS_AS(Formula(kCardinality, arg_set, {}, 2), ValidityError);
    CHECK_THROWS_AS(Formula(kCardinality, arg_set, -2, 3), LogicError);
    CHECK_THROWS_AS(Formula(kCardinality, arg_set, 2, -3), LogicError);
    CHECK_THROWS_AS(Formula(kCardinality, arg_set, 2, 4), ValidityError);
    CHECK_THROWS_AS(Formula(kCardinality, arg_set, 2, 1), ValidityError);

    CHECK_NOTHROW(Formula(kCardinality, arg_set, 0, 0));
    CHECK_NOTHROW(Formula(kCardinality, arg_set, 0, 2));
    CHECK_NOTHROW(Formula(kCardinality, arg_set, 2, 2));

    // Empty args.
    CHECK_THROWS_AS(Formula(kCardinality, {}, 0, 0), ValidityError);
    CHECK_THROWS_AS(Formula(kCardinality, {}, 0, 1), ValidityError);
  }
}

TEST_CASE("FormulaTest.EventArguments", "[mef::event]") {
  Formula::ArgSet arg_set;
  BasicEvent first_child("first");
  BasicEvent second_child("second");
  CHECK(arg_set.size() == 0);
  // Adding first child.
  CHECK_NOTHROW(arg_set.Add(&first_child));
  // Re-adding a child must cause an error.
  CHECK_THROWS_AS(arg_set.Add(&first_child), ValidityError);
  // Check the contents of the children container.
  CHECK(std::get<BasicEvent*>(arg_set.data().front().event) == &first_child);
  // Adding another child.
  CHECK_NOTHROW(arg_set.Add(&second_child));
  CHECK(arg_set.size() == 2);
  CHECK(std::get<BasicEvent*>(arg_set.data().back().event) == &second_child);

  CHECK_NOTHROW(arg_set.Remove(&first_child));
  CHECK(arg_set.size() == 1);
  CHECK_THROWS_AS(arg_set.Remove(&first_child), LogicError);
}

TEST_CASE("FormulaTest.InvalidComplementArguments", "[mef::event]") {
  BasicEvent arg_event("first");
  SECTION("NULL connective with complement") {
    REQUIRE_THROWS_AS(Formula(kNull, {{true, &arg_event}}), LogicError);
  }
  SECTION("NOT connective with complement") {
    REQUIRE_THROWS_AS(Formula(kNot, {{true, &arg_event}}), LogicError);
  }
}

TEST_CASE("FormulaTest.DuplicateViaComplement", "[mef::event]") {
  Formula::ArgSet arg_set;
  BasicEvent arg_event("first");
  SECTION("Complement first") {
    REQUIRE_NOTHROW(arg_set.Add(&arg_event, true));
    CHECK(arg_set.size() == 1);
    REQUIRE_THROWS_AS(arg_set.Add(&arg_event), DuplicateElementError);
  }
  SECTION("Complement second") {
    REQUIRE_NOTHROW(arg_set.Add(&arg_event));
    CHECK(arg_set.size() == 1);
    REQUIRE_THROWS_AS(arg_set.Add(&arg_event, true), DuplicateElementError);
  }
}

TEST_CASE("FormulaTest.InvalidConstantArguments", "[mef::event]") {
  SECTION("Constant True event") {
    REQUIRE_THROWS_AS(Formula(kNot, {&HouseEvent::kTrue}), LogicError);
  }
  SECTION("Constant False event") {
    REQUIRE_THROWS_AS(Formula(kNot, {&HouseEvent::kFalse}), LogicError);
  }
}

TEST_CASE("FormulaTest.Swap", "[mef::event]") {
  BasicEvent one("one"), two("two"), three("three"), four("four");
  Formula formula(kAnd, {{true, &one}, {false, &two}});
  const Formula orig(formula);
  auto check_orig = [orig = formula, &formula] {
    CHECK(formula.connective() == orig.connective());
    CHECK(formula.args() == orig.args());
    CHECK(formula.min_number() == orig.min_number());
  };

  SECTION("Correct") {
    REQUIRE_NOTHROW(formula.Swap(&one, &three));
    CHECK(three.usage());
    CHECK(formula.args().front().event == Formula::ArgEvent(&three));
    CHECK(formula.args().front().complement);

    REQUIRE_NOTHROW(formula.Swap(&two, &one));
    CHECK(formula.args().back().event == Formula::ArgEvent(&one));
    CHECK(formula.args().back().complement == false);
  }

  SECTION("Duplicate") {
    REQUIRE_THROWS_AS(formula.Swap(&one, &two), DuplicateElementError);
    REQUIRE_THROWS_AS(formula.Swap(&two, &one), DuplicateElementError);
    check_orig();
  }

  SECTION("Nonexistent") {
    REQUIRE_THROWS_AS(formula.Swap(&three, &four), LogicError);
    REQUIRE_THROWS_AS(formula.Swap(&three, &three), LogicError);
    check_orig();
  }

  SECTION("The same arg") {
    REQUIRE_NOTHROW(formula.Swap(&one, &one));
    REQUIRE_NOTHROW(formula.Swap(&two, &two));
    check_orig();
  }

  SECTION("Invalid constant") {
    formula = Formula(kNot, {&one});
    REQUIRE_THROWS_AS(formula.Swap(&one, &HouseEvent::kTrue), LogicError);
    REQUIRE_THROWS_AS(formula.Swap(&one, &HouseEvent::kFalse), LogicError);
  }
}

TEST_CASE("MEFGateTest.Cycle", "[mef::event]") {
  Gate root("root");  // Should not appear in the cycle.
  Gate top("Top");
  Gate middle("Middle");
  Gate bottom("Bottom");

  root.formula(std::make_unique<Formula>(kNot, Formula::ArgSet{&top}));
  top.formula(std::make_unique<Formula>(kNot, Formula::ArgSet{&middle}));
  middle.formula(std::make_unique<Formula>(kNot, Formula::ArgSet{&bottom}));
  bottom.formula(std::make_unique<Formula>(kNot, Formula::ArgSet{&top}));

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

  SECTION("Nary") {
    Connective nary = GENERATE(kAnd, kOr, kNand, kNor);
    CAPTURE(nary);
    INFO(kConnectiveToString[nary]);
    CHECK_THROWS_AS(Formula(nary, {}), ValidityError);
    CHECK_THROWS_AS(Formula(nary, {&arg_one}), ValidityError);
    CHECK_NOTHROW(Formula(nary, {&arg_one, &arg_two}));
    CHECK_NOTHROW(Formula(nary, {&arg_one, &arg_two, &arg_three}));
  }

  SECTION("Unary") {
    Connective unary = GENERATE(kNot, kNull);
    CAPTURE(unary);
    INFO(kConnectiveToString[unary]);
    CHECK_THROWS_AS(Formula(unary, {}), ValidityError);
    CHECK_NOTHROW(Formula(unary, {&arg_one}));
    CHECK_THROWS_AS(Formula(unary, {&arg_one, &arg_two}), ValidityError);
  }

  SECTION("Binary") {
    Connective binary = GENERATE(kXor, kImply, kIff);
    CAPTURE(binary);
    INFO(kConnectiveToString[binary]);
    CHECK_THROWS_AS(Formula(binary, {}), ValidityError);
    CHECK_THROWS_AS(Formula(binary, {&arg_one}), ValidityError);
    CHECK_NOTHROW(Formula(binary, {&arg_one, &arg_two}));
    CHECK_THROWS_AS(Formula(binary, {&arg_one, &arg_two, &arg_three}),
                    ValidityError);
  }

  SECTION("ATLEAST connective") {
    CHECK_THROWS_AS(Formula(kAtleast, {}, 2), ValidityError);
    CHECK_THROWS_AS(Formula(kAtleast, {&arg_one}, 2), ValidityError);
    CHECK_THROWS_AS(Formula(kAtleast, {&arg_one, &arg_two}, 2), ValidityError);
    CHECK_NOTHROW(Formula(kAtleast, {&arg_one, &arg_two, &arg_three}, 2));
  }
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
