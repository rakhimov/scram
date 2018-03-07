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

/// @file
/// Implementation of cycle detection specializations.

#include "cycle.h"

namespace scram::mef::cycle {

template <>
bool ContinueConnector(Branch* connector, std::vector<NamedBranch*>* cycle) {
  struct {
    bool operator()(NamedBranch* branch) { return DetectCycle(branch, cycle_); }

    bool operator()(Fork* fork) {
      for (Branch& branch : fork->paths()) {
        if (ContinueConnector(&branch, cycle_))
          return true;
      }
      return false;
    }

    bool operator()(Sequence*) { return false; }

    decltype(cycle) cycle_;
  } continue_connector{cycle};

  return std::visit(continue_connector, connector->target());
}

template <>
bool ContinueConnector(const Instruction* connector,
                       std::vector<Rule*>* cycle) {
  struct Visitor : public InstructionVisitor {
    struct ArgSelector : public InstructionVisitor {
      explicit ArgSelector(Visitor* visitor) : visitor_(visitor) {}

      void Visit(const SetHouseEvent*) override {}
      void Visit(const CollectExpression*) override {}
      void Visit(const CollectFormula*) override {}
      void Visit(const Link*) override {}
      void Visit(const IfThenElse* ite) override { visitor_->Visit(ite); }
      void Visit(const Block* block) override { visitor_->Visit(block); }
      void Visit(const Rule* rule) override {
        // Non-const rules are only needed to mark the nodes.
        if (DetectCycle(const_cast<Rule*>(rule), visitor_->cycle_))
          throw true;
      }

      Visitor* visitor_;
    };

    explicit Visitor(std::vector<Rule*>* t_cycle)
        : cycle_(t_cycle), selector_(this) {}

    void Visit(const SetHouseEvent*) override {}
    void Visit(const CollectExpression*) override {}
    void Visit(const CollectFormula*) override {}
    void Visit(const Link*) override {}
    void Visit(const IfThenElse* ite) override {
      ite->then_instruction()->Accept(&selector_);
      if (ite->else_instruction())
        ite->else_instruction()->Accept(&selector_);
    }
    void Visit(const Block* block) override {
      for (const Instruction* instruction : block->instructions())
        instruction->Accept(&selector_);
    }
    void Visit(const Rule* rule) override {
      for (const Instruction* instruction : rule->instructions())
        instruction->Accept(&selector_);
    }
    std::vector<Rule*>* cycle_;
    ArgSelector selector_;
  } visitor(cycle);

  try {
    connector->Accept(&visitor);
  } catch (bool& ret_val) {
    assert(ret_val && !cycle->empty());
    return true;
  }
  return false;
}

template <>
bool ContinueConnector(const EventTree* connector, std::vector<Link*>* cycle) {
  struct {
    void operator()(const Branch* branch) {
      std::visit(*this, branch->target());
    }
    void operator()(Fork* fork) {
      for (Branch& branch : fork->paths())
        (*this)(&branch);
    }
    void operator()(Sequence* sequence) {
      struct Visitor : public NullVisitor {
        explicit Visitor(decltype(cycle) t_cycle) : visitor_cycle_(t_cycle) {}

        void Visit(const Link* link) override {
          if (DetectCycle(const_cast<Link*>(link), visitor_cycle_))
            throw true;
        }

        decltype(cycle) visitor_cycle_;
      } visitor(cycle_);

      for (const Instruction* instruction : sequence->instructions())
        instruction->Accept(&visitor);
    }

    decltype(cycle) cycle_;
  } continue_connector{cycle};

  try {
    continue_connector(&connector->initial_state());
  } catch (bool& ret_val) {
    assert(ret_val && !cycle->empty());
    return true;
  }
  return false;
}

}  // namespace scram::mef::cycle
