/*
 * Copyright (C) 2015-2018 Olzhas Rakhimov
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

#include <catch.hpp>

#include "fault_tree.h"
#include "initializer.h"
#include "model.h"
#include "settings.h"

/// @todo: Replace w/ proper Catch macros.
#define ASSERT_TRUE REQUIRE
#define ASSERT_FALSE REQUIRE_FALSE

#define STR(arg) #arg  // Wrapper to use catenation.

namespace scram::core::test {

TEST_CASE("PdagTest.Print", "[mef::pdag]") {
  std::unique_ptr<mef::Initializer> init;
  REQUIRE_NOTHROW(init.reset(new mef::Initializer(
      {"tests/input/fta/correct_formulas.xml"}, Settings())));
  const mef::FaultTreePtr& ft = *init->model()->fault_trees().begin();
  Pdag graph(*ft->top_events().front());
  graph.Print();
}

TEST_CASE("PdagTest.Cardinality", "[mef::pdag]") {
  mef::BasicEvent one("one"), two("two");
  mef::Formula::ArgSet arg_set = {&one, &two};
  mef::Gate root("root");

  SECTION("Zero & zero") {
    root.formula(
        std::make_unique<mef::Formula>(mef::kCardinality, arg_set, 0, 2));
    Pdag pdag(root);
    pdag.RemoveNullGates();
    CHECK(pdag.IsTrivial());
    CHECK(pdag.root()->type() == kNull);
    CHECK(pdag.root()->constant());
    CHECK(pdag.root()->args().size() == 1);
    CHECK(*pdag.root()->args().begin() == 1);
  }
  SECTION("and & zero") {
    root.formula(
        std::make_unique<mef::Formula>(mef::kCardinality, arg_set, 2, 2));
    Pdag pdag(root);
    pdag.RemoveNullGates();
    CHECK(!pdag.IsTrivial());
    CHECK(pdag.root()->type() == kAnd);
    CHECK(pdag.root()->args().size() == 2);
    CHECK(pdag.root()->args().count(2));
    CHECK(pdag.root()->args().count(3));
  }
  SECTION("zero & and") {
    root.formula(
        std::make_unique<mef::Formula>(mef::kCardinality, arg_set, 0, 0));
    Pdag pdag(root);
    pdag.RemoveNullGates();
    CHECK(!pdag.IsTrivial());
    CHECK(pdag.root()->type() == kAnd);
    CHECK(pdag.root()->args().size() == 2);
    CHECK(pdag.root()->args().count(-2));
    CHECK(pdag.root()->args().count(-3));
  }
  SECTION("or & zero") {
    root.formula(
        std::make_unique<mef::Formula>(mef::kCardinality, arg_set, 1, 2));
    Pdag pdag(root);
    pdag.RemoveNullGates();
    CHECK(!pdag.IsTrivial());
    CHECK(pdag.root()->type() == kOr);
    CHECK(pdag.root()->args().size() == 2);
    CHECK(pdag.root()->args().count(2));
    CHECK(pdag.root()->args().count(3));
  }
  SECTION("zero & or") {
    root.formula(
        std::make_unique<mef::Formula>(mef::kCardinality, arg_set, 0, 1));
    Pdag pdag(root);
    pdag.RemoveNullGates();
    CHECK(!pdag.IsTrivial());
    CHECK(pdag.root()->type() == kOr);
    CHECK(pdag.root()->args().size() == 2);
    CHECK(pdag.root()->args().count(-2));
    CHECK(pdag.root()->args().count(-3));
  }
}

static_assert(kNumConnectives == 8, "New gate types are not considered!");

class GateTest {
 public:
  GateTest() {
    var_one = std::make_shared<Variable>(&graph);
    var_two = std::make_shared<Variable>(&graph);
    var_three = std::make_shared<Variable>(&graph);
    vars_ = {var_one, var_two, var_three};
    for (int i = 0; i < 2; ++i)
      vars_.emplace_back(new Variable(&graph));  // Extra.
  }

 protected:
  /// Sets up the main gate with the default variables.
  ///
  /// @param[in] type  Type for the main gate.
  /// @param[in] num_vars  Desired number of variables.
  ///
  /// @note The setup is not for one-arg gates (NOT/NULL).
  /// @note For K/N gates, K is set to 2 by default.
  void DefineGate(Connective type, int num_vars) {
    assert(num_vars < 6);
    assert(!(type == kAtleast && num_vars < 2));

    g = std::make_shared<Gate>(type, &graph);
    if (type == kAtleast)
      g->min_number(2);
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

/// Collection of tests
/// for addition of an existing argument to a gate.
///
/// @param short_type  Short name of the gate, i.e., 'And'.
///                    It must have the same root in Connective, i.e., 'kAnd'.
/// @param num_vars  The number of variables to initialize the gate.
#define ADD_ARG_IGNORE_TEST(short_type, num_vars) \
  DefineGate(k##short_type, num_vars);            \
  g->AddArg(var_one);                             \
  REQUIRE_FALSE(g->constant());                   \
  CHECK(g->args().size() == num_vars);            \
  CHECK(g->args<Variable>().size() == num_vars);  \
  CHECK(g->args<Gate>().empty())

/// Tests addition of an existing argument to PDAG gates
/// that do not change the type of the gate.
///
/// @param short_type  Short name of the gate type, i.e., 'And'.
#define TEST_DUP_ARG_IGNORE(short_type)                                   \
  TEST_CASE_METHOD(GateTest, "pdag." STR(DuplicateArgIgnore##short_type), \
                   "[pdag]") {                                            \
    ADD_ARG_IGNORE_TEST(short_type, 2);                                   \
    CHECK(g->type() == k##short_type);                                    \
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
#define TEST_DUP_ARG_TYPE_CHANGE(init_type, final_type)                        \
  TEST_CASE_METHOD(GateTest, "pdag." STR(DuplicateArgChange##init_type##Type), \
                   "[pdag]") {                                                 \
    ADD_ARG_IGNORE_TEST(init_type, 1);                                         \
    CHECK(g->type() == k##final_type);                                         \
  }

TEST_DUP_ARG_TYPE_CHANGE(Or, Null)
TEST_DUP_ARG_TYPE_CHANGE(And, Null)
TEST_DUP_ARG_TYPE_CHANGE(Nor, Not)
TEST_DUP_ARG_TYPE_CHANGE(Nand, Not)

#undef TEST_DUP_ARG_TYPE_CHANGE
#undef ADD_ARG_IGNORE_TEST

TEST_CASE_METHOD(GateTest, "pdag.DuplicateArgXor", "[pdag]") {
  DefineGate(kXor, 1);
  g->AddArg(var_one);
  REQUIRE(g->constant());
  REQUIRE(g->args().size() == 1);
  CHECK(*g->args().begin() == -1);
}

TEST_CASE_METHOD(GateTest, "pdag.DuplicateArgAtleastToNull", "[pdag]") {
  DefineGate(kAtleast, 2);
  g->AddArg(var_one);
  REQUIRE_FALSE(g->constant());
  CHECK(g->type() == kNull);
  CHECK(g->args().size() == 1);
  CHECK(g->args<Variable>().begin()->first == var_two->index());
}

TEST_CASE_METHOD(GateTest, "pdag.DuplicateArgAtleastToAnd", "[pdag]") {
  DefineGate(kAtleast, 3);
  g->min_number(3);  // K equals to the number of input arguments.
  g->AddArg(var_one);
  REQUIRE_FALSE(g->constant());
  CHECK(g->type() == kAnd);
  CHECK(g->args().size() == 2);
  REQUIRE(g->args<Variable>().size() == 1);
  CHECK(g->args<Variable>().begin()->first == var_one->index());
  REQUIRE(g->args<Gate>().size() == 1);

  GatePtr sub = g->args<Gate>().begin()->second;
  CHECK(sub->type() == kOr);  // Special case. K/N is in general.
  CHECK(sub->min_number() == 1);  // This is the reason.
  Gate::ArgSet vars;
  vars.insert(var_two->index());
  vars.insert(var_three->index());
  CHECK(sub->args() == vars);
  CHECK(sub->args<Variable>().size() == 2);
}

TEST_CASE_METHOD(GateTest, "pdag.DuplicateArgAtleastToOrWithOneClone",
                 "[pdag]") {
  DefineGate(kAtleast, 3);
  g->min_number(2);
  g->AddArg(var_one);
  REQUIRE_FALSE(g->constant());
  CHECK(g->type() == kOr);
  CHECK(g->args().size() == 2);
  REQUIRE(g->args<Variable>().size() == 1);
  CHECK(g->args<Variable>().begin()->first == var_one->index());
  REQUIRE(g->args<Gate>().size() == 1);

  GatePtr sub = g->args<Gate>().begin()->second;
  CHECK(sub->type() == kAnd);  // Special case. K/N is in general.
  CHECK(sub->min_number() == 2);
  CHECK(sub->args().size() == 2);  // This is the reason.
  Gate::ArgSet vars;
  vars.insert(var_two->index());
  vars.insert(var_three->index());
  CHECK(sub->args() == vars);
  CHECK(sub->args<Variable>().size() == 2);
}

TEST_CASE_METHOD(GateTest, "pdag.DuplicateArgAtleastToOrWithTwoClones",
                 "[pdag]") {
  DefineGate(kAtleast, 5);
  g->min_number(3);
  g->AddArg(var_one);
  REQUIRE_FALSE(g->constant());
  CHECK(g->type() == kOr);
  CHECK(g->args().size() == 2);
  CHECK(g->args<Variable>().empty());
  REQUIRE(g->args<Gate>().size() == 2);
  auto it = g->args<Gate>().begin();
  GatePtr and_gate = it->second;  // Guessing.
  ++it;
  GatePtr clone_one = it->second;  // Guessing.
  // Correcting the guess.
  if (and_gate->type() != kAnd)
    std::swap(and_gate, clone_one);
  REQUIRE(and_gate->type() == kAnd);
  REQUIRE(clone_one->type() == kAtleast);

  REQUIRE_FALSE(clone_one->constant());
  CHECK(clone_one->min_number() == 3);
  CHECK(clone_one->args().size() == 4);
  CHECK(clone_one->args<Variable>().size() == 4);

  REQUIRE_FALSE(and_gate->constant());
  CHECK(and_gate->args().size() == 2);
  REQUIRE(and_gate->args<Variable>().size() == 1);
  CHECK(and_gate->args<Variable>().begin()->first == var_one->index());
  REQUIRE(and_gate->args<Gate>().size() == 1);

  GatePtr clone_two = and_gate->args<Gate>().begin()->second;
  REQUIRE_FALSE(clone_two->constant());
  CHECK(clone_two->type() == kOr);  // Special case. K/N is in general.
  CHECK(clone_two->min_number() == 1);  // This is the reason.
  CHECK(clone_two->args().size() == 4);
  CHECK(clone_two->args<Variable>().size() == 4);
}

/// Collection of tests
/// for addition of the complement of an existing argument to a gate.
///
/// @param short_type  Short name of the gate, i.e., 'And'.
///                    It must have the same root in Connective, i.e., 'kAnd'.
/// @param const_state  The notion of Boolean constant in the gate (TRUE/FALSE).
#define TEST_ADD_COMPLEMENT_ARG(short_type, const_state)             \
  TEST_CASE_METHOD(GateTest, "pdag." STR(ComplementArg##short_type), \
                   "[pdag]") {                                       \
    DefineGate(k##short_type, 1);                                    \
    g->AddArg(var_one, true);                                        \
    REQUIRE(g->constant());                                          \
    REQUIRE(g->args().size() == 1);                                  \
    ASSERT_##const_state(*g->args().begin() > 0);                    \
    CHECK(g->args<Variable>().empty());                              \
    CHECK(g->args<Gate>().empty());                                  \
  }

TEST_ADD_COMPLEMENT_ARG(And, FALSE)
TEST_ADD_COMPLEMENT_ARG(Or, TRUE)
TEST_ADD_COMPLEMENT_ARG(Nand, TRUE)
TEST_ADD_COMPLEMENT_ARG(Nor, FALSE)
TEST_ADD_COMPLEMENT_ARG(Xor, TRUE)

#undef TEST_ADD_COMPLEMENT_ARG

/// Collection of ATLEAST (K/N) gate tests
/// for addition of the complement of an existing argument.
///
/// @param num_vars  Initial number of variables.
/// @param v_num  Initial K number of the gate.
/// @param final_type  Short name of the final type of the gate, i.e., 'And'.
#define TEST_ADD_COMPLEMENT_ARG_KN(num_vars, v_num, final_type)               \
  TEST_CASE_METHOD(GateTest, "pdag." STR(ComplementArgAtleastTo##final_type), \
                   "[pdag]") {                                                \
    DefineGate(kAtleast, num_vars);                                           \
    g->min_number(v_num);                                                     \
    g->AddArg(var_one, true);                                                 \
    REQUIRE_FALSE(g->constant());                                             \
    CHECK(g->type() == k##final_type);                                        \
    CHECK(g->args().size() == (num_vars - 1));                                \
    CHECK(g->args<Variable>().size() == (num_vars - 1));                      \
    CHECK(g->min_number() == (v_num - 1));                                    \
    CHECK(g->args<Gate>().empty());                                           \
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
  TEST_CASE_METHOD(GateTest, "pdag." STR(arg_state##ConstantArg##init_type), \
                   "[pdag]") {                                               \
    DefineGate(k##init_type, num_vars);                                      \
    g->ProcessConstantArg(var_one, arg_state);                               \
    REQUIRE(g->constant());                                                  \
    REQUIRE(g->args().size() == 1);                                          \
    ASSERT_##const_state(*g->args().begin() > 0);                            \
    CHECK(g->args<Variable>().empty());                                      \
    CHECK(g->args<Gate>().empty());                                          \
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
/// @param v_num  The initial min number of the gate.
/// @param init_type  The initial type of the gate.
/// @param final_type  The final type of the gate.
#define TEST_CONSTANT_ARG_VNUM(arg_state, num_vars, v_num, init_type, \
                               final_type)                            \
  TEST_CASE_METHOD(                                                   \
      GateTest,                                                       \
      "pdag." STR(arg_state##ConstantArg##init_type##To##final_type), \
      "[pdag]") {                                                     \
    DefineGate(k##init_type, num_vars);                               \
    if (v_num)                                                        \
      g->min_number(v_num);                                           \
    g->ProcessConstantArg(var_one, arg_state);                        \
    REQUIRE_FALSE(g->constant());                                     \
    CHECK(g->type() == k##final_type);                                \
    CHECK(g->args<Variable>().size() == (num_vars - 1));              \
    CHECK(g->args().size() == (num_vars - 1));                        \
    CHECK(g->args<Gate>().empty());                                   \
  }

TEST_CONSTANT_ARG_VNUM(true, 3, 2, Atleast, Or)
TEST_CONSTANT_ARG_VNUM(true, 4, 3, Atleast, Atleast)
TEST_CONSTANT_ARG_VNUM(false, 3, 2, Atleast, And)
TEST_CONSTANT_ARG_VNUM(false, 4, 2, Atleast, Atleast)

/// The same tests as TEST_CONSTANT_ARG_VNUM
/// but with no min number initialization.
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

}  // namespace scram::core::test
