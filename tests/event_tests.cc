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

#include <gtest/gtest.h>

#include "cycle.h"
#include "error.h"
#include "expression/constant.h"

namespace scram::mef::test {

// Test for Event base class.
TEST(EventTest, Id) {
  BasicEvent event("event_name");
  EXPECT_EQ(event.id(), "event_name");
}

TEST(BasicEventTest, ExpressionReset) {
  BasicEvent event("event");
  EXPECT_FALSE(event.HasExpression());
  ConstantExpression p_init(0.5);
  event.expression(&p_init);
  ASSERT_TRUE(event.HasExpression());
  EXPECT_EQ(0.5, event.p());

  ConstantExpression p_change(0.1);
  event.expression(&p_change);
  ASSERT_TRUE(event.HasExpression());
  EXPECT_EQ(0.1, event.p());
}

TEST(BasicEventTest, Validate) {
  BasicEvent event("event");
  EXPECT_FALSE(event.HasExpression());
  ConstantExpression p_valid(0.5);
  event.expression(&p_valid);
  ASSERT_TRUE(event.HasExpression());
  EXPECT_NO_THROW(event.Validate());

  ConstantExpression p_negative(-0.1);
  event.expression(&p_negative);
  ASSERT_TRUE(event.HasExpression());
  EXPECT_THROW(event.Validate(), ValidityError);

  ConstantExpression p_large(1.1);
  event.expression(&p_large);
  ASSERT_TRUE(event.HasExpression());
  EXPECT_THROW(event.Validate(), ValidityError);
}

TEST(FormulaTest, VoteNumber) {
  FormulaPtr top(new Formula(kAnd));
  EXPECT_EQ(kAnd, top->type());
  // Setting a vote number for non-Vote formula is an error.
  EXPECT_THROW(top->vote_number(2), LogicError);
  // Resetting to VOTE formula.
  top = FormulaPtr(new Formula(kVote));
  EXPECT_EQ(kVote, top->type());
  // No vote number.
  EXPECT_THROW(top->vote_number(), LogicError);
  // Illegal vote number.
  EXPECT_THROW(top->vote_number(-2), ValidityError);
  // Legal vote number.
  EXPECT_NO_THROW(top->vote_number(2));
  // Trying to reset the vote number.
  EXPECT_THROW(top->vote_number(2), LogicError);
  // Requesting the vote number should succeed.
  ASSERT_NO_THROW(top->vote_number());
  EXPECT_EQ(2, top->vote_number());
}

TEST(FormulaTest, EventArguments) {
  FormulaPtr top(new Formula(kAnd));
  BasicEvent first_child("first");
  BasicEvent second_child("second");
  EXPECT_EQ(0, top->num_args());
  // Adding first child.
  EXPECT_NO_THROW(top->AddArgument(&first_child));
  // Re-adding a child must cause an error.
  EXPECT_THROW(top->AddArgument(&first_child), ValidityError);
  // Check the contents of the children container.
  EXPECT_EQ(&first_child, std::get<BasicEvent*>(top->event_args().front()));
  // Adding another child.
  EXPECT_NO_THROW(top->AddArgument(&second_child));
  EXPECT_EQ(2, top->event_args().size());
  EXPECT_EQ(&second_child, std::get<BasicEvent*>(top->event_args().back()));

  EXPECT_NO_THROW(top->RemoveArgument(&first_child));
  EXPECT_EQ(1, top->num_args());
  EXPECT_THROW(top->RemoveArgument(&first_child), LogicError);
}

TEST(FormulaTest, FormulaArguments) {
  FormulaPtr top(new Formula(kAnd));
  FormulaPtr arg(new Formula(kOr));
  EXPECT_EQ(0, top->num_args());
  Formula* shadow = arg.get();
  // Adding first child.
  EXPECT_NO_THROW(top->AddArgument(std::move(arg)));  // arg is gone.
  EXPECT_EQ(shadow, top->formula_args().begin()->get());
}

TEST(MEFGateTest, Cycle) {
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
  EXPECT_TRUE(ret);
  std::vector<Gate*> print_cycle = {&top, &bottom, &middle, &top};
  EXPECT_EQ(print_cycle, cycle);
  EXPECT_EQ("Top->Middle->Bottom->Top", cycle::PrintCycle(cycle));
}

// Test gate type validation.
TEST(FormulaTest, Validate) {
  FormulaPtr top(new Formula(kAnd));
  BasicEvent arg_one("a");
  BasicEvent arg_two("b");
  BasicEvent arg_three("c");

  // AND Formula tests.
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  EXPECT_NO_THROW(top->Validate());
  top->AddArgument(&arg_three);
  EXPECT_NO_THROW(top->Validate());

  // OR Formula tests.
  top = FormulaPtr(new Formula(kOr));
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  EXPECT_NO_THROW(top->Validate());
  top->AddArgument(&arg_three);
  EXPECT_NO_THROW(top->Validate());

  // NOT Formula tests.
  top = FormulaPtr(new Formula(kNot));
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  EXPECT_NO_THROW(top->Validate());
  top->AddArgument(&arg_two);
  EXPECT_THROW(top->Validate(), ValidityError);

  // NULL Formula tests.
  top = FormulaPtr(new Formula(kNull));
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  EXPECT_NO_THROW(top->Validate());
  top->AddArgument(&arg_two);
  EXPECT_THROW(top->Validate(), ValidityError);

  // NOR Formula tests.
  top = FormulaPtr(new Formula(kNor));
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  EXPECT_NO_THROW(top->Validate());
  top->AddArgument(&arg_three);
  EXPECT_NO_THROW(top->Validate());

  // NAND Formula tests.
  top = FormulaPtr(new Formula(kNand));
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  EXPECT_NO_THROW(top->Validate());
  top->AddArgument(&arg_three);
  EXPECT_NO_THROW(top->Validate());

  // XOR Formula tests.
  top = FormulaPtr(new Formula(kXor));
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  EXPECT_NO_THROW(top->Validate());
  top->AddArgument(&arg_three);
  EXPECT_THROW(top->Validate(), ValidityError);

  // VOTE/ATLEAST formula tests.
  top = FormulaPtr(new Formula(kVote));
  top->vote_number(2);
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_one);
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_two);
  EXPECT_THROW(top->Validate(), ValidityError);
  top->AddArgument(&arg_three);
  EXPECT_NO_THROW(top->Validate());
}

