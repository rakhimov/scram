/// @file cycle.h
/// Facilities to detect and print cycles.
#ifndef SCRAM_SRC_CYCLE_H_
#define SCRAM_SRC_CYCLE_H_

#include <assert.h>

#include <string>
#include <vector>

#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>

namespace scram {
namespace cycle {

template<class N, class C>
bool ContinueConnector(C* connector, std::vector<std::string>* cycle);

/// Traverses nodes with connectors to find a cycle.
/// Interrupts the detection at first cycle. Nodes get marked.
/// @param[in,out] node The node to start with.
/// @param[out] cycle If a cycle is detected, it is given in reverse,
///                   ending with the input node's original name.
///                   This is for printing errors and efficiency.
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
/// @param[in,out] connector Connector to nodes.
/// @param[out] cycle The cycle path if detected.
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

/// @param[in] cycle Cycle containing names in reverse order.
/// @returns String representation of the cycle.
std::string PrintCycle(const std::vector<std::string>& cycle);

}  // namespace cycle
}  // namespace scram

#endif  // SCRAM_SRC_CYCLE_H_
