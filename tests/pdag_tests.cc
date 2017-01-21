/*
 * Copyright (C) 2015-2017 Olzhas Rakhimov
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

#include "pdag.h"

#include <gtest/gtest.h>

#include "initializer.h"
#include "fault_tree.h"
#include "model.h"
#include "settings.h"

namespace scram {
namespace core {
namespace test {

TEST(PdagTest, Print) {
  std::unique_ptr<mef::Initializer> init;
  ASSERT_NO_THROW(
      init.reset(new mef::Initializer(
          {"./share/scram/input/fta/correct_formulas.xml"}, Settings())));
  const mef::FaultTreePtr& ft = *init->model()->fault_trees().begin();
  Pdag graph(*ft->top_events().front());
  graph.Print();
}

static_assert(kNumOperators == 8, "New gate types are not considered!");

class GateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    var_one = std::make_shared<Variable>(&graph);
    var_two = std::make_shared<Variable>(&graph);
    var_three = std::make_shared<Variable>(&graph);
    vars_ = {var_one, var_two, var_three};
    for (int i = 0; i < 2; ++i)
      vars_.emplace_back(new Variable(&graph));  // Extra.
  }

  void TearDown() override {}

  /// Sets up the main gate with the default variables.
  ///
  /// @param[in] type  Type for the main gate.
  /// @param[in] num_vars  Desired number of variables.
  ///
  /// @note The setup is not for one-arg gates (NOT/NULL).
  /// @note For K/N gates, K is set to 2 by default.
  void DefineGate(Operator type, int num_vars) {
    assert(num_vars < 6);
    assert(!(type == kVote && num_vars < 2));

    g = std::make_shared<Gate>(type, &graph);
    if (type == kVote)
      g->vote_number(2);
    for (int i = 0; i < num_vars; ++i)
      g->AddArg(vars_[i]);

    assert(!g->constant());
    assert(g->type() == type);
    assert(g->args().size() == num_vars);
    assert(g->args<Variable>().size() == num_vars);
    assert(g->args<Gate>().empty());
  }

  GatePtr g;  // Main gate for manipulations.
  // Collection of variables for gate input.
  VariablePtr var_one;
  VariablePtr var_two;
  VariablePtr var_three;

 private:
  Pdag graph;  // The manager of unique indices.
  std::vector<VariablePtr> vars_;  // For convenience only.
};

#ifndef NDEBUG
TEST_F(GateTest, AddArgDeathTests) {
  DefineGate(kNull, 1);
  EXPECT_DEATH(g->AddArg(var_two), "");
  DefineGate(kNot, 1);
  EXPECT_DEATH(g->AddArg(var_two), "");
  DefineGate(kXor, 2);
  EXPECT_DEATH(g->AddArg(var_three), "");
  DefineGate(kAnd, 1);
  EXPECT_DEATH(g->AddArg(1, var_two), "");  // Wrong index.
  DefineGate(kAnd, 1);
  EXPECT_DEATH(g->AddArg(0, var_two), "");  // Wrong index.
  DefineGate(kAnd, 2);
  g->MakeConstant(false);  // Constant state.
  EXPECT_DEATH(g->AddArg(var_two->index(), var_two), "");  // Wrong index.
  DefineGate(kVote, 3);
  g->vote_number(-1);  // Negative vote number.
  EXPECT_DEATH(g->AddArg(var_three->index(), var_three), "");
}
#endif

/// Collection of tests
/// for addition of an existing argument to a gate.
///
/// @param short_type  Short name of the gate, i.e., 'And'.
///                    It must have the same root in Operator, i.e., 'kAnd'.
/// @param num_vars  The number of variables to initialize the gate.
#define ADD_ARG_IGNORE_TEST(short_type, num_vars)  \
  DefineGate(k##short_type, num_vars);             \
  g->AddArg(var_one);                              \
  ASSERT_FALSE(g->constant());                     \
  EXPECT_EQ(num_vars, g->args().size());           \
  EXPECT_EQ(num_vars, g->args<Variable>().size()); \
  EXPECT_TRUE(g->args<Gate>().empty())

/// Tests addition of an existing argument to PDAG gates
/// that do not change the type of the gate.
///
/// @param short_type  Short name of the gate type, i.e., 'And'.
#define TEST_DUP_ARG_IGNORE(short_type)              \
  TEST_F(GateTest, DuplicateArgIgnore##short_type) { \
    ADD_ARG_IGNORE_TEST(short_type, 2);              \
    EXPECT_EQ(k##short_type, g->type());             \
  }

TEST_DUP_ARG_IGNORE(And)
TEST_DUP_ARG_IGNORE(Or)
TEST_DUP_ARG_IGNORE(Nand)
TEST_DUP_ARG_IGNORE(Nor)

#undef TEST_DUP_ARG_IGNORE

/// Tests addition of an existing argument
/// that changes the type of the gate.
///
/// @param init_type  Short name of the initial type of the gate, i.e., 'And'.
/// @param final_type  The resulting type of addition operation.
#define TEST_DUP_ARG_TYPE_CHANGE(init_type, final_type)   \
  TEST_F(GateTest, DuplicateArgChange##init_type##Type) { \
    ADD_ARG_IGNORE_TEST(init_type, 1);                    \
    EXPECT_EQ(k##final_type, g->type());                  \
  }

TEST_DUP_ARG_TYPE_CHANGE(Or, Null)
TEST_DUP_ARG_TYPE_CHANGE(And, Null)
TEST_DUP_ARG_TYPE_CHANGE(Nor, Not)
TEST_DUP_ARG_TYPE_CHANGE(Nand, Not)

#undef TEST_DUP_ARG_TYPE_CHANGE
#undef ADD_ARG_IGNORE_TEST

TEST_F(GateTest, DuplicateArgXor) {
  DefineGate(kXor, 1);
  g->AddArg(var_one);
  ASSERT_TRUE(g->constant());
  ASSERT_EQ(1, g->args().size());
  EXPECT_GT(0, *g->args().begin());
}

TEST_F(GateTest, DuplicateArgVoteToNull) {
  DefineGate(kVote, 2);
  g->AddArg(var_one);
  ASSERT_FALSE(g->constant());
  EXPECT_EQ(kNull, g->type());
  EXPECT_EQ(1, g->args().size());
  EXPECT_EQ(var_two->index(), g->args<Variable>().begin()->first);
}

TEST_F(GateTest, DuplicateArgVoteToAnd) {
  DefineGate(kVote, 3);
  g->vote_number(3);  // K equals to the number of input arguments.
  g->AddArg(var_one);
  ASSERT_FALSE(g->constant());
  EXPECT_EQ(kAnd, g->type());
  EXPECT_EQ(2, g->args().size());
  ASSERT_EQ(1, g->args<Variable>().size());
  EXPECT_EQ(var_one->index(), g->args<Variable>().begin()->first);
  ASSERT_EQ(1, g->args<Gate>().size());

  GatePtr sub = g->args<Gate>().begin()->second;
  EXPECT_EQ(kOr, sub->type());  // Special case. K/N is in general.
  EXPECT_EQ(1, sub->vote_number());  // This is the reason.
  Gate::ArgSet vars;
  vars.insert(var_two->index());
  vars.insert(var_three->index());
  EXPECT_EQ(vars, sub->args());
  EXPECT_EQ(2, sub->args<Variable>().size());
}

TEST_F(GateTest, DuplicateArgVoteToOrWithOneClone) {
  DefineGate(kVote, 3);
  g->vote_number(2);
  g->AddArg(var_one);
  ASSERT_FALSE(g->constant());
  EXPECT_EQ(kOr, g->type());
  EXPECT_EQ(2, g->args().size());
  ASSERT_EQ(1, g->args<Variable>().size());
  EXPECT_EQ(var_one->index(), g->args<Variable>().begin()->first);
  ASSERT_EQ(1, g->args<Gate>().size());

  GatePtr sub = g->args<Gate>().begin()->second;
  EXPECT_EQ(kAnd, sub->type());  // Special case. K/N is in general.
  EXPECT_EQ(2, sub->vote_number());
  EXPECT_EQ(2, sub->args().size());  // This is the reason.
  Gate::ArgSet vars;
  vars.insert(var_two->index());
  vars.insert(var_three->index());
  EXPECT_EQ(vars, sub->args());
  EXPECT_EQ(2, sub->args<Variable>().size());
}

TEST_F(GateTest, DuplicateArgVoteToOrWithTwoClones) {
  DefineGate(kVote, 5);
  g->vote_number(3);
  g->AddArg(var_one);
  ASSERT_FALSE(g->constant());
  EXPECT_EQ(kOr, g->type());
  EXPECT_EQ(2, g->args().size());
  EXPECT_TRUE(g->args<Variable>().empty());
  ASSERT_EQ(2, g->args<Gate>().size());
  auto it = g->args<Gate>().begin();
  GatePtr and_gate = it->second;  // Guessing.
  ++it;
  GatePtr clone_one = it->second;  // Guessing.
  // Correcting the guess.
  if (and_gate->type() != kAnd)
    std::swap(and_gate, clone_one);
  ASSERT_EQ(kAnd, and_gate->type());
  ASSERT_EQ(kVote, clone_one->type());

  ASSERT_FALSE(clone_one->constant());
  EXPECT_EQ(3, clone_one->vote_number());
  EXPECT_EQ(4, clone_one->args().size());
  EXPECT_EQ(4, clone_one->args<Variable>().size());

  ASSERT_FALSE(and_gate->constant());
  EXPECT_EQ(2, and_gate->args().size());
  ASSERT_EQ(1, and_gate->args<Variable>().size());
  EXPECT_EQ(var_one->index(), and_gate->args<Variable>().begin()->first);
  ASSERT_EQ(1, and_gate->args<Gate>().size());

  GatePtr clone_two = and_gate->args<Gate>().begin()->second;
  ASSERT_FALSE(clone_two->constant());
  EXPECT_EQ(kOr, clone_two->type());  // Special case. K/N is in general.
  EXPECT_EQ(1, clone_two->vote_number());  // This is the reason.
  EXPECT_EQ(4, clone_two->args().size());
  EXPECT_EQ(4, clone_two->args<Variable>().size());
}

/// Collection of tests
/// for addition of the complement of an existing argument to a gate.
///
/// @param short_type  Short name of the gate, i.e., 'And'.
///                    It must have the same root in Operator, i.e., 'kAnd'.
/// @param const_state  The notion of Boolean constant in the gate (TRUE/FALSE).
#define TEST_ADD_COMPLEMENT_ARG(short_type, const_state) \
  TEST_F(GateTest, ComplementArg##short_type) {          \
    DefineGate(k##short_type, 1);                        \
    g->AddArg(var_one, true);                            \
    ASSERT_TRUE(g->constant());                          \
    ASSERT_EQ(1, g->args().size());                      \
    ASSERT_##const_state(*g->args().begin() > 0);        \
    EXPECT_TRUE(g->args<Variable>().empty());            \
    EXPECT_TRUE(g->args<Gate>().empty());                \
  }

TEST_ADD_COMPLEMENT_ARG(And, FALSE)
TEST_ADD_COMPLEMENT_ARG(Or, TRUE)
TEST_ADD_COMPLEMENT_ARG(Nand, TRUE)
TEST_ADD_COMPLEMENT_ARG(Nor, FALSE)
TEST_ADD_COMPLEMENT_ARG(Xor, TRUE)

#undef TEST_ADD_COMPLEMENT_ARG

/// Collection of VOTE (K/N) gate tests
/// for addition of the complement of an existing argument.
///
/// @param num_vars  Initial number of variables.
/// @param v_num  Initial K number of the gate.
/// @param final_type  Short name of the final type of the gate, i.e., 'And'.
#define TEST_ADD_COMPLEMENT_ARG_KN(num_vars, v_num, final_type) \
  TEST_F(GateTest, ComplementArgVoteTo##final_type) {           \
    DefineGate(kVote, num_vars);                                \
    g->vote_number(v_num);                                      \
    g->AddArg(var_one, true);                                   \
    ASSERT_FALSE(g->constant());                                \
    EXPECT_EQ(k##final_type, g->type());                        \
    EXPECT_EQ(num_vars - 1, g->args().size());                  \
    EXPECT_EQ(num_vars - 1, g->args<Variable>().size());        \
    EXPECT_EQ(v_num - 1, g->vote_number());                     \
    EXPECT_TRUE(g->args<Gate>().empty());                       \
  }

TEST_ADD_COMPLEMENT_ARG_KN(2, 2, Null)  // Join operation.
TEST_ADD_COMPLEMENT_ARG_KN(3, 2, Or)  // General case.
TEST_ADD_COMPLEMENT_ARG_KN(3, 3, And)  // Join operation.

#undef TEST_ADD_COMPLEMENT_ARG_KN

/// Tests for processing of a constant argument of a gate,
/// which results in gate becoming constant itself.
///
/// @param arg_state  The true or false state of the gate argument.
/// @param num_vars  The initial number of gate arguments.
/// @param init_type  The initial type of the gate.
/// @param const_state  The notion of Boolean constant in the gate (TRUE/FALSE).
#define TEST_CONSTANT_ARG_STATE(arg_state, num_vars, init_type, const_state) \
  TEST_F(GateTest, arg_state##ConstantArg##init_type) {                      \
    DefineGate(k##init_type, num_vars);                                      \
    g->ProcessConstantArg(var_one, arg_state);                               \
    ASSERT_TRUE(g->constant());                                              \
    ASSERT_EQ(1, g->args().size());                                          \
    ASSERT_##const_state(*g->args().begin() > 0);                            \
    EXPECT_TRUE(g->args<Variable>().empty());                                \
    EXPECT_TRUE(g->args<Gate>().empty());                                    \
  }

TEST_CONSTANT_ARG_STATE(true, 1, Null, TRUE)
TEST_CONSTANT_ARG_STATE(false, 1, Null, FALSE)
TEST_CONSTANT_ARG_STATE(false, 1, Not, TRUE)
TEST_CONSTANT_ARG_STATE(true, 1, Not, FALSE)
TEST_CONSTANT_ARG_STATE(true, 2, Or, TRUE)
TEST_CONSTANT_ARG_STATE(false, 2, And, FALSE)
TEST_CONSTANT_ARG_STATE(true, 2, Nor, FALSE)
TEST_CONSTANT_ARG_STATE(false, 2, Nand, TRUE)

#undef TEST_CONSTANT_ARG_STATE

/// Tests for processing of a constant argument of a gate,
/// which results in type change of the gate.
///
/// @param arg_state  The true or false state of the gate argument.
/// @param num_vars  The initial number of gate arguments.
/// @param v_num  The initial vote number of the gate.
/// @param init_type  The initial type of the gate.
/// @param final_type  The final type of the gate.
#define TEST_CONSTANT_ARG_VNUM(arg_state, num_vars, v_num, init_type,   \
                               final_type)                              \
  TEST_F(GateTest, arg_state##ConstantArg##init_type##To##final_type) { \
    DefineGate(k##init_type, num_vars);                                 \
    if (v_num)                                                          \
      g->vote_number(v_num);                                            \
    g->ProcessConstantArg(var_one, arg_state);                          \
    ASSERT_FALSE(g->constant());                                        \
    EXPECT_EQ(k##final_type, g->type());                                \
    EXPECT_EQ(num_vars - 1, g->args().size());                          \
    EXPECT_EQ(num_vars - 1, g->args<Variable>().size());                \
    EXPECT_TRUE(g->args<Gate>().empty());                               \
  }

TEST_CONSTANT_ARG_VNUM(true, 3, 2, Vote, Or)
TEST_CONSTANT_ARG_VNUM(true, 4, 3, Vote, Vote)
TEST_CONSTANT_ARG_VNUM(false, 3, 2, Vote, And)
TEST_CONSTANT_ARG_VNUM(false, 4, 2, Vote, Vote)

/// The same tests as TEST_CONSTANT_ARG_VNUM
/// but with no vote number initialization.
#define TEST_CONSTANT_ARG(arg_state, num_vars, init_type, final_type) \
  TEST_CONSTANT_ARG_VNUM(arg_state, num_vars, false, init_type, final_type)

TEST_CONSTANT_ARG(false, 2, Or, Null)
TEST_CONSTANT_ARG(false, 3, Or, Or)
TEST_CONSTANT_ARG(true, 2, And, Null)
TEST_CONSTANT_ARG(true, 3, And, And)
TEST_CONSTANT_ARG(false, 2, Nor, Not)
TEST_CONSTANT_ARG(false, 3, Nor, Nor)
TEST_CONSTANT_ARG(true, 2, Nand, Not)
TEST_CONSTANT_ARG(true, 3, Nand, Nand)
TEST_CONSTANT_ARG(true, 2, Xor, Not)
TEST_CONSTANT_ARG(false, 2, Xor, Null)

#undef TEST_CONSTANT_ARG_VNUM
#undef TEST_CONSTANT_ARG

}  // namespace test
}  // namespace core
}  // namespace scram
