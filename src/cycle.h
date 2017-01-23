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

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "event.h"
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
inline const Formula* GetConnector(const GatePtr& node) {
  return &node->formula();
}
inline Expression* GetConnector(Parameter* node) { return node; }
/// @}

/// Retrieves nodes from a connector.
///
/// @param[in] connector  The connector starting from another node.
///
/// @returns  The iterable collection of nodes on the other end of connection.
///
/// @{
inline const std::vector<GatePtr>& GetNodes(const FormulaPtr& connector) {
  return connector->gate_args();
}
inline const std::vector<GatePtr>& GetNodes(const Formula* connector) {
  return connector->gate_args();
}
inline auto GetNodes(Expression* connector) {
  return connector->args() |
         boost::adaptors::transformed([](const ExpressionPtr& arg) {
           return dynamic_cast<Parameter*>(arg.get());
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
inline
const std::vector<FormulaPtr>& GetConnectors(const FormulaPtr& connector) {
  return connector->formula_args();
}
inline const std::vector<FormulaPtr>& GetConnectors(const Formula* connector) {
  return connector->formula_args();
}
inline auto GetConnectors(Expression* connector) {
  return connector->args() |
         boost::adaptors::filtered([](const ExpressionPtr& arg) {
           return dynamic_cast<Parameter*>(arg.get()) == nullptr;
         }) |
         boost::adaptors::transformed(
             [](const ExpressionPtr& arg) { return arg.get(); });
}
/// @}

template <class Ptr>
bool ContinueConnector(const Ptr& connector, std::vector<std::string>* cycle);

/// Traverses nodes with connectors to find a cycle.
/// Interrupts the detection at first cycle.
/// Nodes get marked.
///
/// The connector of the node is retrieved via unqualified call to
/// GetConnector(node).
///
/// @tparam Ptr  The pointer type managing nodes in the graph.
///
/// @param[in,out] node  The node to start with.
/// @param[out] cycle  If a cycle is detected,
///                    it is given in reverse,
///                    ending with the input node's original name.
///                    This is for printing errors and efficiency.
///
/// @returns True if a cycle is found.
///
/// @post All traversed nodes are marked with non-clear marks.
template <class Ptr>
bool DetectCycle(const Ptr& node, std::vector<std::string>* cycle) {
  if (!node->mark()) {
    node->mark(NodeMark::kTemporary);
    if (ContinueConnector(GetConnector(node), cycle)) {
      cycle->push_back(node->name());
      return true;
    }
    node->mark(NodeMark::kPermanent);
  } else if (node->mark() == NodeMark::kTemporary) {
    cycle->push_back(node->name());
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
/// @tparam Ptr  The pointer type managing the connectors (nodes, edges).
///
/// @param[in,out] connector  Connector to nodes.
/// @param[out] cycle  The cycle path if detected.
///
/// @returns True if a cycle is detected.
template <class Ptr>
bool ContinueConnector(const Ptr& connector, std::vector<std::string>* cycle) {
  for (const auto& node : GetNodes(connector)) {
    if (DetectCycle(node, cycle))
      return true;
  }
  for (const auto& link : GetConnectors(connector)) {
    if (ContinueConnector(link, cycle))
      return true;
  }
  return false;
}

/// Prints the detected cycle from the output
/// produced by cycle detection functions.
///
/// @param[in] cycle  Cycle containing names in reverse order.
///
/// @returns String representation of the cycle.
std::string PrintCycle(const std::vector<std::string>& cycle);

}  // namespace cycle
}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_CYCLE_H_
