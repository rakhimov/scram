/// @file indexed_fault_tree.cc
/// Implementation of IndexedFaultTree.
#include "indexed_fault_tree.h"

#include <set>
#include <utility>

#include <boost/assign.hpp>

#include "event.h"

namespace scram {

const std::map<std::string, GateType> IndexedFaultTree::kStringToType_ =
    boost::assign::map_list_of("and", kAndGate) ("or", kOrGate)
                              ("atleast", kAtleastGate) ("xor", kXorGate)
                              ("not", kNotGate) ("nand", kNandGate)
                              ("nor", kNorGate) ("null", kNullGate);

IndexedFaultTree::IndexedFaultTree(int top_event_id)
    : top_event_index_(top_event_id),
      kGateIndex_(top_event_id),
      new_gate_index_(0) {}

void IndexedFaultTree::InitiateIndexedFaultTree(
    const boost::unordered_map<int, GatePtr>& int_to_inter,
    const std::map<std::string, int>& ccf_basic_to_gates,
    const boost::unordered_map<std::string, int>& all_to_int) {
  // Assume that new ccf_gates are not re-added into general index container.
  new_gate_index_ = all_to_int.size() + ccf_basic_to_gates.size() + 1;

  boost::unordered_map<int, GatePtr>::const_iterator it;
  for (it = int_to_inter.begin(); it != int_to_inter.end(); ++it) {
    IndexedFaultTree::ProcessFormula(it->first, it->second->formula(),
                                     ccf_basic_to_gates, all_to_int);
  }
}

void IndexedFaultTree::ProcessFormula(
    int index,
    const FormulaPtr& formula,
    const std::map<std::string, int>& ccf_basic_to_gates,
    const boost::unordered_map<std::string, int>& all_to_int) {
  assert(!indexed_gates_.count(index));
  GateType type = kStringToType_.find(formula->type())->second;
  IndexedGatePtr gate(new IndexedGate(index, type));
  if (type == kAtleastGate) gate->vote_number(formula->vote_number());

  typedef boost::shared_ptr<Event> EventPtr;

  const std::map<std::string, EventPtr>* children = &formula->event_args();
  std::map<std::string, EventPtr>::const_iterator it_children;
  for (it_children = children->begin(); it_children != children->end();
       ++it_children) {
    int child_index = all_to_int.find(it_children->first)->second;
    // Replace CCF basic events with the corresponding events.
    if (ccf_basic_to_gates.count(it_children->first))
      child_index = ccf_basic_to_gates.find(it_children->first)->second;
    gate->InitiateWithChild(child_index);
  }
  const std::set<FormulaPtr>* formulas = &formula->formula_args();
  std::set<FormulaPtr>::const_iterator it_f;
  for (it_f = formulas->begin(); it_f != formulas->end(); ++it_f) {
    int child_index = ++new_gate_index_;
    IndexedFaultTree::ProcessFormula(child_index, *it_f, ccf_basic_to_gates,
                                     all_to_int);
    gate->InitiateWithChild(child_index);
  }
  IndexedFaultTree::AddGate(gate);
}

}  // namespace scram
