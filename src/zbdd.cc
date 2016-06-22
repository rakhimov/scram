/*
 * Copyright (C) 2015-2016 Olzhas Rakhimov
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

/// @file zbdd.cc
/// Implementation of Zero-Suppressed BDD algorithms.

#include "zbdd.h"

#include <algorithm>

#include <boost/range/algorithm.hpp>

#include "ext.h"
#include "logger.h"

namespace scram {
namespace core {

#ifndef NDEBUG
/// Runs assertions on ZBDD structure.
///
/// @param[in] full  A flag for full test including submodules.
#define CHECK_ZBDD(full)            \
  Zbdd::ClearMarks(root_, full);    \
  Zbdd::TestStructure(root_, full); \
  Zbdd::ClearMarks(root_, full)
#else
#define CHECK_ZBDD(full)  ///< No checks on release.
#endif

void Zbdd::Log() noexcept {
  CHECK_ZBDD(false);
  LOG(DEBUG4) << "# of ZBDD nodes created: " << set_id_ - 1;
  LOG(DEBUG4) << "# of entries in unique table: " << unique_table_.size();
  LOG(DEBUG4) << "# of entries in AND table: " << and_table_.size();
  LOG(DEBUG4) << "# of entries in OR table: " << or_table_.size();
  LOG(DEBUG4) << "# of entries in subsume table: " << subsume_table_.size();
  LOG(DEBUG4) << "# of entries in minimal table: " << minimal_results_.size();
  Zbdd::ClearMarks(root_, false);
  LOG(DEBUG4) << "# of SetNodes in ZBDD: " << Zbdd::CountSetNodes(root_);
  Zbdd::ClearMarks(root_, false);
  LOG(DEBUG4) << "# of products: " << Zbdd::CountProducts(root_, false);
  Zbdd::ClearMarks(root_, false);
}

Zbdd::Zbdd(Bdd* bdd, const Settings& settings) noexcept
    : Zbdd(bdd->root(), bdd->coherent(), bdd, settings) {
  CHECK_ZBDD(true);
}

Zbdd::Zbdd(const BooleanGraph* fault_tree, const Settings& settings) noexcept
    : Zbdd(fault_tree->root(), settings) {
  assert(!fault_tree->complement() && "Complements must be propagated.");
  if (fault_tree->root()->IsConstant()) {
    if (fault_tree->root()->state() == kNullState) {
      root_ = kEmpty_;
    } else {
      root_ = kBase_;
    }
  } else if (fault_tree->root()->type() == kNull) {
    GatePtr top = fault_tree->root();
    assert(top->args().size() == 1);
    assert(top->args<Gate>().empty());
    int child = *top->args().begin();
    if (child < 0) {
      root_ = kBase_;
    } else {
      VariablePtr var = top->args<Variable>().begin()->second;
      root_ =
          Zbdd::FindOrAddVertex(var->index(), kBase_, kEmpty_, var->order());
    }
  }
  CHECK_ZBDD(true);
}

void Zbdd::Analyze() noexcept {
  assert(root_->terminal() ||
         SetNode::Ptr(root_)->max_set_order() <= kSettings_.limit_order());
  root_ = Zbdd::Minimize(root_);  // Likely to be minimal by now.
  assert(root_->terminal() || SetNode::Ptr(root_)->minimal());
  for (const auto& entry : modules_) entry.second->Analyze();

  CLOCK(gen_time);
  LOG(DEBUG3) << "Getting products from minimized ZBDD: G" << module_index_;
  // Complete cleanup of the memory.
  Zbdd::ReleaseTables();
  Zbdd::ClearMarks(root_, false);
  Zbdd::ClearCounts(root_, false);
  Zbdd::EncodeLimitOrder(root_, kSettings_.limit_order());
  Zbdd::ClearMarks(root_, false);
  products_ = Zbdd::GenerateProducts(root_);

  // Cleanup of temporary products.
  modules_.clear();
  root_ = kEmpty_;
  LOG(DEBUG4) << "# of generated products: " << products_.size();
  LOG(DEBUG3) << "G" << module_index_ << " analysis time: " << DUR(gen_time);
}

Zbdd::Zbdd(const Settings& settings, bool coherent, int module_index) noexcept
    : kBase_(new Terminal<SetNode>(true)),
      kEmpty_(new Terminal<SetNode>(false)),
      kSettings_(settings),
      root_(kEmpty_),
      coherent_(coherent),
      module_index_(module_index),
      set_id_(2) {}

Zbdd::Zbdd(const Bdd::Function& module, bool coherent, Bdd* bdd,
           const Settings& settings, int module_index) noexcept
    : Zbdd(settings, coherent, module_index) {
  CLOCK(init_time);
  LOG(DEBUG2) << "Creating ZBDD from BDD: G" << module_index;
  LOG(DEBUG4) << "Limit on product order: " << settings.limit_order();
  PairTable<VertexPtr> ites;
  root_ = Zbdd::Minimize(Zbdd::ConvertBdd(module.vertex, module.complement, bdd,
                                          kSettings_.limit_order(), &ites));
  assert(root_->terminal() || SetNode::Ptr(root_)->minimal());
  Zbdd::Log();
  LOG(DEBUG2) << "Created ZBDD from BDD in " << DUR(init_time);
  std::unordered_map<int, std::pair<bool, int>> sub_modules;
  Zbdd::GatherModules(root_, 0, &sub_modules);
  for (const auto& entry : sub_modules) {
    int index = entry.first;
    assert(!modules_.count(index) && "Recalculating modules.");
    Bdd::Function sub = bdd->modules().find(std::abs(index))->second;
    assert(!sub.vertex->terminal() && "Unexpected BDD terminal vertex.");
    int limit = entry.second.second;
    assert(limit >= 0 && "Order cut-off is not strict.");
    bool module_coherence = entry.second.first && (index > 0);
    if (limit == 0 && module_coherence) {  // Unity is impossible.
      Zbdd::JoinModule(index, std::unique_ptr<Zbdd>(new Zbdd(settings)));
      continue;
    }
    Settings adjusted(settings);
    adjusted.limit_order(limit);
    sub.complement ^= index < 0;
    Zbdd::JoinModule(index, std::unique_ptr<Zbdd>(
            new Zbdd(sub, module_coherence, bdd, adjusted, index)));
  }
  if (ext::any_of(modules_, [](const ModuleEntry& member) {
        return member.second->root_->terminal();
      })) {
    LOG(DEBUG4) << "Eliminating constant modules from ZBDD...";
    std::unordered_map<int, VertexPtr> results;
    root_ = Zbdd::EliminateConstantModules(root_, &results);
  }
}

Zbdd::Zbdd(const GatePtr& gate, const Settings& settings) noexcept
    : Zbdd(settings, gate->coherent(), gate->index()) {
  if (gate->IsConstant() || gate->type() == kNull) return;
  assert(!settings.prime_implicants() && "Not implemented.");
  CLOCK(init_time);
  assert(gate->module() && "The constructor is meant for module gates.");
  LOG(DEBUG3) << "Converting module to ZBDD: G" << gate->index();
  LOG(DEBUG4) << "Limit on product order: " << settings.limit_order();
  std::unordered_map<int, std::pair<VertexPtr, int>> gates;
  std::unordered_map<int, GatePtr> module_gates;
  root_ = Zbdd::ConvertGraph(gate, &gates, &module_gates);
  if (!coherent_) {
    LOG(DEBUG4) << "Eliminating complements from ZBDD...";
    std::unordered_map<int, VertexPtr> results;
    root_ = Zbdd::EliminateComplements(root_, &results);
  }
  LOG(DEBUG4) << "Minimizing ZBDD...";
  root_ = Zbdd::Minimize(root_);
  Zbdd::Log();
  LOG(DEBUG3) << "Finished module conversion to ZBDD in " << DUR(init_time);
  std::unordered_map<int, std::pair<bool, int>> sub_modules;
  Zbdd::GatherModules(root_, 0, &sub_modules);
  for (const auto& entry : sub_modules) {
    int index = entry.first;
    assert(index > 0 && "No complement gates.");
    assert(!modules_.count(index) && "Recalculating modules.");
    int limit = entry.second.second;
    assert(limit >= 0 && "Order cut-off is not strict.");
    bool coherent = entry.second.first;
    if (limit == 0 && coherent) {  // Unity is impossible.
      Zbdd::JoinModule(index, std::unique_ptr<Zbdd>(new Zbdd(settings)));
      continue;
    }
    GatePtr module_gate = module_gates.find(index)->second;
    Settings adjusted(settings);
    adjusted.limit_order(limit);
    Zbdd::JoinModule(index,
                     std::unique_ptr<Zbdd>(new Zbdd(module_gate, adjusted)));
  }
  Zbdd::EliminateConstantModules();
}

#undef CHECK_ZBDD

SetNodePtr Zbdd::FindOrAddVertex(int index, const VertexPtr& high,
                                 const VertexPtr& low, int order,
                                 bool module, bool coherent) noexcept {
  assert(high->id() != low->id() && "Reduction failure.");

  SetNodeWeakPtr& in_table =
      unique_table_.FindOrAdd(index, high->id(), low->id());
  if (!in_table.expired()) return in_table.lock();
  assert(order > 0 && "Improper order.");
  SetNodePtr node(new SetNode(index, order, set_id_++, high, low));
  node->module(module);
  node->coherent(coherent);
  int high_order = high->terminal() ? 0 : SetNode::Ptr(high)->max_set_order();
  high_order += !Zbdd::MayBeUnity(node);
  int low_order = low->terminal() ? 0 : SetNode::Ptr(low)->max_set_order();
  node->max_set_order(std::max(high_order, low_order));

  in_table = node;
  return node;
}

SetNodePtr Zbdd::FindOrAddVertex(const SetNodePtr& node, const VertexPtr& high,
                                 const VertexPtr& low) noexcept {
  if (node->high()->id() == high->id() &&
      node->low()->id() == low->id()) return node;
  return Zbdd::FindOrAddVertex(node->index(), high, low, node->order(),
                               node->module(), node->coherent());
}

SetNodePtr Zbdd::FindOrAddVertex(const GatePtr& gate, const VertexPtr& high,
                                 const VertexPtr& low) noexcept {
  return Zbdd::FindOrAddVertex(gate->index(), high, low, gate->order(),
                               gate->module(), gate->coherent());
}

Zbdd::VertexPtr Zbdd::GetReducedVertex(const ItePtr& ite, bool complement,
                                       const VertexPtr& high,
                                       const VertexPtr& low) noexcept {
  if (high->id() == low->id()) return low;
  if (high->terminal() && !Terminal<SetNode>::Ptr(high)->value()) return low;
  if (low->terminal() && Terminal<SetNode>::Ptr(low)->value()) return low;
  assert(ite->index() > 0 && "BDD indices are never negative.");
  return Zbdd::FindOrAddVertex(complement ? -ite->index() : ite->index(),
                               high, low, ite->order(), ite->module(),
                               ite->coherent());
}

Zbdd::VertexPtr Zbdd::GetReducedVertex(const SetNodePtr& node,
                                       const VertexPtr& high,
                                       const VertexPtr& low) noexcept {
  if (high->id() == low->id()) return low;
  if (high->terminal() && !Terminal<SetNode>::Ptr(high)->value()) return low;
  if (low->terminal() && Terminal<SetNode>::Ptr(low)->value()) return low;
  if (node->high()->id() == high->id() &&
      node->low()->id() == low->id()) return node;
  return Zbdd::FindOrAddVertex(node, high, low);
}

Zbdd::VertexPtr Zbdd::ConvertBdd(const Bdd::VertexPtr& vertex, bool complement,
                                 Bdd* bdd_graph, int limit_order,
                                 PairTable<VertexPtr>* ites) noexcept {
  if (vertex->terminal()) return complement ? kEmpty_ : kBase_;
  VertexPtr& result =
      (*ites)[{complement ? -vertex->id() : vertex->id(), limit_order}];
  if (result) return result;
  if (!coherent_ && kSettings_.prime_implicants()) {
    result = Zbdd::ConvertBddPrimeImplicants(Ite::Ptr(vertex), complement,
                                             bdd_graph, limit_order, ites);
  } else {
    result = Zbdd::ConvertBdd(Ite::Ptr(vertex), complement, bdd_graph,
                              limit_order, ites);
  }
  assert(result->terminal() ||
         SetNode::Ptr(result)->max_set_order() <= limit_order);
  return result;
}

Zbdd::VertexPtr Zbdd::ConvertBdd(const ItePtr& ite, bool complement,
                                 Bdd* bdd_graph, int limit_order,
                                 PairTable<VertexPtr>* ites) noexcept {
  if (ite->module() && !ite->coherent())
    return Zbdd::ConvertBddPrimeImplicants(ite, complement, bdd_graph,
                                           limit_order, ites);
  VertexPtr low =
      Zbdd::ConvertBdd(ite->low(), ite->complement_edge() ^ complement,
                       bdd_graph, limit_order, ites);
  if (limit_order == 0) {  // Cut-off on the set order.
    if (low->terminal()) return low;
    return kEmpty_;
  }
  VertexPtr high =
      Zbdd::ConvertBdd(ite->high(), complement, bdd_graph, --limit_order, ites);
  return Zbdd::GetReducedVertex(ite, false, high, low);
}

Zbdd::VertexPtr
Zbdd::ConvertBddPrimeImplicants(const ItePtr& ite, bool complement,
                                Bdd* bdd_graph, int limit_order,
                                PairTable<VertexPtr>* ites) noexcept {
  Bdd::Function common = Bdd::Consensus::Calculate(bdd_graph, ite, complement);
  VertexPtr consensus = Zbdd::ConvertBdd(common.vertex, common.complement,
                                         bdd_graph, limit_order, ites);
  if (limit_order == 0) {  // Cut-off on the product order.
    if (consensus->terminal()) return consensus;
    return kEmpty_;
  }
  int sublimit = limit_order - 1;  // Assumes non-Unity element.
  if (ite->module() && !kSettings_.prime_implicants()) {
    assert(!ite->coherent() && "Only non-coherent modules through PI.");
    sublimit += 1;  // Unity modules may happen with minimal cut sets.
  }
  VertexPtr high =
      Zbdd::ConvertBdd(ite->high(), complement, bdd_graph, sublimit, ites);
  VertexPtr low =
      Zbdd::ConvertBdd(ite->low(), ite->complement_edge() ^ complement,
                       bdd_graph, sublimit, ites);
  return Zbdd::GetReducedVertex(ite, false, high,
                                Zbdd::GetReducedVertex(ite, true, low,
                                                       consensus));
}

Zbdd::VertexPtr Zbdd::ConvertGraph(
    const GatePtr& gate,
    std::unordered_map<int, std::pair<VertexPtr, int>>* gates,
    std::unordered_map<int, GatePtr>* module_gates) noexcept {
  assert(!gate->IsConstant() && "Unexpected constant gate!");
  VertexPtr result;
  if (auto it_entry = ext::find(*gates, gate->index())) {
    std::pair<VertexPtr, int>& entry = it_entry->second;
    result = entry.first;
    assert(entry.second < gate->parents().size());
    if (++entry.second == gate->parents().size()) gates->erase(it_entry);
    return result;
  }
  std::vector<VertexPtr> args;
  for (const Gate::Arg<Variable>& arg : gate->args<Variable>()) {
    args.push_back(
        Zbdd::FindOrAddVertex(arg.first, kBase_, kEmpty_, arg.second->order()));
  }
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    assert(arg.first > 0 && "Complements must be pushed down to variables.");
    if (arg.second->module()) {
      module_gates->insert(arg);
      args.push_back(Zbdd::FindOrAddVertex(arg.second, kBase_, kEmpty_));
    } else {
      args.push_back(Zbdd::ConvertGraph(arg.second, gates, module_gates));
    }
  }
  boost::sort(args, [](const VertexPtr& lhs, const VertexPtr& rhs) {
    if (lhs->terminal()) return true;
    if (rhs->terminal()) return false;
    return SetNode::Ptr(lhs)->order() > SetNode::Ptr(rhs)->order();
  });
  auto it = args.cbegin();
  for (result = *it++; it != args.cend(); ++it) {
    result = Zbdd::Apply(gate->type(), result, *it, kSettings_.limit_order());
  }
  Zbdd::ClearTables();
  assert(result);
  assert(result->terminal() ||
         SetNode::Ptr(result)->max_set_order() <= kSettings_.limit_order());
  if (gate->parents().size() > 1) gates->insert({gate->index(), {result, 1}});
  return result;
}

Triplet Zbdd::GetResultKey(const VertexPtr& arg_one, const VertexPtr& arg_two,
                           int order) noexcept {
  assert(order >= 0 && "Illegal order for computations.");
  assert(!arg_one->terminal() && !arg_two->terminal());
  assert(arg_one->id() && arg_two->id());
  assert(arg_one->id() != arg_two->id());
  int min_id = std::min(arg_one->id(), arg_two->id());
  int max_id = std::max(arg_one->id(), arg_two->id());
  return {min_id, max_id, order};
}

/// Forward declarations of interdependent Apply operation specializations.
/// @{
template <>
Zbdd::VertexPtr Zbdd::Apply<kAnd>(const VertexPtr& arg_one,
                                  const VertexPtr& arg_two,
                                  int limit_order) noexcept;
template <>
Zbdd::VertexPtr Zbdd::Apply<kOr>(const VertexPtr& arg_one,
                                 const VertexPtr& arg_two,
                                 int limit_order) noexcept;
/// @}

/// Specialization of Apply for AND operator for non-terminal ZBDD vertices.
template <>
Zbdd::VertexPtr Zbdd::Apply<kAnd>(const SetNodePtr& arg_one,
                                  const SetNodePtr& arg_two,
                                  int limit_order) noexcept {
  VertexPtr high;
  VertexPtr low;
  int limit_high = limit_order - !Zbdd::MayBeUnity(arg_one);
  if (arg_one->order() == arg_two->order() &&
      arg_one->index() == arg_two->index()) {  // The same variable.
    // (x*f1 + f0) * (x*g1 + g0) = x*(f1*(g1 + g0) + f0*g1) + f0*g0
    high = Zbdd::Apply<kOr>(
        Zbdd::Apply<kAnd>(
            arg_one->high(),
            Zbdd::Apply<kOr>(arg_two->high(), arg_two->low(), limit_high),
            limit_high),
        Zbdd::Apply<kAnd>(arg_one->low(), arg_two->high(), limit_high),
        limit_high);
    low = Zbdd::Apply<kAnd>(arg_one->low(), arg_two->low(), limit_order);
  } else {
    assert((arg_one->order() < arg_two->order() ||
            arg_one->index() > arg_two->index()) &&
           "Ordering contract failed.");
    if (arg_one->order() == arg_two->order()) {
      // (x*f1 + f0) * (~x*g1 + g0) = x*f1*g0 + f0*(~x*g1 + g0)
      high = Zbdd::Apply<kAnd>(arg_one->high(), arg_two->low(), limit_high);
    } else {
      high = Zbdd::Apply<kAnd>(arg_one->high(), arg_two, limit_high);
    }
    low = Zbdd::Apply<kAnd>(arg_one->low(), arg_two, limit_order);
  }
  if (!high->terminal() && SetNode::Ptr(high)->order() == arg_one->order()) {
    assert(SetNode::Ptr(high)->index() < arg_one->index());
    high = SetNode::Ptr(high)->low();
  }
  return Zbdd::Minimize(Zbdd::GetReducedVertex(arg_one, high, low));
}

/// Specialization of Apply for AND operator for any ZBDD vertices.
template <>
Zbdd::VertexPtr Zbdd::Apply<kAnd>(const VertexPtr& arg_one,
                                  const VertexPtr& arg_two,
                                  int limit_order) noexcept {
  if (limit_order < 0) return kEmpty_;
  if (arg_one->terminal()) {
    if (Terminal<SetNode>::Ptr(arg_one)->value())
      return Zbdd::Prune(arg_two, limit_order);
    return kEmpty_;
  }
  if (arg_two->terminal()) {
    if (Terminal<SetNode>::Ptr(arg_two)->value())
      return Zbdd::Prune(arg_one, limit_order);
    return kEmpty_;
  }
  if (arg_one->id() == arg_two->id()) return Zbdd::Prune(arg_one, limit_order);

  VertexPtr& result =
      and_table_[Zbdd::GetResultKey(arg_one, arg_two, limit_order)];
  if (result) return result;  // Already computed.

  SetNodePtr set_one = SetNode::Ptr(arg_one);
  SetNodePtr set_two = SetNode::Ptr(arg_two);
  if (set_one->order() > set_two->order()) {
    std::swap(set_one, set_two);
  } else if (set_one->order() == set_two->order() &&
             set_one->index() < set_two->index()) {
    std::swap(set_one, set_two);
  }
  result = Zbdd::Apply<kAnd>(set_one, set_two, limit_order);
  assert(result->terminal() ||
         SetNode::Ptr(result)->max_set_order() <= limit_order);
  return result;
}

/// Specialization of Apply for OR operator for non-terminal ZBDD vertices.
template <>
Zbdd::VertexPtr Zbdd::Apply<kOr>(const SetNodePtr& arg_one,
                                 const SetNodePtr& arg_two,
                                 int limit_order) noexcept {
  VertexPtr high;
  VertexPtr low;
  int limit_high = limit_order - !Zbdd::MayBeUnity(arg_one);
  if (arg_one->order() == arg_two->order() &&
      arg_one->index() == arg_two->index()) {  // The same variable.
    high = Zbdd::Apply<kOr>(arg_one->high(), arg_two->high(), limit_high);
    low = Zbdd::Apply<kOr>(arg_one->low(), arg_two->low(), limit_order);
  } else {
    assert((arg_one->order() < arg_two->order() ||
            arg_one->index() > arg_two->index()) &&
           "Ordering contract failed.");
    if (arg_one->order() == arg_two->order()) {
      if (arg_one->high()->terminal() && arg_two->high()->terminal())
        return kBase_;
    }
    high = Zbdd::Prune(arg_one->high(), limit_high);
    low = Zbdd::Apply<kOr>(arg_one->low(), arg_two, limit_order);
  }
  if (!high->terminal() && SetNode::Ptr(high)->order() == arg_one->order()) {
    assert(SetNode::Ptr(high)->index() < arg_one->index());
    high = SetNode::Ptr(high)->low();
  }
  return Zbdd::Minimize(Zbdd::GetReducedVertex(arg_one, high, low));
}

/// Specialization of Apply for OR operator for any ZBDD vertices.
template <>
Zbdd::VertexPtr Zbdd::Apply<kOr>(const VertexPtr& arg_one,
                                 const VertexPtr& arg_two,
                                 int limit_order) noexcept {
  if (limit_order < 0) return kEmpty_;
  if (arg_one->terminal()) {
    if (Terminal<SetNode>::Ptr(arg_one)->value()) return kBase_;
    return Zbdd::Prune(arg_two, limit_order);
  }
  if (arg_two->terminal()) {
    if (Terminal<SetNode>::Ptr(arg_two)->value()) return kBase_;
    return Zbdd::Prune(arg_one, limit_order);
  }
  if (arg_one->id() == arg_two->id()) return Zbdd::Prune(arg_one, limit_order);

  VertexPtr& result =
      or_table_[Zbdd::GetResultKey(arg_one, arg_two, limit_order)];
  if (result) return result;  // Already computed.

  SetNodePtr set_one = SetNode::Ptr(arg_one);
  SetNodePtr set_two = SetNode::Ptr(arg_two);
  if (set_one->order() > set_two->order()) {
    std::swap(set_one, set_two);
  } else if (set_one->order() == set_two->order() &&
             set_one->index() < set_two->index()) {
    std::swap(set_one, set_two);
  }
  result = Zbdd::Apply<kOr>(set_one, set_two, limit_order);
  assert(result->terminal() ||
         SetNode::Ptr(result)->max_set_order() <= limit_order);
  return result;
}

Zbdd::VertexPtr Zbdd::Apply(Operator type, const VertexPtr& arg_one,
                            const VertexPtr& arg_two,
                            int limit_order) noexcept {
  if (type == kAnd) return Zbdd::Apply<kAnd>(arg_one, arg_two, limit_order);
  assert(type == kOr && "Only normalized operations in BDD.");
  return Zbdd::Apply<kOr>(arg_one, arg_two, limit_order);
}

Zbdd::VertexPtr Zbdd::EliminateComplements(
    const VertexPtr& vertex,
    std::unordered_map<int, VertexPtr>* wide_results) noexcept {
  if (vertex->terminal()) return vertex;
  VertexPtr& result = (*wide_results)[vertex->id()];
  if (result) return result;
  SetNodePtr node = SetNode::Ptr(vertex);
  result = Zbdd::EliminateComplement(
      node,
      Zbdd::EliminateComplements(node->high(), wide_results),
      Zbdd::EliminateComplements(node->low(), wide_results));
  return result;
}

Zbdd::VertexPtr Zbdd::EliminateComplement(const SetNodePtr& node,
                                          const VertexPtr& high,
                                          const VertexPtr& low) noexcept {
  // Cut-off does not matter with the OR operation with conforming inputs.
  assert(high->terminal() ||
         SetNode::Ptr(high)->terminal() <= kSettings_.limit_order());
  assert(low->terminal() ||
         SetNode::Ptr(low)->terminal() <= kSettings_.limit_order());
  if (node->index() < 0 && !(node->module() && !node->coherent()))
    return Zbdd::Apply<kOr>(high, low, kSettings_.limit_order());
  return Zbdd::Minimize(Zbdd::GetReducedVertex(node, high, low));
}

void Zbdd::EliminateConstantModules() noexcept {
  if (ext::any_of(modules_, [](const ModuleEntry& module) {
        return module.second->root_->terminal();
      })) {
    LOG(DEBUG4) << "Eliminating constant modules from ZBDD: G" << module_index_;
    std::unordered_map<int, VertexPtr> results;
    root_ = Zbdd::EliminateConstantModules(root_, &results);
  }
}

Zbdd::VertexPtr Zbdd::EliminateConstantModules(
    const VertexPtr& vertex,
    std::unordered_map<int, VertexPtr>* results) noexcept {
  if (vertex->terminal()) return vertex;
  VertexPtr& result = (*results)[vertex->id()];
  if (result) return result;
  SetNodePtr node = SetNode::Ptr(vertex);
  result = Zbdd::EliminateConstantModule(
      node,
      Zbdd::EliminateConstantModules(node->high(), results),
      Zbdd::EliminateConstantModules(node->low(), results));
  return result;
}

Zbdd::VertexPtr Zbdd::EliminateConstantModule(const SetNodePtr& node,
                                              const VertexPtr& high,
                                              const VertexPtr& low) noexcept {
  if (node->module()) {
    Zbdd* module = modules_.find(node->index())->second.get();
    if (module->root_->terminal()) {
      if (!Terminal<SetNode>::Ptr(module->root_)->value()) return low;
      return Zbdd::Apply<kOr>(high, low, kSettings_.limit_order());
    }
  }
  return Zbdd::Minimize(Zbdd::GetReducedVertex(node, high, low));
}

Zbdd::VertexPtr Zbdd::Minimize(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return vertex;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->minimal()) return vertex;
  VertexPtr& result = minimal_results_[vertex->id()];
  if (result) return result;
  VertexPtr high = Zbdd::Minimize(node->high());
  VertexPtr low = Zbdd::Minimize(node->low());
  high = Zbdd::Subsume(high, low);
  assert(high->id() != low->id() && "Subsume failed!");
  if (high->terminal() && !Terminal<SetNode>::Ptr(high)->value()) {
    result = low;  // Reduction rule.
    return result;
  }
  result = Zbdd::FindOrAddVertex(node, high, low);
  SetNode::Ptr(result)->minimal(true);
  return result;
}

Zbdd::VertexPtr Zbdd::Subsume(const VertexPtr& high,
                              const VertexPtr& low) noexcept {
  if (low->terminal())
    return Terminal<SetNode>::Ptr(low)->value() ? kEmpty_ : high;
  if (high->terminal()) return high;  // No need to reduce terminal sets.
  VertexPtr& computed = subsume_table_[{high->id(), low->id()}];
  if (computed) return computed;

  SetNodePtr high_node = SetNode::Ptr(high);
  SetNodePtr low_node = SetNode::Ptr(low);
  if (high_node->order() > low_node->order() ||
      (high_node->order() == low_node->order() &&
       high_node->index() < low_node->index())) {
    computed = Zbdd::Subsume(high, low_node->low());
    return computed;
  }
  VertexPtr subhigh;
  VertexPtr sublow;
  if (high_node->order() == low_node->order() &&
      high_node->index() == low_node->index()) {
    assert(high_node->index() == low_node->index());
    subhigh = Zbdd::Subsume(high_node->high(), low_node->high());
    subhigh = Zbdd::Subsume(subhigh, low_node->low());
    sublow = Zbdd::Subsume(high_node->low(), low_node->low());
  } else {
    assert(high_node->order() < low_node->order() ||
           (high_node->order() == low_node->order() &&
            high_node->index() > low_node->index()));
    subhigh = Zbdd::Subsume(high_node->high(), low);
    sublow = Zbdd::Subsume(high_node->low(), low);
  }
  if (subhigh->terminal() && !Terminal<SetNode>::Ptr(subhigh)->value()) {
    computed = sublow;
    return computed;
  }
  assert(subhigh->id() != sublow->id());
  SetNodePtr new_high = Zbdd::FindOrAddVertex(high_node, subhigh, sublow);
  new_high->minimal(high_node->minimal());
  computed = new_high;
  return computed;
}

Zbdd::VertexPtr Zbdd::Prune(const VertexPtr& vertex, int limit_order) noexcept {
  if (limit_order < 0) return kEmpty_;
  if (vertex->terminal()) return vertex;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->max_set_order() <= limit_order) return node;
  VertexPtr& result = prune_results_[{node->id(), limit_order}];
  if (result) return result;

  int limit_high = limit_order - !Zbdd::MayBeUnity(node);
  result = Zbdd::GetReducedVertex(node, Zbdd::Prune(node->high(), limit_high),
                                  Zbdd::Prune(node->low(), limit_order));
  if (!result->terminal()) SetNode::Ptr(result)->minimal(node->minimal());
  return result;
}

bool Zbdd::MayBeUnity(const SetNodePtr& node) noexcept {
  if (kSettings_.prime_implicants()) return false;
  // Unity node tests for minimal cut sets.
  if (node->index() < 0) return true;  // Unity complement vars.
  // Non-modular gates can be implied by other gates in the product;
  // that is, (G1 & G2 = I & G2) if G2 implies G1.
  //
  // Non-coherent gates contain complements to be approximated to Unity.
  if (this->IsGate(node)) return !node->module() || !node->coherent();
  return false;  // Positive non-gate variable.
}

int Zbdd::GatherModules(
    const VertexPtr& vertex,
    int current_order,
    std::unordered_map<int, std::pair<bool, int>>* modules) noexcept {
  assert(current_order >= 0);
  if (vertex->terminal())
    return Terminal<SetNode>::Ptr(vertex)->value() ? 0 : -1;
  SetNodePtr node = SetNode::Ptr(vertex);
  int contribution = !Zbdd::MayBeUnity(node);
  int high_order = current_order + contribution;
  int min_high = Zbdd::GatherModules(node->high(), high_order, modules);
  assert(min_high >= 0 && "No terminal Empty should be on high branch.");
  if (node->module()) {
    int module_order = kSettings_.limit_order() - min_high - current_order;
    assert(module_order >= 0 && "Improper application of a cut-off.");
    if (auto it = ext::find(*modules, node->index())) {
      std::pair<bool, int>& entry = it->second;
      assert(entry.first == node->coherent() && "Inconsistent flags.");
      entry.second = std::max(entry.second, module_order);
    } else {
      modules->insert({node->index(), {node->coherent(), module_order}});
    }
  }
  int min_low = Zbdd::GatherModules(node->low(), current_order, modules);
  assert(min_low >= -1);
  if (min_low == -1) return min_high + contribution;
  return std::min(min_high + contribution, min_low);
}

void Zbdd::EncodeLimitOrder(const VertexPtr& vertex, int limit_order) noexcept {
  if (vertex->terminal()) return;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->count() >= limit_order) return;
  node->count(limit_order);
  Zbdd::EncodeLimitOrder(node->high(), limit_order - 1);
  Zbdd::EncodeLimitOrder(node->low(), limit_order);
}

std::vector<std::vector<int>>
Zbdd::GenerateProducts(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) {
    if (Terminal<SetNode>::Ptr(vertex)->value()) return {{}};  // The Base set.
    return {};  // Don't include 0/NULL sets.
  }
  SetNodePtr node = SetNode::Ptr(vertex);
  assert(node->minimal() && "Detected non-minimal ZBDD.");
  if (node->count() <= 0) return {};  // The result of a conservative count.

  if (node->mark()) return node->products();
  node->mark(true);
  std::vector<Product> low = Zbdd::GenerateProducts(node->low());
  std::vector<Product> high = Zbdd::GenerateProducts(node->high());
  std::vector<Product> result;
  for (auto& product : low) {
    if (product.size() <= node->count())
      result.emplace_back(std::move(product));
  }
  if (node->module()) {
    Zbdd* module = modules_.find(node->index())->second.get();
    for (const auto& product : high) {  // Cross-product.
      for (const auto& module_set : module->products()) {
        if (product.size() + module_set.size() > node->count())
          continue;  // Cut-off on the product size.
        Product combo = product;
        combo.insert(combo.end(), module_set.begin(), module_set.end());
        result.emplace_back(std::move(combo));
      }
    }
  } else {
    for (auto& product : high) {
      if (product.size() < node->count()) {
        product.push_back(node->index());
        result.emplace_back(std::move(product));
      }
    }
  }

  // Destroy the subgraph to remove extra reference counts.
  node->CutBranches();

  if (node->use_count() > 2) node->products(result);
  return result;
}

int Zbdd::CountSetNodes(const VertexPtr& vertex) noexcept {
  if (vertex->terminal()) return 0;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->mark()) return 0;
  node->mark(true);
  return 1 + Zbdd::CountSetNodes(node->high()) +
         Zbdd::CountSetNodes(node->low());
}

int64_t Zbdd::CountProducts(const VertexPtr& vertex, bool modules) noexcept {
  if (vertex->terminal()) {
    if (Terminal<SetNode>::Ptr(vertex)->value()) return 1;
    return 0;
  }
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->mark()) return node->count();
  node->mark(true);
  int64_t multiplier = 1;  // Multiplier of the module.
  if (modules && node->module()) {
    Zbdd* module = modules_.find(node->index())->second.get();
    multiplier = module->CountProducts(module->root_, true);
  }
  node->count(multiplier * Zbdd::CountProducts(node->high(), modules) +
              Zbdd::CountProducts(node->low(), modules));
  return node->count();
}

void Zbdd::ClearMarks(const VertexPtr& vertex, bool modules) noexcept {
  if (vertex->terminal()) return;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (!node->mark()) return;
  node->mark(false);
  if (modules && node->module()) {
    Zbdd* module = modules_.find(node->index())->second.get();
    module->ClearMarks(module->root_, true);
  }
  Zbdd::ClearMarks(node->high(), modules);
  Zbdd::ClearMarks(node->low(), modules);
}

void Zbdd::ClearCounts(const VertexPtr& vertex, bool modules) noexcept {
  if (vertex->terminal()) return;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->mark()) return;
  node->mark(true);
  node->count(0);
  if (modules && node->module()) {
    Zbdd* module = modules_.find(node->index())->second.get();
    module->ClearCounts(module->root_, true);
  }
  Zbdd::ClearCounts(node->high(), modules);
  Zbdd::ClearCounts(node->low(), modules);
}

void Zbdd::TestStructure(const VertexPtr& vertex, bool modules) noexcept {
  if (vertex->terminal()) return;
  SetNodePtr node = SetNode::Ptr(vertex);
  if (node->mark()) return;
  node->mark(true);
  assert(node->index() && "Illegal index for a node.");
  assert(node->order() && "Improper order for nodes.");
  assert(node->high() && node->low() && "Malformed node high/low pointers.");
  assert(!(node->high()->terminal() &&
           !Terminal<SetNode>::Ptr(node->high())->value()) &&
         "Reduction rule failure.");
  assert((node->high()->id() != node->low()->id()) && "Minimization failure.");
  assert(!(!node->high()->terminal() &&
           node->order() >= SetNode::Ptr(node->high())->order()) &&
         "Ordering of nodes failed.");
  assert(!(!node->low()->terminal() &&
           node->order() > SetNode::Ptr(node->low())->order()) &&
         "Ordering of nodes failed.");
  assert(!(!node->low()->terminal() &&
           node->order() == SetNode::Ptr(node->low())->order() &&
           node->index() <= SetNode::Ptr(node->low())->index()) &&
         "Ordering of complements failed.");
  assert(!(!node->high()->terminal() && node->minimal() &&
           !SetNode::Ptr(node->high())->minimal()) &&
         "Non-minimal branches in minimal ZBDD.");
  assert(!(!node->low()->terminal() && node->minimal() &&
           !SetNode::Ptr(node->low())->minimal()) &&
         "Non-minimal branches in minimal ZBDD.");
  if (modules && node->module()) {
    Zbdd* module = modules_.find(node->index())->second.get();
    assert(!module->root_->terminal() && "Terminal modules must be removed.");
    module->TestStructure(module->root_, true);
  }
  Zbdd::TestStructure(node->high(), modules);
  Zbdd::TestStructure(node->low(), modules);
}

namespace zbdd {

CutSetContainer::CutSetContainer(const Settings& settings, int module_index,
                                 int gate_index_bound) noexcept
    : Zbdd(settings, /*coherence=*/false, module_index),
      gate_index_bound_(gate_index_bound) {}

