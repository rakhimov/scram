/*
 * Copyright (C) 2014-2017 Olzhas Rakhimov
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

/// @file cycle.h
/// Validation facilities to detect and print cycles in graphs.

#ifndef SCRAM_SRC_CYCLE_H_
#define SCRAM_SRC_CYCLE_H_

#include <string>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "element.h"
#include "error.h"
#include "event.h"
#include "event_tree.h"
#include "parameter.h"

namespace scram {
namespace mef {
namespace cycle {

/// Determines the connectors between nodes.
///
/// @param[in] node  The node under cycle investigation.
///
/// @returns The connector belonging to the node.
///
/// @{
inline const Formula* GetConnector(Gate* node) { return &node->formula(); }
inline Expression* GetConnector(Parameter* node) { return node; }
inline Branch* GetConnector(NamedBranch* node) { return node; }
inline const Instruction* GetConnector(Rule* node) { return node; }
inline const EventTree* GetConnector(Link* node) { return &node->event_tree(); }
/// @}

/// Retrieves nodes from a connector.
///
/// @param[in] connector  The connector starting from another node.
///
/// @returns  The iterable collection of nodes on the other end of connection.
///
/// @{
inline auto GetNodes(const Formula* connector) {
  return connector->event_args() |
         boost::adaptors::transformed(
             [](const Formula::EventArg& event_args) -> Gate* {
               if (auto* arg = boost::get<Gate*>(&event_args))
                 return *arg;
               return nullptr;
             }) |
         boost::adaptors::filtered([](auto* ptr) { return ptr != nullptr; });
}
inline auto GetNodes(Expression* connector) {
  return connector->args() | boost::adaptors::transformed([](Expression* arg) {
           return dynamic_cast<Parameter*>(arg);
         }) |
         boost::adaptors::filtered([](auto* ptr) { return ptr != nullptr; });
}
/// @}

/// Retrieves connectors from a connector.
///
/// @param[in] connector  The connector starting from another node.
///
/// @returns  The iterable collection of connectors.
///
/// @{
inline auto GetConnectors(const Formula* connector) {
  return connector->formula_args() |
         boost::adaptors::transformed(
             [](const FormulaPtr& ptr) { return ptr.get(); });
}
inline auto GetConnectors(Expression* connector) {
  return connector->args() | boost::adaptors::filtered([](Expression* arg) {
           return dynamic_cast<Parameter*>(arg) == nullptr;
         });
}
/// @}

template <class T, class N>
bool ContinueConnector(T* connector, std::vector<N*>* cycle);

/// Traverses nodes with connectors to find a cycle.
/// Interrupts the detection at first cycle.
/// Nodes get marked.
///
/// The connector of the node is retrieved via unqualified call to
/// GetConnector(node).
///
/// @tparam T  The type of nodes in the graph.
///
/// @param[in,out] node  The node to start with.
/// @param[out] cycle  If a cycle is detected,
///                    it is given in reverse,
///                    ending with the cycle node.
///
/// @returns True if a cycle is found.
///
/// @post All traversed nodes are marked with non-clear marks.
template <class T>
bool DetectCycle(T* node, std::vector<T*>* cycle) {
  if (!node->mark()) {
    node->mark(NodeMark::kTemporary);
    if (ContinueConnector(GetConnector(node), cycle)) {
      if (cycle->size() == 1 || cycle->back() != cycle->front())
        cycle->push_back(node);
      return true;
    }
    node->mark(NodeMark::kPermanent);
  } else if (node->mark() == NodeMark::kTemporary) {
    assert(cycle->empty() && "The report container must be provided empty.");
    cycle->push_back(node);
    return true;
  }
  assert(node->mark() == NodeMark::kPermanent);
  return false;
}

/// Helper function to check for cyclic references through connectors.
/// Connectors may get market upon traversal.
///
/// Connectors and nodes of the connector are retrieved via unqualified calls:
/// GetConnectors(connector) and GetNodes(connector).
///
/// @tparam T  The type managing the connectors (nodes, edges).
/// @tparam N  The node type.
///
/// @param[in,out] connector  Connector to nodes.
/// @param[out] cycle  The cycle path if detected.
///
/// @returns True if a cycle is detected.
template <class T, class N>
bool ContinueConnector(T* connector, std::vector<N*>* cycle) {
  for (N* node : GetNodes(connector)) {
    if (DetectCycle(node, cycle))
      return true;
  }
  for (auto* link : GetConnectors(connector)) {
    if (ContinueConnector(link, cycle))
      return true;
  }
  return false;
}

/// Cycle detection specialization for event tree named branches.
template <>
inline bool ContinueConnector(Branch* connector,
                              std::vector<NamedBranch*>* cycle) {
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

  return boost::apply_visitor(continue_connector, connector->target());
}

/// Cycle detection specialization for visitor-based traversal of instructions.
template <>
inline bool ContinueConnector(const Instruction* connector,
                              std::vector<Rule*>* cycle) {
  struct Visitor : public InstructionVisitor {
    struct ArgSelector : public InstructionVisitor {
      explicit ArgSelector(Visitor* visitor) : visitor_(visitor) {}

      void Visit(const SetHouseEvent*) override {}
      void Visit(const CollectExpression*) override {}
      void Visit(const CollectFormula*) override {}
      void Visit(const Link*) override {}
      void Visit(const IfThenElse* ite) override {
        visitor_->Visit(ite);
      }
      void Visit(const Block* block) override {
        visitor_->Visit(block);
      }
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

/// Cycle detection specialization for visitor-based traversal of event-trees.
template <>
inline bool ContinueConnector(const EventTree* connector,
                              std::vector<Link*>* cycle) {
  struct {
    void operator()(const Branch* branch) {
      boost::apply_visitor(*this, branch->target());
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

/// Retrieves a unique name for a node.
template <class T>
const std::string& GetUniqueName(const T* node) {
  return Id::unique_name(*node);
}

/// Specialization for event-tree link name retrieval.
template <>
inline const std::string& GetUniqueName(const Link* node) {
  return node->event_tree().name();
}

/// Prints the detected cycle from the output
/// produced by cycle detection functions.
///
/// @tparam T  The node type with GetUniqueName(T*) defined.
///
/// @param[in] cycle  Cycle containing nodes in reverse order.
///
/// @returns String representation of the cycle.
template <class T>
std::string PrintCycle(const std::vector<T*>& cycle) {
  assert(cycle.size() > 1);
  assert(cycle.front() == cycle.back() && "No cycle is provided.");
  return boost::join(
      boost::adaptors::reverse(cycle) |
          boost::adaptors::transformed(
              [](T* node) -> decltype(auto) { return GetUniqueName(node); }),
      "->");
}

/// Checks for cycles in a model constructs.
///
/// @tparam T  The type of the node.
/// @tparam SinglePassRange  The range type with node pointers.
///
/// @param[in] container  The range with nodes to be tested.
/// @param[in] type  The type of nodes for error messages.
///
/// @throws CycleError  A cycle is detected in the graph of nodes.
template <class T, class SinglePassRange>
void CheckCycle(const SinglePassRange& container, const char* type) {
  std::vector<T*> cycle;
  for (const auto& node : container) {
    if (DetectCycle(&*node, &cycle)) {
      throw CycleError("Detected a cycle in " + GetUniqueName(&*node) + " " +
                       std::string(type) + ":\n" + PrintCycle(cycle));
    }
  }
}

}  // namespace cycle
}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_CYCLE_H_
