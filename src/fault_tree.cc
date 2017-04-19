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

/// @file fault_tree.cc
/// Implementation of fault tree and component containers.

#include "fault_tree.h"

#include "error.h"

namespace scram {
namespace mef {

Component::Component(std::string name, std::string base_path,
                     RoleSpecifier role)
    : Element(std::move(name)),
      Role(role, std::move(base_path)) {}

void Component::Add(const GatePtr& gate) {
  AddEvent(gate, &gates_);
}

void Component::Add(const BasicEventPtr& basic_event) {
  AddEvent(basic_event, &basic_events_);
}

void Component::Add(const HouseEventPtr& house_event) {
  AddEvent(house_event, &house_events_);
}

void Component::Add(const ParameterPtr& parameter) {
  mef::AddElement<ValidationError>(parameter, &parameters_,
                                   "Duplicate parameter: ");
}

void Component::Add(const CcfGroupPtr& ccf_group) {
  if (ccf_groups_.count(ccf_group->name())) {
    throw ValidationError("Duplicate CCF group " + ccf_group->name());
  }
  for (const BasicEventPtr& member : ccf_group->members()) {
    const std::string& name = member->name();
    if (gates_.count(name) || basic_events_.count(name) ||
        house_events_.count(name)) {
      throw ValidationError("Duplicate event " + name +
                            " from CCF group " + ccf_group->name());
    }
  }
  for (const auto& member : ccf_group->members())
    basic_events_.insert(member);
  ccf_groups_.insert(ccf_group);
}

void Component::Add(std::unique_ptr<Component> component) {
  if (components_.count(component->name())) {
    throw ValidationError("Duplicate component " + component->name());
  }
  components_.insert(std::move(component));
}

void Component::GatherGates(std::unordered_set<Gate*>* gates) {
  for (const GatePtr& gate : gates_)
    gates->insert(gate.get());

  for (const ComponentPtr& component : components_)
    component->GatherGates(gates);
}

template <class Ptr, class Container>
void Component::AddEvent(const Ptr& event, Container* container) {
  const std::string& name = event->name();
  if (gates_.count(name) || basic_events_.count(name) ||
      house_events_.count(name)) {
    throw ValidationError("Duplicate event " + name);
  }
  container->insert(event);
}

FaultTree::FaultTree(const std::string& name) : Component(name) {}

void FaultTree::CollectTopEvents() {
  top_events_.clear();
  std::unordered_set<Gate*> gates;
  Component::GatherGates(&gates);
  // Detects top events.
  for (Gate* gate : gates)
    MarkNonTopGates(gate, gates);

  for (Gate* gate : gates) {
    if (gate->mark()) {  // Not a top event.
      gate->mark(NodeMark::kClear);  // Cleaning up.
    } else {
      top_events_.push_back(gate);
    }
  }
}

void FaultTree::MarkNonTopGates(Gate* gate,
                                const std::unordered_set<Gate*>& gates) {
  if (gate->mark())
    return;
  MarkNonTopGates(gate->formula(), gates);
}

void FaultTree::MarkNonTopGates(const Formula& formula,
                                const std::unordered_set<Gate*>& gates) {
  for (const Formula::EventArg& event_arg : formula.event_args()) {
    if (auto* gate = boost::get<Gate*>(&event_arg)) {
      if (gates.count(*gate)) {
        MarkNonTopGates(*gate, gates);
        (*gate)->mark(NodeMark::kPermanent);  // Any non clear mark can be used.
      }
    }
  }
  for (const FormulaPtr& arg : formula.formula_args()) {
    MarkNonTopGates(*arg, gates);
  }
}

}  // namespace mef
}  // namespace scram