Zbdd::VertexPtr CutSetContainer::ConvertGate(const GatePtr& gate) noexcept {
  assert(gate->type() == kAnd || gate->type() == kOr);
  assert(gate->args<Constant>().empty());
  assert(gate->args().size() > 1);
  std::vector<SetNodePtr> args;
  for (const Gate::Arg<Variable>& arg : gate->args<Variable>()) {
    args.push_back(
        Zbdd::FindOrAddVertex(arg.first, kBase_, kEmpty_, arg.second->order()));
  }
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    assert(arg.first > 0 && "Complements must be pushed down to variables.");
    args.push_back(Zbdd::FindOrAddVertex(arg.second, kBase_, kEmpty_));
  }
  boost::sort(args, [](const SetNodePtr& lhs, const SetNodePtr& rhs) {
    return lhs->order() > rhs->order();
  });
  auto it = args.cbegin();
  VertexPtr result = *it;
  for (++it; it != args.cend(); ++it) {
    result =
        Zbdd::Apply(gate->type(), result, *it, Zbdd::settings().limit_order());
  }
  Zbdd::ClearTables();
  return result;
}

Zbdd::VertexPtr CutSetContainer::ExtractIntermediateCutSets(
    int index) noexcept {
  assert(index && index > gate_index_bound_);
  assert(!Zbdd::root()->terminal() &&
         "Impossible to have intermediate cut sets.");
  assert(index == SetNode::Ptr(Zbdd::root())->index() && "Broken ordering!");
  assert(SetNode::Ptr(Zbdd::root())->max_set_order() <=
         Zbdd::settings().limit_order());
  LOG(DEBUG5) << "Extracting cut sets for G" << index;
  SetNodePtr node = SetNode::Ptr(Zbdd::root());
  Zbdd::root(node->low());
  return node->high();
}

Zbdd::VertexPtr
CutSetContainer::ExpandGate(const VertexPtr& gate_zbdd,
                            const VertexPtr& cut_sets) noexcept {
  assert(gate_zbdd->terminal() || SetNode::Ptr(gate_zbdd)->max_set_order() <=
                                      Zbdd::settings().limit_order());
  assert(cut_sets->terminal() || SetNode::Ptr(cut_sets)->max_set_order() <=
                                     Zbdd::settings().limit_order());
  return Zbdd::Apply<kAnd>(gate_zbdd, cut_sets, Zbdd::settings().limit_order());
}

void CutSetContainer::Merge(const VertexPtr& vertex) noexcept {
  assert(vertex->terminal() || SetNode::Ptr(vertex)->max_set_order() <=
                                   Zbdd::settings().limit_order());
  Zbdd::root(
      Zbdd::Apply<kOr>(Zbdd::root(), vertex, Zbdd::settings().limit_order()));
  Zbdd::ClearTables();
}

}  // namespace zbdd

}  // namespace core
}  // namespace scram