TEST(MEFGateTest, Inhibit) {
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
  EXPECT_THROW(top->Validate(), ValidityError);
  top->formula().AddArgument(&arg_one);
  EXPECT_THROW(top->Validate(), ValidityError);
  top->formula().AddArgument(&arg_two);
  EXPECT_THROW(top->Validate(), ValidityError);

  top->formula().AddArgument(&arg_three);
  EXPECT_THROW(top->Validate(), ValidityError);

  top = GatePtr(new Gate("top"));
  top->formula(FormulaPtr(new Formula(kAnd)));
  top->AddAttribute(inh_attr);

  Attribute cond;
  cond.name = "flavor";
  cond.value = "conditional";
  arg_three.AddAttribute(cond);
  top->formula().AddArgument(&arg_one);  // Basic event.
  top->formula().AddArgument(&arg_three);  // Conditional event.
  EXPECT_NO_THROW(top->Validate());
  arg_one.AddAttribute(cond);
  EXPECT_THROW(top->Validate(), ValidityError);
}

TEST(PrimaryEventTest, HouseProbability) {
  // House primary event.
  HouseEventPtr primary(new HouseEvent("valve"));
  EXPECT_FALSE(primary->state());  // Default state.
  // Setting with a valid values.
  EXPECT_NO_THROW(primary->state(true));
  EXPECT_TRUE(primary->state());
  EXPECT_NO_THROW(primary->state(false));
  EXPECT_FALSE(primary->state());
}

}  // namespace scram::mef::test
