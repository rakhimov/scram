/*
 * Copyright (C) 2015 Olzhas Rakhimov
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

#include "boolean_graph.h"

#include <gtest/gtest.h>

#include "initializer.h"
#include "fault_tree.h"
#include "model.h"
#include "settings.h"

namespace scram {
namespace test {

TEST(BooleanGraphTest, Print) {
  Settings settings;
  Initializer* init = new Initializer(settings);
  std::vector<std::string> input_files;
  input_files.push_back("./share/scram/input/fta/correct_formulas.xml");
  EXPECT_NO_THROW(init->ProcessInputFiles(input_files));
  BooleanGraph* graph = new BooleanGraph(init->model()
                                         ->fault_trees().begin()->second
                                         ->top_events().front());
  graph->Print();
  delete init;
  delete graph;
}

using VariablePtr = std::shared_ptr<Variable>;
using IGatePtr = std::shared_ptr<IGate>;

static_assert(kNumOperators == 8, "New gate types are not considered!");

class IGateAddArgTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    var_one = std::make_shared<Variable>();
    var_two = std::make_shared<Variable>();
    var_three = std::make_shared<Variable>();
    vars_ = {var_one, var_two, var_three};
  }

  /// Sets up the main gate with the default variables.
  ///
  /// @param[in] type  Type for the main gate.
  /// @param[in] num_vars  Desired number of variables.
  ///
  /// @note The setup is not for one-arg gates (NOT/NULL).
  /// @note For K/N gates, K is set to 2 by default.
  void DefineGate(Operator type, int num_vars) {
    assert(type != kNotGate && type != kNullGate);
    assert(num_vars < 4);
    assert(!(type == kAtleastGate && num_vars < 3));

    g = std::make_shared<IGate>(type);
    if (type == kAtleastGate) g->vote_number(2);
    for (int i = 0; i < num_vars; ++i) g->AddArg(vars_[i]->index(), vars_[i]);

    assert(g->state() == kNormalState);
    assert(g->type() == type);
    assert(g->args().size() == num_vars);
    assert(g->variable_args().size() == num_vars);
    assert(g->gate_args().empty());
    assert(g->constant_args().empty());
  }

  virtual void TearDown() {
    Node::ResetIndex();
    Variable::ResetIndex();
  }

  IGatePtr g;  // Main gate for manipulations.
  // Collection of variables for gate input.
  VariablePtr var_one;
  VariablePtr var_two;
  VariablePtr var_three;

 private:
  std::vector<VariablePtr> vars_;  // For convenience only.
};

/// @def ADD_IGNORE_TEST(Type, num_vars)
///
/// Collection of tests
/// for addition of an existing argument to a gate.
///
/// @param Type  Short name of the gate, i.e., 'And'.
///              It must have the same root in Operator, i.e., 'kAndGate'.
/// @param num_vars  The number of variables to initialize the gate.
#define ADD_ARG_IGNORE_TEST(Type, num_vars)       \
  DefineGate(k##Type##Gate, num_vars);            \
  g->AddArg(var_one->index(), var_one);           \
  ASSERT_EQ(kNormalState, g->state());            \
  EXPECT_EQ(num_vars, g->args().size());          \
  EXPECT_EQ(num_vars, g->variable_args().size()); \
  EXPECT_TRUE(g->gate_args().empty());            \
  EXPECT_TRUE(g->constant_args().empty())

/// @def TEST_DUP_ARG_IGNORE(Type)
///
/// Tests addition of an existing argument to Boolean graph gates
/// that do not change the type of the gate.
///
/// @param Type  Short name of the gate type, i.e., 'And'.
#define TEST_DUP_ARG_IGNORE(Type)                     \
  TEST_F(IGateAddArgTest, DuplicateArgIgnore##Type) { \
    ADD_ARG_IGNORE_TEST(Type, 2);                     \
    EXPECT_EQ(k##Type##Gate, g->type());              \
  }

TEST_DUP_ARG_IGNORE(And);
TEST_DUP_ARG_IGNORE(Or);
TEST_DUP_ARG_IGNORE(Nand);
TEST_DUP_ARG_IGNORE(Nor);

#undef TEST_DUP_ARG_IGNORE

/// @def TEST_DUP_ARG_TYPE_CHANGE(Type)
///
/// Tests duplication addition that changes the type of the gate.
///
/// @param InitType  Short name of the initial type of the gate, i.e., 'And'.
/// @param FinalType  The resulting type of addition operation.
#define TEST_DUP_ARG_TYPE_CHANGE(InitType, FinalType)           \
  TEST_F(IGateAddArgTest, DuplicateArgChange##InitType##Type) { \
    ADD_ARG_IGNORE_TEST(InitType, 1);                           \
    EXPECT_EQ(k##FinalType##Gate, g->type());                   \
  }

TEST_DUP_ARG_TYPE_CHANGE(Or, Null);
TEST_DUP_ARG_TYPE_CHANGE(And, Null);
TEST_DUP_ARG_TYPE_CHANGE(Nor, Not);
TEST_DUP_ARG_TYPE_CHANGE(Nand, Not);

#undef TEST_DUP_ARG_TYPE_CHANGE

TEST_F(IGateAddArgTest, DuplicateArgXor) {
  DefineGate(kXorGate, 1);
  g->AddArg(var_one->index(), var_one);
  EXPECT_EQ(kNullState, g->state());
  EXPECT_TRUE(g->args().empty());
}

}  // namespace test
}  // namespace scram
