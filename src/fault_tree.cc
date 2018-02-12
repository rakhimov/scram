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
/// Implementation of fault tree and component containers.

#include "fault_tree.h"

#include "error.h"

namespace scram::mef {

Component::Component(std::string name, std::string base_path,
                     RoleSpecifier role)
    : Element(std::move(name)), Role(role, std::move(base_path)) {}

void Component::Add(CcfGroup* ccf_group) {
  if (ccf_groups().count(ccf_group->name())) {
    SCRAM_THROW(DuplicateElementError())
        << errinfo_element(ccf_group->name(), "CCF group");
  }
  for (BasicEvent* member : ccf_group->members())
    CheckDuplicateEvent(*member);

  for (auto* member : ccf_group->members())
    Composite::Add(member);

  Composite::Add(ccf_group);
}

void Component::GatherGates(std::unordered_set<Gate*>* gates) {
  gates->insert(data<Gate>().begin(), data<Gate>().end());
  for (Component& component : table<Component>())
    component.GatherGates(gates);
}

void Component::CheckDuplicateEvent(const Event& event) {
  const std::string& name = event.name();
  if (gates().count(name) || basic_events().count(name) ||
      house_events().count(name)) {
    SCRAM_THROW(DuplicateElementError())
        << errinfo_element(name, "event")
        << errinfo_container(Element::name(), kTypeString);
  }
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
  for (const Formula::Arg& arg : gate->formula().args()) {
    if (Gate* const* arg_gate = std::get_if<Gate*>(&arg.event)) {
      if (gates.count(*arg_gate)) {
        MarkNonTopGates(*arg_gate, gates);
        (*arg_gate)->mark(NodeMark::kPermanent);  // Any non clear mark.
      }
    }
  }
}

}  // namespace scram::mef
