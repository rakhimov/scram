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

TEST_CASE("FormulaTest.VoteNumber", "[mef::event]") {
  FormulaPtr top(new Formula(kAnd));
  CHECK(top->type() == kAnd);
  // Setting a vote number for non-Vote formula is an error.
  CHECK_THROWS_AS(top->vote_number(2), LogicError);
  // Resetting to VOTE formula.
  top = FormulaPtr(new Formula(kVote));
  CHECK(top->type() == kVote);
  // No vote number.
  CHECK_THROWS_AS(top->vote_number(), LogicError);
  // Illegal vote number.
  CHECK_THROWS_AS(top->vote_number(-2), ValidityError);
  // Legal vote number.
  CHECK_NOTHROW(top->vote_number(2));
  // Trying to reset the vote number.
  CHECK_THROWS_AS(top->vote_number(2), LogicError);
  // Requesting the vote number should succeed.
  REQUIRE_NOTHROW(top->vote_number());
  CHECK(top->vote_number() == 2);
}

TEST_CASE("FormulaTest.EventArguments", "[mef::event]") {
  FormulaPtr top(new Formula(kAnd));
  BasicEvent first_child("first");
  BasicEvent second_child("second");
  CHECK(top->num_args() == 0);
  // Adding first child.
  CHECK_NOTHROW(top->AddArgument(&first_child));
  // Re-adding a child must cause an error.
  CHECK_THROWS_AS(top->AddArgument(&first_child), ValidityError);
  // Check the contents of the children container.
  CHECK(std::get<BasicEvent*>(top->event_args().front()) == &first_child);
  // Adding another child.
  CHECK_NOTHROW(top->AddArgument(&second_child));
  CHECK(top->event_args().size() == 2);
  CHECK(std::get<BasicEvent*>(top->event_args().back()) == &second_child);

  CHECK_NOTHROW(top->RemoveArgument(&first_child));
  CHECK(top->num_args() == 1);
  CHECK_THROWS_AS(top->RemoveArgument(&first_child), LogicError);
}

TEST_CASE("FormulaTest.FormulaArguments", "[mef::event]") {
  FormulaPtr top(new Formula(kAnd));
  FormulaPtr arg(new Formula(kOr));
  CHECK(top->num_args() == 0);
  Formula* shadow = arg.get();
  // Adding first child.
  CHECK_NOTHROW(top->AddArgument(std::move(arg)));  // arg is gone.
  CHECK(top->formula_args().begin()->get() == shadow);
}

TEST_CASE("MEFGateTest.Cycle", "[mef::event]") {
  Gate root("root");  // Should not appear in the cycle.
  Gate top("Top");
  Gate middle("Middle");
  Gate bottom("Bottom");

  FormulaPtr formula_root(new Formula(kNot));
  formula_root->AddArgument(&top);
  root.formula(std::move(formula_root));

  FormulaPtr formula_one(new Formula(kNot));
  formula_one->AddArgument(&middle);
  FormulaPtr formula_two(new Formula(kNot));
  formula_two->AddArgument(&bottom);
  FormulaPtr formula_three(new Formula(kNot));
  formula_three->AddArgument(&top);  // Looping here.
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

// Test gate type validation.
TEST_CASE("FormulaTest.Validate", "[mef::event]") {
  FormulaPtr top(new Formula(kAnd));
  BasicEvent arg_one("a");
  BasicEvent arg_two("b");
  BasicEvent arg_three("c");

  // AND Formula tests.
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  CHECK_NOTHROW(top->Validate());
  top->AddArgument(&arg_three);
  CHECK_NOTHROW(top->Validate());

  // OR Formula tests.
  top = FormulaPtr(new Formula(kOr));
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  CHECK_NOTHROW(top->Validate());
  top->AddArgument(&arg_three);
  CHECK_NOTHROW(top->Validate());

  // NOT Formula tests.
  top = FormulaPtr(new Formula(kNot));
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  CHECK_NOTHROW(top->Validate());
  top->AddArgument(&arg_two);
  CHECK_THROWS_AS(top->Validate(), ValidityError);

  // NULL Formula tests.
  top = FormulaPtr(new Formula(kNull));
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  CHECK_NOTHROW(top->Validate());
  top->AddArgument(&arg_two);
  CHECK_THROWS_AS(top->Validate(), ValidityError);

  // NOR Formula tests.
  top = FormulaPtr(new Formula(kNor));
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  CHECK_NOTHROW(top->Validate());
  top->AddArgument(&arg_three);
  CHECK_NOTHROW(top->Validate());

  // NAND Formula tests.
  top = FormulaPtr(new Formula(kNand));
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  CHECK_NOTHROW(top->Validate());
  top->AddArgument(&arg_three);
  CHECK_NOTHROW(top->Validate());

  // XOR Formula tests.
  top = FormulaPtr(new Formula(kXor));
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  CHECK_NOTHROW(top->Validate());
  top->AddArgument(&arg_three);
  CHECK_THROWS_AS(top->Validate(), ValidityError);

  // VOTE/ATLEAST formula tests.
  top = FormulaPtr(new Formula(kVote));
  top->vote_number(2);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->AddArgument(&arg_three);
  CHECK_NOTHROW(top->Validate());
}

TEST_CASE("MEFGateTest.Inhibit", "[mef::event]") {
  BasicEvent arg_one("a");
  BasicEvent arg_two("b");
  BasicEvent arg_three("c");
  // INHIBIT Gate tests.
  Attribute inh_attr;
  inh_attr.name = "flavor";
  inh_attr.value = "inhibit";
  GatePtr top(new Gate("top"));
  top->formula(FormulaPtr(new Formula(kAnd)));
  top->AddAttribute(inh_attr);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->formula().AddArgument(&arg_one);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
  top->formula().AddArgument(&arg_two);
  CHECK_THROWS_AS(top->Validate(), ValidityError);

  top->formula().AddArgument(&arg_three);
  CHECK_THROWS_AS(top->Validate(), ValidityError);

  top = GatePtr(new Gate("top"));
  top->formula(FormulaPtr(new Formula(kAnd)));
  top->AddAttribute(inh_attr);

  Attribute cond;
  cond.name = "flavor";
  cond.value = "conditional";
  arg_three.AddAttribute(cond);
  top->formula().AddArgument(&arg_one);  // Basic event.
  top->formula().AddArgument(&arg_three);  // Conditional event.
  CHECK_NOTHROW(top->Validate());
  arg_one.AddAttribute(cond);
  CHECK_THROWS_AS(top->Validate(), ValidityError);
}

TEST_CASE("PrimaryEventTest.HouseProbability", "[mef::event]") {
  // House primary event.
  HouseEventPtr primary(new HouseEvent("valve"));
  CHECK_FALSE(primary->state());  // Default state.
  // Setting with a valid values.
  CHECK_NOTHROW(primary->state(true));
  CHECK(primary->state());
  CHECK_NOTHROW(primary->state(false));
  CHECK_FALSE(primary->state());
}

}  // namespace scram::mef::test
