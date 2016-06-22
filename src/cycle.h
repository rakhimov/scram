/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

#include <cassert>

#include <string>
#include <vector>

#include "event.h"
#include "expression.h"

namespace scram {
namespace cycle {

/// Determines the connectors between nodes.
///
/// @param[in] node  The node under cycle investigation.
///
/// @returns The connector belonging to the node.
///
/// @{
inline const mef::Formula* GetConnector(const mef::GatePtr& node) {
  return &node->formula();
}
inline mef::Expression* GetConnector(mef::Parameter* node) { return node; }
/// @}

/// Retrieves nodes from a connector.
///
/// @param[in] connector  The connector starting from another node.
///
/// @returns  The iterable collection of nodes on the other end of connection.
///
/// @{
inline
const std::vector<mef::GatePtr>& GetNodes(const mef::FormulaPtr& connector) {
  return connector->gate_args();
}
inline
const std::vector<mef::GatePtr>& GetNodes(const mef::Formula* connector) {
  return connector->gate_args();
}
inline std::vector<mef::Parameter*> GetNodes(mef::Expression* connector) {
  std::vector<mef::Parameter*> nodes;
  for (const mef::ExpressionPtr& arg : connector->args()) {
    mef::Parameter* ptr = dynamic_cast<mef::Parameter*>(arg.get());
    if (ptr) nodes.push_back(ptr);
  }
  return nodes;
}
/// @}

/// Retrieves connectors from a connector.
///
/// @param[in] connector  The connector starting from another node.
///
/// @returns  The iterable collection of connectors.
///
/// @{
inline const std::vector<mef::FormulaPtr>&
GetConnectors(const mef::FormulaPtr& connector) {
  return connector->formula_args();
}
inline const std::vector<mef::FormulaPtr>&
GetConnectors(const mef::Formula* connector) {
  return connector->formula_args();
}
inline std::vector<mef::Expression*> GetConnectors(mef::Expression* connector) {
  std::vector<mef::Expression*> connectors;
  for (const mef::ExpressionPtr& arg : connector->args()) {
    if (dynamic_cast<mef::Parameter*>(arg.get()) == nullptr)
      connectors.push_back(arg.get());
  }
  return connectors;
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
template <class Ptr>
bool DetectCycle(const Ptr& node, std::vector<std::string>* cycle) {
  if (node->mark().empty()) {
    node->mark("temporary");
    if (ContinueConnector(GetConnector(node), cycle)) {
      cycle->push_back(node->name());
      return true;
    }
    node->mark("permanent");
  } else if (node->mark() == "temporary") {
    cycle->push_back(node->name());
    return true;
  }
  assert(node->mark() == "permanent");
  return false;  // This also covers permanently marked gates.
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
    if (DetectCycle(node, cycle)) return true;
  }
  for (const auto& link : GetConnectors(connector)) {
    if (ContinueConnector(link, cycle)) return true;
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
}  // namespace scram

#endif  // SCRAM_SRC_CYCLE_H_
