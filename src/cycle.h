/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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
/// Facilities to detect and print cycles.

#ifndef SCRAM_SRC_CYCLE_H_
#define SCRAM_SRC_CYCLE_H_

#include <cassert>
#include <string>
#include <vector>

namespace scram {
namespace cycle {

template<class N, class C>
bool ContinueConnector(C* connector, std::vector<std::string>* cycle);

/// Traverses nodes with connectors to find a cycle.
/// Interrupts the detection at first cycle.
/// Nodes get marked.
///
/// @param[in,out] node The node to start with.
/// @param[out] cycle If a cycle is detected,
///                   it is given in reverse,
///                   ending with the input node's original name.
///                   This is for printing errors and efficiency.
///
/// @returns True if a cycle is found.
template<class N, class C>
bool DetectCycle(N* node, std::vector<std::string>* cycle) {
  if (node->mark() == "") {
    node->mark("temporary");
    if (ContinueConnector<N, C>(node->connector(), cycle)) {
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
/// Connecters may get market upon traversal.
///
/// @param[in,out] connector Connector to nodes.
/// @param[out] cycle The cycle path if detected.
///
/// @returns True if a cycle is detected.
template<class N, class C>
bool ContinueConnector(C* connector, std::vector<std::string>* cycle) {
  typename std::vector<N*>::const_iterator it;
  for (it = connector->nodes().begin(); it != connector->nodes().end(); ++it) {
    if (DetectCycle<N, C>(*it, cycle)) return true;
  }
  typename std::vector<C*>::const_iterator it_c;
  for (it_c = connector->connectors().begin();
       it_c != connector->connectors().end(); ++it_c) {
    if (ContinueConnector<N, C>(*it_c, cycle)) return true;
  }
  return false;
}

/// Prints the detected cycle from the output
/// produced by cycle detection functions.
///
/// @param[in] cycle Cycle containing names in reverse order.
///
/// @returns String representation of the cycle.
std::string PrintCycle(const std::vector<std::string>& cycle);

}  // namespace cycle
}  // namespace scram

#endif  // SCRAM_SRC_CYCLE_H_
