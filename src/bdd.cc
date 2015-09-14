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

/// @file bdd.cc
/// Implementation of BDD fault tree analysis algorithms.

#include "bdd.h"

#include <algorithm>

namespace scram {

Vertex::~Vertex() {}  // Empty body for pure virtual destructor.

Terminal::Terminal(bool value) : value_(value) {}

Ite::Ite(int index) : index_(index) {}

Bdd::Bdd(BooleanGraph* fault_tree)
    : fault_tree_(fault_tree),
      kOne_(std::make_shared<Terminal>(true)),
      kZero_(std::make_shared<Terminal>(false)) {}

void Bdd::Analyze() noexcept {
  assert(fault_tree_->coherent());
  fault_tree_->ClearOptiValues();
  Bdd::TopologicalOrder(fault_tree_->root(), 0);
  fault_tree_->ClearGateMarks();
  Bdd::ConvertGates(fault_tree_->root());
}

int Bdd::TopologicalOrder(const IGatePtr& root, int order) noexcept {
  if (root->opti_value()) return order;
  for (const std::pair<int, IGatePtr>& arg : root->gate_args()) {
    order = Bdd::TopologicalOrder(arg.second, order);
  }
  using VariablePtr = std::shared_ptr<Variable>;
  for (const std::pair<int, VariablePtr>& arg : root->variable_args()) {
    if (!arg.second->opti_value()) arg.second->opti_value(++order);
  }
  assert(root->constant_args().empty());
  root->opti_value(++order);
  return order;
}

void Bdd::ConvertGates(const IGatePtr& root) noexcept {
  if (root->mark()) return;
  root->mark(true);
  Bdd::ProcessGate(root);
  for (const std::pair<int, IGatePtr>& arg : root->gate_args()) {
    Bdd::ConvertGates(arg.second);
  }
}

void Bdd::ProcessGate(const IGatePtr& gate) noexcept {
  std::vector<std::pair<int, NodePtr>> args;
  args.insert(args.end(), gate->gate_args().begin(), gate->gate_args().end());
  args.insert(args.end(), gate->variable_args().begin(),
              gate->variable_args().end());
  std::sort(args.begin(), args.end(),
            [](const std::pair<int, NodePtr>& lhs,
               const std::pair<int, NodePtr>& rhs) {
              return lhs.second->opti_value() > rhs.second->opti_value();
            });
  gates_.emplace(gate->index(), Bdd::IfThenElse(gate->type(), args, 0));
}

std::shared_ptr<Ite> Bdd::IfThenElse(
    Operator type,
    const std::vector<std::pair<int, NodePtr>>& args,
    int pos) noexcept {
  assert(pos >= 0);
  assert(pos < args.size());
  const auto& arg = args[pos];
  ++pos;  // Next argument for subgraphs.
  assert(arg.first > 0);
  ItePtr ite(new Ite(arg.second->index()));
  switch (type) {
    case kOrGate:
      ite->high(kOne_);
      if (pos == args.size()) {
        ite->low(kZero_);
      } else {
        ite->low(Bdd::IfThenElse(type, args, pos));
      }
      break;
    case kAndGate:
      if (pos == args.size()) {
        ite->high(kOne_);
      } else {
        ite->high(Bdd::IfThenElse(type, args, pos));
      }
      ite->low(kZero_);
      break;
    default:
      assert(false);
  }
  return ite;
}

}  // namespace scram
