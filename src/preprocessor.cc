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

/// @file preprocessor.cc
/// Implementation of preprocessing algorithms.
/// The main goal of preprocessing algorithms is
/// to make PDAGs simpler, modular, easier for analysis.
///
/// If a preprocessing algorithm has
/// its limitations, side-effects, and assumptions,
/// the documentation in the header file
/// must contain all the relevant information within
/// its description, preconditions, postconditions, notes, or warnings.
/// The default assumption for all algorithms is
/// that the PDAG is valid and well-formed.
///
/// Some suggested contracts/notes for preprocessing algorithms:
///
///   * Works with coherent graphs only
///   * Works with positive gates or nodes only
///   * Depends on node visit information, gate marks, or other node flags.
///   * May introduce NULL or UNITY state gates or constants
///   * May introduce NULL/NOT type gates
///   * Operates on certain gate types only
///   * Works with normalized gates or structure only
///   * Cannot accept a graph with gates of certain types
///   * May destroy modules
///   * Can accept graphs with constants or constant gates
///   * Depends on other preprocessing functions or algorithms
///   * Swaps the root gate of the graph with another (arg) gate
///   * Removes gates or other kind of nodes
///   * May introduce new gate clones or subgraphs,
///     making the graph more complex.
///   * Works on particular cases or setups only
///   * Has trade-offs
///   * Runs better/More effective before/after some preprocessing step(s)
///   * Coupled with another preprocessing algorithms
///   * Idempotent (consecutive, repeated application doesn't yield any change)
///   * One-time operation (any repetition is pointless or dangerous)
///
/// Assuming that the PDAG is provided
/// in the state as described in the contract,
/// the algorithms should never throw an exception.
///
/// The algorithms must guarantee
/// that, given a valid and well-formed PDAG,
/// the resulting PDAG
/// will at least be valid, well-formed,
/// and semantically equivalent (isomorphic) to the input PDAG.
/// Moreover, the algorithms must be deterministic
/// and produce stable results.
///
/// If the contract is violated,
/// the result or behavior of the algorithm can be undefined.
/// There is no requirement
/// to check for the contract violations
/// and to exit gracefully.

#include "preprocessor.h"

#include <cstdlib>

#include <algorithm>
#include <array>
#include <list>
#include <queue>
#include <unordered_set>

#include <boost/functional/hash.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

#include "ext/algorithm.h"
#include "ext/find_iterator.h"
#include "logger.h"

namespace scram {
namespace core {

namespace pdag {

template <class T>
std::vector<T*> OrderArguments(Gate* gate) noexcept {
  std::vector<T*> args;
  for (const Gate::Arg<T>& arg : gate->args<T>()) {
    args.push_back(arg.second.get());
  }
  boost::sort(args, [](T* lhs, T* rhs) {
    return lhs->parents().size() > rhs->parents().size();
  });
  return args;
}

void TopologicalOrder(Pdag* graph) noexcept {
  // Assigns the order starting from the given gate arguments.
  auto topological_order = [](auto& self, Gate* root, int order) {
    if (root->order())
      return order;
    for (Gate* arg : OrderArguments<Gate>(root)) {
      order = self(self, arg, order);
    }
    for (Variable* arg : OrderArguments<Variable>(root)) {
      if (!arg->order())
        arg->order(++order);
    }
    assert(!root->constant());
    root->order(++order);
    return order;
  };

  graph->Clear<Pdag::kOrder>();
  topological_order(topological_order, graph->root().get(), 0);
}

void MarkCoherence(Pdag* graph) noexcept {
  auto mark_coherence = [](auto& self, const GatePtr& gate) {
    if (gate->mark())
      return;
    gate->mark(true);
    bool coherent = true;  // Optimistic initialization.
    switch (gate->type()) {
      case kXor:
      case kNor:
      case kNot:
      case kNand:
        coherent = false;
        break;
      default:
        assert(coherent);
    }
    for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
      self(self, arg.second);
      if (coherent && (arg.first < 0 || !arg.second->coherent()))
        coherent = false;  // Must continue with all gates.
    }
    if (coherent) {
      for (const Gate::Arg<Variable>& arg : gate->args<Variable>()) {
        if (arg.first < 0) {
          coherent = false;
          break;
        }
      }
    }
    assert(!gate->constant());
    gate->coherent(coherent);
  };

  graph->Clear<Pdag::kGateMark>();
  mark_coherence(mark_coherence, graph->root());
  assert(!(graph->coherent() && !graph->root()->coherent()));
  graph->coherent(!graph->complement() && graph->root()->coherent());
}

}  // namespace pdag

Preprocessor::Preprocessor(Pdag* graph) noexcept : graph_(graph) {}

void Preprocessor::operator()() noexcept {
  TIMER(DEBUG2, "Preprocessing");
  this->Run();
}

void Preprocessor::Run() noexcept {
  pdag::Transform(graph_, [this](Pdag*) { RunPhaseOne(); },
                  [this](Pdag*) { RunPhaseTwo(); },
                  [this](Pdag*) {
                    if (!graph_->normal())
                      RunPhaseThree();
                  });
}

/// Container of unique gates.
/// This container acts like an unordered set of gates.
/// The gates are equivalent
/// if they have the same semantics.
/// However, this set does not test
/// for the isomorphism of the gates' Boolean formulas.
class Preprocessor::GateSet {
 public:
  /// Inserts a gate into the set
  /// if it is semantically unique.
  ///
  /// @param[in] gate  The gate to insert.
  ///
  /// @returns A pair of the unique gate and
  ///          the insertion success flag.
  std::pair<GatePtr, bool> insert(const GatePtr& gate) noexcept {
    auto result = table_[gate->type()].insert(gate);
    return {*result.first, result.second};
  }

 private:
  /// Functor for hashing gates by their arguments.
  ///
  /// @note The hashing discards the logic of the gate.
  struct Hash {
    /// Operator overload for hashing.
    ///
    /// @param[in] gate  The gate which hash must be calculated.
    ///
    /// @returns Hash value of the gate
    ///          from its arguments but not logic.
    std::size_t operator()(const GatePtr& gate) const noexcept {
      return boost::hash_range(gate->args().begin(), gate->args().end());
    }
  };
  /// Functor for equality test for gates by their arguments.
  ///
  /// @note The equality discards the logic of the gate.
  struct Equal {
    /// Operator overload for gate argument equality test.
    ///
    /// @param[in] lhs  The first gate.
    /// @param[in] rhs  The second gate.
    ///
    /// @returns true if the gate arguments are equal.
    bool operator()(const GatePtr& lhs, const GatePtr& rhs) const noexcept {
      assert(lhs->type() == rhs->type());
      if (lhs->args() != rhs->args())
        return false;
      if (lhs->type() == kVote && lhs->vote_number() != rhs->vote_number())
        return false;
      return true;
    }
  };
  /// Container of gates grouped by their types.
  std::array<std::unordered_set<GatePtr, Hash, Equal>, kNumOperators> table_;
};

namespace {  // PDAG structure verification tools.

/// Functor to sanity check the marks of PDAG gates.
class TestGateMarks {
 public:
  /// Helper function to find discontinuous gate marking.
  /// The traversal will fail with assert
  /// upon depth-first encounter of a problem.
  ///
  /// @param[in] gate  The starting gate to traverse.
  /// @param[in] mark  Assumed mark of the whole graph.
  ///
  /// @returns true if the job is done.
  bool operator()(const Gate& gate, bool mark) noexcept {
    if (tested_gates_.insert(gate.index()).second == false)
      return false;
    assert(gate.mark() == mark && "Found discontinuous gate mark.");
    for (const auto& arg : gate.args<Gate>())
      (*this)(arg.second, mark);
    return true;
  }

 private:
  std::unordered_set<int> tested_gates_;  ///< Alternative to gate marking.
};

/// Functor to sanity check the structure of PDAG gates.
class TestGateStructure {
 public:
  /// Helper function to find malformed gates.
  /// The traversal will fail with assert
  /// upon depth-first encounter of a problem.
  ///
  /// @param[in] gate  The starting gate to traverse.
  ///
  /// @returns true if the job is done.
  bool operator()(const Gate& gate) noexcept {
    if (tested_gates_.insert(gate.index()).second == false)
      return false;
    assert(!gate.constant() && "Constant gates are not clear!");
    switch (gate.type()) {
      case kNull:
      case kNot:
        assert(gate.args().size() == 1 && "Malformed one-arg gate!");
        break;
      case kXor:
        assert(gate.args().size() == 2 && "Malformed XOR gate!");
        break;
      case kVote:
        assert(gate.vote_number() > 1 && "K/N has wrong K!");
        assert(gate.args().size() > gate.vote_number() && "K/N has wrong N!");
        break;
      default:
        assert(gate.args().size() > 1 && "Missing arguments!");
    }
    for (const auto& arg : gate.args<Gate>())
      (*this)(arg.second);
    return true;
  }

 private:
  std::unordered_set<int> tested_gates_;  ///< Alternative to gate marking.
};

}  // namespace

/// A collection of sanity checks between preprocessing phases.
#define SANITY_ASSERT                                                      \
  assert(graph_->root() && "Corrupted pointer to the root gate.");         \
  assert(graph_->root()->parents().empty() && "Root can't have parents."); \
  assert(!(graph_->coherent() && graph_->complement()));                   \
  assert(!graph_->HasConstants() && "Const gate cleanup is broken!");      \
  assert(!graph_->HasNullGates() && "Null gate cleanup is broken!");       \
  assert(TestGateStructure()(*graph_->root()));                            \
  assert(TestGateMarks()(*graph_->root(), graph_->root()->mark()))

void Preprocessor::RunPhaseOne() noexcept {
  TIMER(DEBUG2, "Preprocessing Phase I");
  graph_->Log();
  if (graph_->HasNullGates()) {
    TIMER(DEBUG3, "Removing NULL gates");
    graph_->RemoveNullGates();
    if (graph_->IsTrivial())
      return;
  }
  SANITY_ASSERT;
  if (!graph_->coherent()) {
    NormalizeGates(/*full=*/false);
  }
}

void Preprocessor::RunPhaseTwo() noexcept {
  TIMER(DEBUG2, "Preprocessing Phase II");
  SANITY_ASSERT;
  graph_->Log();
  pdag::Transform(graph_,
                  [this](Pdag*) {
                    while (ProcessMultipleDefinitions())
                      continue;
                  },
                  [this](Pdag*) { DetectModules(); },
                  [this](Pdag*) {
                    while (CoalesceGates(/*common=*/false))
                      continue;
                  },
                  [this](Pdag*) { MergeCommonArgs(); },
                  [this](Pdag*) { DetectDistributivity(); },
                  [this](Pdag*) { DetectModules(); },
                  [this](Pdag*) { BooleanOptimization(); },
                  [this](Pdag*) { DecomposeCommonNodes(); },
                  [this](Pdag*) { DetectModules(); },
                  [this](Pdag*) {
                    while (CoalesceGates(/*common=*/false))
                      continue;
                  },
                  [this](Pdag*) { DetectModules(); });
  graph_->Log();
}

void Preprocessor::RunPhaseThree() noexcept {
  TIMER(DEBUG2, "Preprocessing Phase III");
  SANITY_ASSERT;
  graph_->Log();
  assert(!graph_->normal());
  NormalizeGates(/*full=*/true);
  graph_->normal(true);

  if (graph_->IsTrivial())
    return;
  LOG(DEBUG2) << "Continue with Phase II within Phase III";
  RunPhaseTwo();
}

void Preprocessor::RunPhaseFour() noexcept {
  TIMER(DEBUG2, "Preprocessing Phase IV");
  SANITY_ASSERT;
  graph_->Log();
  assert(!graph_->coherent());
  LOG(DEBUG3) << "Propagating complements...";
  if (graph_->complement()) {
    const GatePtr& root = graph_->root();
    assert(root->type() == kOr || root->type() == kAnd ||
           root->type() == kNull);
    if (root->type() == kOr || root->type() == kAnd)
      root->type(root->type() == kOr ? kAnd : kOr);
    root->NegateArgs();
    graph_->complement() = false;
  }
  std::unordered_map<int, GatePtr> complements;
  graph_->Clear<Pdag::kGateMark>();
  PropagateComplements(graph_->root(), false, &complements);
  complements.clear();
  LOG(DEBUG3) << "Complement propagation is done!";

  if (graph_->IsTrivial())
    return;
  LOG(DEBUG2) << "Continue with Phase II within Phase IV";
  RunPhaseTwo();
}

void Preprocessor::RunPhaseFive() noexcept {
  TIMER(DEBUG2, "Preprocessing Phase V");
  SANITY_ASSERT;
  graph_->Log();
  while (CoalesceGates(/*common=*/true))
    continue;

  if (graph_->IsTrivial())
    return;
  LOG(DEBUG2) << "Continue with Phase II within Phase V";
  RunPhaseTwo();
  if (graph_->IsTrivial())
    return;

  while (CoalesceGates(/*common=*/true))
    continue;

  if (graph_->IsTrivial())
    return;
  graph_->Log();
}

#undef SANITY_ASSERT

namespace {  // Helper functions for all preprocessing algorithms.

/// Detects overlap in ranges.
///
/// @param[in] a_min  The lower boundary of the first range.
/// @param[in] a_max  The upper boundary of the first range.
/// @param[in] b_min  The lower boundary of the second range.
/// @param[in] b_max  The upper boundary of the second range.
///
/// @returns true if there's overlap in the ranges.
bool DetectOverlap(int a_min, int a_max, int b_min, int b_max) noexcept {
  assert(a_min < a_max);
  assert(b_min < b_max);
  return a_min <= b_max && a_max >= b_min;
}

/// Checks if a node within a graph enter and exit times.
///
/// @param[in] node  The node to be tested.
/// @param[in] enter_time  The enter time of the root gate of the graph.
/// @param[in] exit_time  The exit time of the root gate of the graph.
///
/// @returns true if the node within the graph visit times.
bool IsNodeWithinGraph(const NodePtr& node, int enter_time,
                       int exit_time) noexcept {
  assert(enter_time > 0);
  assert(exit_time > enter_time);
  assert(node->EnterTime() >= 0);
  assert(node->LastVisit() >= node->EnterTime());
  return node->EnterTime() > enter_time && node->LastVisit() < exit_time;
}

/// Checks if a subgraph with a root gate is within a subgraph.
/// The positive result means
/// that all nodes of the subgraph is contained within the main graph.
///
/// @param[in] root  The root gate of the subgraph.
/// @param[in] enter_time  The enter time of the root gate of the graph.
/// @param[in] exit_time  The exit time of the root gate of the graph.
///
/// @returns true if the subgraph within the graph visit times.
bool IsSubgraphWithinGraph(const GatePtr& root, int enter_time,
                           int exit_time) noexcept {
  assert(enter_time > 0);
  assert(exit_time > enter_time);
  assert(root->min_time() > 0);
  assert(root->max_time() > root->min_time());
  return root->min_time() > enter_time && root->max_time() < exit_time;
}

}  // namespace

void Preprocessor::NormalizeGates(bool full) noexcept {
  TIMER(DEBUG3, (full ? "Full normalization" : "Partial normalization"));
  assert(!graph_->HasNullGates());
  if (full)
    pdag::TopologicalOrder(graph_);  // K/N gates need order.

  const GatePtr& root_gate = graph_->root();
  Operator type = root_gate->type();
  switch (type) {  // Handle special case for the root gate.
    case kNor:
    case kNand:
    case kNot:
      graph_->complement() ^= true;
      break;
    default:  // All other types keep the sign of the root.
      assert((type == kAnd || type == kOr || type == kVote ||
              type == kXor || type == kNull) &&
             "Update the logic if new gate types are introduced.");
  }
  // Process negative gates.
  // Note that root's negative gate is processed in the above lines.
  graph_->Clear<Pdag::kGateMark>();
  NotifyParentsOfNegativeGates(root_gate);

  graph_->Clear<Pdag::kGateMark>();
  NormalizeGate(root_gate, full);  // Registers null gates only.

  assert(!graph_->HasConstants());
  graph_->RemoveNullGates();
}

void Preprocessor::NotifyParentsOfNegativeGates(const GatePtr& gate) noexcept {
  if (gate->mark())
    return;
  gate->mark(true);
  gate->NegateNonCoherentGateArgs();
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    NotifyParentsOfNegativeGates(arg.second);
  }
}

void Preprocessor::NormalizeGate(const GatePtr& gate, bool full) noexcept {
  if (gate->mark())
    return;
  gate->mark(true);
  assert(!gate->constant());
  assert(!gate->args().empty());
  // Depth-first traversal before the arguments may get changed.
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    NormalizeGate(arg.second, full);
  }

  switch (gate->type()) {  // Negation is already processed.
    case kNor:
      assert(gate->args().size() > 1);
      gate->type(kOr);
      break;
    case kNand:
      assert(gate->args().size() > 1);
      gate->type(kAnd);
      break;
    case kXor:
      assert(gate->args().size() == 2);
      if (full)
        NormalizeXorGate(gate);
      break;
    case kVote:
      assert(gate->args().size() > 2);
      assert(gate->vote_number() > 1);
      if (full)
        NormalizeVoteGate(gate);
      break;
    case kNot:
      assert(gate->args().size() == 1);
      gate->type(kNull);
      break;
    default:  // Already normal gates.
      assert(gate->type() == kAnd || gate->type() == kOr);
      assert(gate->args().size() > 1);
  }
}

void Preprocessor::NormalizeXorGate(const GatePtr& gate) noexcept {
  assert(gate->args().size() == 2);
  auto gate_one = std::make_shared<Gate>(kAnd, graph_);
  auto gate_two = std::make_shared<Gate>(kAnd, graph_);
  gate_one->mark(true);
  gate_two->mark(true);

  gate->type(kOr);
  auto it = gate->args().begin();
  gate->ShareArg(*it, gate_one);
  gate->ShareArg(*it, gate_two);
  gate_two->NegateArg(*it);

  ++it;  // Handling the second argument.
  gate->ShareArg(*it, gate_one);
  gate_one->NegateArg(*it);
  gate->ShareArg(*it, gate_two);

  gate->EraseArgs();
  gate->AddArg(gate_one);
  gate->AddArg(gate_two);
}

void Preprocessor::NormalizeVoteGate(const GatePtr& gate) noexcept {
  assert(gate->type() == kVote);
  int vote_number = gate->vote_number();

  assert(vote_number > 0);  // Vote number can be 1 for special OR gates.
  assert(gate->args().size() > 1);
  if (gate->args().size() == vote_number) {
    gate->type(kAnd);
    return;
  } else if (vote_number == 1) {
    gate->type(kOr);
    return;
  }

  auto it = boost::max_element(gate->args(), [&gate](int lhs, int rhs) {
    return gate->GetArg(lhs)->order() < gate->GetArg(rhs)->order();
  });
  assert(it != gate->args().cend());
  auto first_arg = std::make_shared<Gate>(kAnd, graph_);
  gate->TransferArg(*it, first_arg);

  auto grand_arg = std::make_shared<Gate>(kVote, graph_);
  first_arg->AddArg(grand_arg);
  grand_arg->vote_number(vote_number - 1);

  auto second_arg = std::make_shared<Gate>(kVote, graph_);
  second_arg->vote_number(vote_number);

  for (int index : gate->args()) {
    gate->ShareArg(index, grand_arg);
    gate->ShareArg(index, second_arg);
  }

  first_arg->mark(true);
  second_arg->mark(true);
  grand_arg->mark(true);

  gate->type(kOr);
  gate->EraseArgs();
  gate->AddArg(first_arg);
  gate->AddArg(second_arg);

  NormalizeVoteGate(grand_arg);
  NormalizeVoteGate(second_arg);
}

void Preprocessor::PropagateComplements(
    const GatePtr& gate,
    bool keep_modules,
    std::unordered_map<int, GatePtr>* complements) noexcept {
  if (gate->mark())
    return;
  gate->mark(true);
  // If the argument gate is complement,
  // then create a new gate
  // that propagates its sign to its arguments
  // and itself becomes non-complement.
  // Keep track of complement gates
  // for optimization of repeated complements.
  std::vector<std::pair<int, GatePtr>> to_swap;  // Gate args with negation.
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    const GatePtr& arg_gate = arg.second;
    if ((arg.first > 0) || (keep_modules && arg_gate->module())) {
      PropagateComplements(arg_gate, keep_modules, complements);
      continue;
    }  // arg is complement and (not keep_modules or arg is not module).
    if (auto it = ext::find(*complements, arg_gate->index())) {
      to_swap.emplace_back(arg.first, it->second);
      assert(it->second->mark());
      continue;  // Existing complements are already processed.
    }
    Operator type = arg_gate->type();
    assert(type == kAnd || type == kOr);
    Operator complement_type = type == kOr ? kAnd : kOr;
    GatePtr complement;
    if (arg_gate->parents().size() == 1) {  // Optimization. Reuse.
      arg_gate->type(complement_type);
      arg_gate->NegateArgs();
      complement = arg_gate;
    } else {
      complement = arg_gate->Clone();
      if (arg_gate->module())
        arg_gate->module(false);  // Not good.
      complement->type(complement_type);
      complement->NegateArgs();
      complements->emplace(arg_gate->index(), complement);
    }
    to_swap.emplace_back(arg.first, complement);
    PropagateComplements(complement, keep_modules, complements);
  }

  for (const auto& arg : to_swap) {
    assert(arg.first < 0);
    gate->EraseArg(arg.first);
    gate->AddArg(arg.second);
    assert(!gate->constant() && "No duplicates are expected.");
  }
}

bool Preprocessor::CoalesceGates(bool common) noexcept {
  TIMER(DEBUG3, "Coalescing gates");
  assert(!graph_->HasNullGates());
  if (graph_->root()->constant())
    return false;
  graph_->Clear<Pdag::kGateMark>();
  bool ret = CoalesceGates(graph_->root(), common);

  graph_->RemoveNullGates();  // Actually, only constants are created.
  return ret;
}

bool Preprocessor::CoalesceGates(const GatePtr& gate, bool common) noexcept {
  if (gate->mark())
    return false;
  gate->mark(true);
  Operator target_type = kNull;  // What kind of arg gate are we searching for?
  switch (gate->type()) {
    case kNand:
    case kAnd:
      assert(gate->args().size() > 1);
      target_type = kAnd;
      break;
    case kNor:
    case kOr:
      assert(gate->args().size() > 1);
      target_type = kOr;
      break;
    default:
      target_type = kNull;  // Implicit no operation!
  }
  assert(!gate->args().empty());
  std::vector<GatePtr> to_join;  // Gate arguments of the same logic.
  bool changed = false;  // Indication if the graph is changed.
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    const GatePtr& arg_gate = arg.second;
    changed |= CoalesceGates(arg_gate, common);

    if (target_type == kNull)
      continue;  // Coalescing is impossible.
    if (arg_gate->constant())
      continue;  // No args to join.
    if (arg.first < 0)
      continue;  // Cannot coalesce a negative arg gate.
    if (arg_gate->module())
      continue;  // Preserve modules.
    if (!common && arg_gate->parents().size() > 1)
      continue;  // Check common.

    if (arg_gate->type() == target_type)
      to_join.push_back(arg_gate);
  }

  changed |= !to_join.empty();
  for (const GatePtr& arg : to_join) {
    gate->CoalesceGate(arg);
    if (gate->constant())
      break;  // The parent is constant. No need to join other arguments.
    assert(gate->args().size() > 1);  // Does not produce NULL type gates.
  }
  return changed;
}

bool Preprocessor::ProcessMultipleDefinitions() noexcept {
  assert(!graph_->HasNullGates());
  if (graph_->root()->constant())
    return false;

  TIMER(DEBUG3, "Detecting multiple definitions");

  graph_->Clear<Pdag::kGateMark>();
  // The original gate and its multiple definitions.
  std::unordered_map<GatePtr, std::vector<GateWeakPtr>> multi_def;
  {
    GateSet unique_gates;
    DetectMultipleDefinitions(graph_->root(), &multi_def, &unique_gates);
  }
  graph_->Clear<Pdag::kGateMark>();

  if (multi_def.empty())
    return false;
  LOG(DEBUG4) << multi_def.size() << " gates are multiply defined.";
  for (const auto& def : multi_def) {
    LOG(DEBUG5) << "Gate " << def.first->index() << ": " << def.second.size()
                << " times.";
    for (const GateWeakPtr& duplicate : def.second) {
      if (duplicate.expired())
        continue;
      ReplaceGate(duplicate.lock(), def.first);
    }
  }
  graph_->RemoveNullGates();
  return true;
}

void Preprocessor::DetectMultipleDefinitions(
    const GatePtr& gate,
    std::unordered_map<GatePtr, std::vector<GateWeakPtr>>* multi_def,
    GateSet* unique_gates) noexcept {
  if (gate->mark())
    return;
  gate->mark(true);
  assert(!gate->constant());

  if (!gate->module()) {  // Modules are unique by definition.
    std::pair<GatePtr, bool> ret = unique_gates->insert(gate);
    assert(ret.first->mark());
    if (!ret.second) {  // The gate is duplicate.
      (*multi_def)[ret.first].push_back(gate);
      return;
    }
  }
  // No redefinition is found for this gate.
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    DetectMultipleDefinitions(arg.second, multi_def, unique_gates);
  }
}

void Preprocessor::DetectModules() noexcept {
  TIMER(DEBUG3, "Module detection");
  assert(!graph_->HasNullGates());
  const GatePtr& root_gate = graph_->root();  // No change in this algorithm.
  // First stage, traverse the graph depth-first for gates
  // and indicate visit time for each node.
  LOG(DEBUG4) << "Assigning timings to nodes...";
  graph_->Clear<Pdag::kVisit>();
  AssignTiming(0, root_gate);
  LOG(DEBUG4) << "Timings are assigned to nodes.";

  graph_->Clear<Pdag::kGateMark>();
  FindModules(root_gate);

  assert(!root_gate->Revisited());  // Sanity checks.
  assert(root_gate->min_time() == 1);
  assert(root_gate->max_time() == root_gate->ExitTime());
}

int Preprocessor::AssignTiming(int time, const GatePtr& gate) noexcept {
  if (gate->Visit(++time))
    return time;  // Revisited gate.
  assert(!gate->constant());

  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    time = AssignTiming(time, arg.second);
  }
  for (const Gate::Arg<Variable>& arg : gate->args<Variable>()) {
    arg.second->Visit(++time);  // Enter the leaf.
    arg.second->Visit(time);  // Exit at the same time.
  }
  bool re_visited = gate->Visit(++time);  // Exiting the gate in second visit.
  assert(!re_visited && "Detected a cycle!");  // No cyclic visiting.
  return time;
}

void Preprocessor::FindModules(const GatePtr& gate) noexcept {
  if (gate->mark())
    return;
  gate->mark(true);
  int enter_time = gate->EnterTime();
  int exit_time = gate->ExitTime();
  int min_time = enter_time;
  int max_time = exit_time;

  std::vector<std::pair<int, NodePtr>> non_shared_args;
  std::vector<std::pair<int, NodePtr>> modular_args;
  std::vector<std::pair<int, NodePtr>> non_modular_args;

  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    const GatePtr& arg_gate = arg.second;
    FindModules(arg_gate);
    if (arg_gate->module() && !arg_gate->Revisited()) {
      assert(arg_gate->parents().size() == 1);
      assert(arg_gate->parents().count(gate->index()));
      assert(IsSubgraphWithinGraph(arg_gate, enter_time, exit_time));

      non_shared_args.push_back(arg);
      continue;  // Sub-graph's visit times are within the Enter and Exit time.
    }
    if (IsSubgraphWithinGraph(arg_gate, enter_time, exit_time)) {
      modular_args.push_back(arg);
    } else {
      non_modular_args.push_back(arg);
      min_time = std::min(min_time, arg_gate->min_time());
      max_time = std::max(max_time, arg_gate->max_time());
    }
  }

  for (const Gate::Arg<Variable>& arg : gate->args<Variable>()) {
    const VariablePtr& var = arg.second;
    if (var->parents().size() == 1) {
      assert(IsNodeWithinGraph(var, enter_time, exit_time));
      assert(var->parents().count(gate->index()));

      non_shared_args.push_back(arg);
      continue;  // The single parent argument.
    }
    if (IsNodeWithinGraph(var, enter_time, exit_time)) {
      modular_args.push_back(arg);
    } else {
      non_modular_args.push_back(arg);
      min_time = std::min(min_time, var->EnterTime());
      max_time = std::max(max_time, var->LastVisit());
    }
  }

  // Determine if this gate is module itself.
  if (!gate->module() && min_time == enter_time && max_time == exit_time) {
    LOG(DEBUG4) << "Found original module: G" << gate->index();
    assert(non_modular_args.empty());
    gate->module(true);
  }

  max_time = std::max(max_time, gate->LastVisit());
  gate->min_time(min_time);
  gate->max_time(max_time);

  ProcessModularArgs(gate, non_shared_args, &modular_args, &non_modular_args);
}

void Preprocessor::ProcessModularArgs(
    const GatePtr& gate,
    const std::vector<std::pair<int, NodePtr>>& non_shared_args,
    std::vector<std::pair<int, NodePtr>>* modular_args,
    std::vector<std::pair<int, NodePtr>>* non_modular_args) noexcept {
  assert(gate->args().size() ==
         (non_shared_args.size() + modular_args->size() +
          non_modular_args->size()) &&
         "Module detection has messed up grouping of arguments.");
  // Attempting to create new modules for specific gate types.
  switch (gate->type()) {
    case kNor:
    case kOr:
    case kNand:
    case kAnd: {
      CreateNewModule(gate, non_shared_args);

      FilterModularArgs(modular_args, non_modular_args);
      assert(modular_args->size() != 1 && "One modular arg is non-shared.");
      std::vector<std::vector<std::pair<int, NodePtr>>> groups;
      GroupModularArgs(modular_args, &groups);
      CreateNewModules(gate, *modular_args, groups);
      break;
    }
    default:
      assert("More complex gates are considered impossible to sub-modularize!");
  }
}

GatePtr Preprocessor::CreateNewModule(
    const GatePtr& gate,
    const std::vector<std::pair<int, NodePtr>>& args) noexcept {
  GatePtr module;  // Empty pointer as an indication of a failure.
  if (args.empty())
    return module;
  if (args.size() == 1)
    return module;
  if (args.size() == gate->args().size()) {
    assert(gate->module());
    return module;
  }
  assert(args.size() < gate->args().size());
  switch (gate->type()) {
    case kNand:
    case kAnd:
      module = std::make_shared<Gate>(kAnd, graph_);
      break;
    case kNor:
    case kOr:
      module = std::make_shared<Gate>(kOr, graph_);
      break;
    default:
      return module;  // Cannot create sub-modules for other types.
  }
  module->module(true);
  assert(gate->mark());
  module->mark(true);  // Keep consistent marking with the subgraph.
  for (const auto& arg : args) {
    gate->TransferArg(arg.first, module);
  }
  gate->AddArg(module);
  assert(gate->args().size() > 1);
  LOG(DEBUG4) << "Created a module G" << module->index() << " with "
              << args.size() << " arguments for G" << gate->index();
  return module;
}

void Preprocessor::FilterModularArgs(
    std::vector<std::pair<int, NodePtr>>* modular_args,
    std::vector<std::pair<int, NodePtr>>* non_modular_args) noexcept {
  if (modular_args->empty() || non_modular_args->empty())
    return;
  auto it_end = modular_args->end();
  for (auto it_begin = modular_args->begin(),
            check_begin = non_modular_args->begin(),
            check_end = non_modular_args->end();
       check_begin != check_end;) {
    check_begin = std::partition(
        it_begin, it_end,
        [check_begin, check_end](const std::pair<int, NodePtr>& mod_arg) {
          return std::none_of(check_begin, check_end,
                              [&mod_arg](const std::pair<int, NodePtr>& arg) {
            return DetectOverlap(mod_arg.second->min_time(),
                                 mod_arg.second->max_time(),
                                 arg.second->min_time(),
                                 arg.second->max_time());
          });
        });
    check_end = it_end;
    it_end = check_begin;
  }
  non_modular_args->insert(non_modular_args->end(), it_end,
                           modular_args->end());
  modular_args->erase(it_end, modular_args->end());
}

void Preprocessor::GroupModularArgs(
    std::vector<std::pair<int, NodePtr>>* modular_args,
    std::vector<std::vector<std::pair<int, NodePtr>>>* groups) noexcept {
  if (modular_args->empty())
    return;
  assert(modular_args->size() > 1);
  assert(groups->empty());
  // Group nodes by the intersection of their visit ranges.
  boost::sort(*modular_args, [](const std::pair<int, NodePtr>& lhs_pair,
                                const std::pair<int, NodePtr>& rhs_pair) {
    const NodePtr& lhs_node = lhs_pair.second;
    const NodePtr& rhs_node = rhs_pair.second;
    if (lhs_node->max_time() < rhs_node->min_time())
      return true;
    if (lhs_node->min_time() > rhs_node->max_time())
      return false;
    // Considering the intersection.
    if (lhs_node->max_time() < rhs_node->max_time())
      return true;
    if (lhs_node->max_time() > rhs_node->max_time())
      return false;
    return lhs_node->min_time() > rhs_node->min_time();
  });
  // Gather intersections into groups.
  for (auto it = modular_args->rbegin(), it_end = modular_args->rend();
       it != it_end;) {
    int min_time = it->second->min_time();
    auto it_group_end =
        std::find_if(it + 1, it_end,
                     [it, &min_time](const std::pair<int, NodePtr>& arg) {
          assert(it->second->max_time() >= arg.second->max_time());
          if (min_time <= arg.second->max_time()) {
            min_time = std::min(min_time, arg.second->min_time());
            return false;
          }
          return true;  // Found first element not in the group.
        });
    assert(it_group_end - it > 1);
    groups->emplace_back(it, it_group_end);
    it = it_group_end;  // Continue with the next group.
  }
  LOG(DEBUG4) << "Grouped modular args in " << groups->size() << " group(s).";
  assert(!groups->empty());
}

void Preprocessor::CreateNewModules(
    const GatePtr& gate,
    const std::vector<std::pair<int, NodePtr>>& modular_args,
    const std::vector<std::vector<std::pair<int, NodePtr>>>& groups) noexcept {
  if (modular_args.empty())
    return;
  assert(modular_args.size() > 1);
  assert(!groups.empty());
  if (modular_args.size() == gate->args().size() && groups.size() == 1) {
    assert(gate->module());
    return;
  }
  GatePtr main_arg;

  if (modular_args.size() == gate->args().size()) {
    assert(groups.size() > 1);
    assert(gate->module());
    main_arg = gate;
  } else {
    main_arg = CreateNewModule(gate, modular_args);
    assert(main_arg);
  }
  for (const auto& group : groups) {
    CreateNewModule(main_arg, group);
  }
}

std::vector<GateWeakPtr> Preprocessor::GatherModules() noexcept {
  graph_->Clear<Pdag::kGateMark>();
  const GatePtr& root = graph_->root();
  assert(!root->mark());
  assert(root->module());
  root->mark(true);
  std::vector<GateWeakPtr> modules;
  modules.push_back(root);
  std::queue<Gate*> gates_queue;
  gates_queue.push(root.get());
  while (!gates_queue.empty()) {
    Gate* gate = gates_queue.front();
    gates_queue.pop();
    assert(gate->mark());
    for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
      const GatePtr& arg_gate = arg.second;
      assert(!arg_gate->constant());
      if (arg_gate->mark())
        continue;
      arg_gate->mark(true);
      gates_queue.push(arg_gate.get());
      if (arg_gate->module())
        modules.push_back(arg_gate);
    }
  }
  return modules;
}

bool Preprocessor::MergeCommonArgs() noexcept {
  TIMER(DEBUG3, "Merging common arguments");
  assert(!graph_->HasNullGates());
  bool changed = false;

  LOG(DEBUG4) << "Merging common arguments for AND gates...";
  changed |= MergeCommonArgs(kAnd);
  LOG(DEBUG4) << "Finished merging for AND gates!";

  LOG(DEBUG4) << "Merging common arguments for OR gates...";
  changed |= MergeCommonArgs(kOr);
  LOG(DEBUG4) << "Finished merging for OR gates!";

  assert(!graph_->HasNullGates());
  return changed;
}

bool Preprocessor::MergeCommonArgs(Operator op) noexcept {
  assert(op == kAnd || op == kOr);
  graph_->Clear<Pdag::kCount>();
  graph_->Clear<Pdag::kGateMark>();
  // Gather and group gates
  // by their operator types and common arguments.
  MarkCommonArgs(graph_->root(), op);
  graph_->Clear<Pdag::kGateMark>();
  std::vector<GateWeakPtr> modules = GatherModules();
  graph_->Clear<Pdag::kGateMark>();
  LOG(DEBUG4) << "Working with " << modules.size() << " modules...";
  bool changed = false;
  for (const auto& module : modules) {
    if (module.expired())
      continue;
    GatePtr root = module.lock();
    MergeTable::Candidates candidates;
    GatherCommonArgs(root, op, &candidates);
    graph_->Clear<Pdag::kGateMark>(root);
    if (candidates.size() < 2)
      continue;
    FilterMergeCandidates(&candidates);
    if (candidates.size() < 2)
      continue;
    std::vector<MergeTable::Candidates> groups;
    GroupCandidatesByArgs(&candidates, &groups);
    for (const auto& group : groups) {
      // Finding common parents for the common arguments.
      MergeTable::Collection parents;
      GroupCommonParents(2, group, &parents);
      if (parents.empty())
        continue;  // No candidates for merging.
      changed = true;
      LOG(DEBUG4) << "Merging " << parents.size() << " collection...";
      MergeTable table;
      GroupCommonArgs(parents, &table);
      LOG(DEBUG4) << "Transforming " << table.groups.size()
                  << " table groups...";
      for (MergeTable::MergeGroup& member_group : table.groups) {
        TransformCommonArgs(&member_group);
      }
    }
    graph_->RemoveNullGates();
  }
  return changed;
}

void Preprocessor::MarkCommonArgs(const GatePtr& gate, Operator op) noexcept {
  if (gate->mark())
    return;
  gate->mark(true);

  bool in_group = gate->type() == op;

  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    const GatePtr& arg_gate = arg.second;
    assert(!arg_gate->constant());
    MarkCommonArgs(arg_gate, op);
    if (in_group)
      arg_gate->AddCount(arg.first > 0);
  }

  if (!in_group)
    return;  // No need to visit leaf variables.

  for (const Gate::Arg<Variable>& arg : gate->args<Variable>()) {
    arg.second->AddCount(arg.first > 0);
  }
  assert(!gate->constant());
}

void Preprocessor::GatherCommonArgs(const GatePtr& gate, Operator op,
                                    MergeTable::Candidates* group) noexcept {
  if (gate->mark())
    return;
  gate->mark(true);

  bool in_group = gate->type() == op;

  std::vector<int> common_args;
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    const GatePtr& arg_gate = arg.second;
    assert(!arg_gate->constant());
    if (!arg_gate->module())
      GatherCommonArgs(arg_gate, op, group);
    if (!in_group)
      continue;
    int count = arg.first > 0 ? arg_gate->pos_count() : arg_gate->neg_count();
    if (count > 1)
      common_args.push_back(arg.first);
  }

  if (!in_group)
    return;  // No need to check variables.

  for (const Gate::Arg<Variable>& arg : gate->args<Variable>()) {
    const VariablePtr& var = arg.second;
    int count = arg.first > 0 ? var->pos_count() : var->neg_count();
    if (count > 1)
      common_args.push_back(arg.first);
  }
  assert(!gate->constant());

  if (common_args.size() < 2)
    return;  // Can't be merged anyway.

  boost::sort(common_args);  // Unique and stable.
  group->emplace_back(gate, common_args);
}

void Preprocessor::FilterMergeCandidates(
    MergeTable::Candidates* candidates) noexcept {
  assert(candidates->size() > 1);
  boost::stable_sort(*candidates, [](const MergeTable::Candidate& lhs,
                                     const MergeTable::Candidate& rhs) {
    return lhs.second.size() < rhs.second.size();
  });
  bool cleanup = false;  // Clean constant or NULL type gates.
  for (auto it = candidates->begin(); it != candidates->end(); ++it) {
    const GatePtr& gate = it->first;
    MergeTable::CommonArgs& common_args = it->second;
    if (gate->args().size() == 1)
      continue;
    if (gate->constant())
      continue;
    if (common_args.size() < 2)
      continue;
    if (gate->args().size() != common_args.size())
      continue;  // Not perfect.
    auto it_next = it;
    for (++it_next; it_next != candidates->end(); ++it_next) {
      const GatePtr& comp_gate = it_next->first;
      MergeTable::CommonArgs& comp_args = it_next->second;
      if (comp_args.size() < common_args.size())
        continue;  // Changed gate.
      assert(!comp_gate->constant());
      if (!boost::includes(comp_args, common_args))
        continue;

      MergeTable::CommonArgs diff;
      boost::set_difference(comp_args, common_args, std::back_inserter(diff));
      diff.push_back(gate->index());
      diff.erase(boost::unique(boost::sort(diff)).end(), diff.end());
      comp_args = std::move(diff);
      for (int index : common_args)
        comp_gate->EraseArg(index);
      comp_gate->AddArg(gate);
      if (comp_gate->constant()) {  // Complement of gate is arg.
        comp_args.clear();
        cleanup = true;
      } else if (comp_gate->args().size() == 1) {  // Perfect substitution.
        comp_gate->type(kNull);
        assert(comp_args.size() == 1);
        cleanup = true;
      } else if (comp_args.size() == 1) {
        cleanup = true;
      }
    }
  }
  if (!cleanup)
    return;
  boost::remove_erase_if(*candidates, [](const MergeTable::Candidate& mem) {
    return mem.first->constant() || mem.first->type() == kNull ||
           mem.second.size() == 1;
  });
}

void Preprocessor::GroupCandidatesByArgs(
    MergeTable::Candidates* candidates,
    std::vector<MergeTable::Candidates>* groups) noexcept {
  if (candidates->empty())
    return;
  assert(candidates->size() > 1);
  assert(groups->empty());
  // Group candidates by the intersection of their [min_arg, max_arg] ranges.
  boost::sort(*candidates, [](const MergeTable::Candidate& lhs_candidate,
                              const MergeTable::Candidate& rhs_candidate) {
    const MergeTable::CommonArgs& lhs_args = lhs_candidate.second;
    const MergeTable::CommonArgs& rhs_args = rhs_candidate.second;
    if (lhs_args.back() < rhs_args.front())
      return true;
    if (lhs_args.front() > rhs_args.back())
      return false;
    // Considering the intersection.
    if (lhs_args.back() < rhs_args.back())
      return true;
    if (lhs_args.back() > rhs_args.back())
      return false;
    return lhs_args.front() > rhs_args.front();
  });

  std::vector<std::list<MergeTable::Candidate*>> super_groups;
  for (auto it = candidates->rbegin(), it_end = candidates->rend();
       it != it_end;) {
    int min_time = it->second.front();
    std::list<MergeTable::Candidate*> super_group = {&*it};
    for (++it; it != it_end && min_time <= it->second.back(); ++it) {
      min_time = std::min(min_time, it->second.front());
      super_group.push_back(&*it);
    }
    super_groups.emplace_back(std::move(super_group));
  }

  for (auto& super_group : super_groups) {
    while (!super_group.empty()) {
      MergeTable::Candidates group;
      const MergeTable::CommonArgs& common_args = super_group.front()->second;
      std::set<int> group_args(common_args.begin(), common_args.end());
      assert(group_args.size() > 1);
      group.push_back(*super_group.front());
      super_group.pop_front();

      int prev_size = 0;  // To track the change in group arguments.
      while (prev_size < group_args.size()) {
        prev_size = group_args.size();
        for (auto it = super_group.begin(); it != super_group.end();) {
          const MergeTable::CommonArgs& member_args = (*it)->second;
          if (ext::intersects(member_args, group_args)) {
            group.push_back(**it);
            group_args.insert(member_args.begin(), member_args.end());
            it = super_group.erase(it);
          } else {
            ++it;
          }
        }
      }
      if (group.size() > 1)
        groups->emplace_back(std::move(group));
    }
  }
  BLOG(DEBUG4, !groups->empty()) << "Grouped merge candidates in "
                                 << groups->size() << " group(s).";
}

void Preprocessor::GroupCommonParents(
    int num_common_args,
    const MergeTable::Candidates& group,
    MergeTable::Collection* parents) noexcept {
  for (int i = 0; i < group.size(); ++i) {
    const std::vector<int>& args_gate = group[i].second;
    assert(args_gate.size() > 1);
    int j = i;
    for (++j; j < group.size(); ++j) {
      const std::vector<int>& args_comp = group[j].second;
      assert(args_comp.size() > 1);

      std::vector<int> common;
      boost::set_intersection(args_gate, args_comp, std::back_inserter(common));
      if (common.size() < num_common_args)
        continue;  // Doesn't satisfy.
      MergeTable::CommonParents& common_parents = (*parents)[common];
      common_parents.insert(group[i].first);
      common_parents.insert(group[j].first);
    }
  }
}

void Preprocessor::GroupCommonArgs(const MergeTable::Collection& options,
                                   MergeTable* table) noexcept {
  assert(!options.empty());
  MergeTable::MergeGroup all_options(options.begin(), options.end());
  boost::stable_sort(all_options, [](const MergeTable::Option& lhs,
                                     const MergeTable::Option& rhs) {
    return lhs.first.size() < rhs.first.size();
  });

  while (!all_options.empty()) {
    MergeTable::OptionGroup best_group;
    FindOptionGroup(&all_options, &best_group);
    assert(!best_group.empty());
    MergeTable::MergeGroup merge_group;  // The group to go into the table.
    for (MergeTable::Option* member : best_group) {
      merge_group.push_back(*member);
      member->second.clear();  // To remove the best group from the all options.
    }
    table->groups.push_back(merge_group);

    const MergeTable::CommonParents& gates = merge_group.front().second;
    /// @todo This strategy deletes too many groups.
    ///       The intersections must be considered for each option.
    const MergeTable::CommonArgs& args = merge_group.back().first;
    for (MergeTable::Option& option : all_options) {
      if (!ext::intersects(option.first, args))
        continue;  // Doesn't affect this option.
      MergeTable::CommonParents& parents = option.second;
      for (const GatePtr& gate : gates)
        parents.erase(gate);
    }
    boost::remove_erase_if(all_options, [](const MergeTable::Option& option) {
      return option.second.size() < 2;
    });
  }
}

void Preprocessor::FindOptionGroup(
    MergeTable::MergeGroup* all_options,
    MergeTable::OptionGroup* best_group) noexcept {
  assert(best_group->empty());
  // Find the best starting option.
  MergeTable::MergeGroup::iterator best_option;
  FindBaseOption(all_options, &best_option);
  bool best_is_found = best_option != all_options->end();
  auto it = best_is_found ? best_option : all_options->begin();
  for (; it != all_options->end(); ++it) {
    MergeTable::OptionGroup group = {&*it};
    auto it_next = it;
    for (++it_next; it_next != all_options->end(); ++it_next) {
      MergeTable::Option* candidate = &*it_next;
      bool superset = boost::includes(candidate->first, group.back()->first);
      if (!superset)
        continue;  // Does not include all the arguments.
      bool parents = boost::includes(group.back()->second, candidate->second);
      if (!parents)
        continue;  // Parents do not match.
      group.push_back(candidate);
    }
    if (group.size() > best_group->size()) {  // The more members, the merrier.
      *best_group = group;
    } else if (group.size() == best_group->size()) {  // Optimistic choice.
      if (group.front()->second.size() < best_group->front()->second.size())
        *best_group = group;  // The fewer parents, the more room for others.
    }
    if (best_is_found)
      break;
  }
}

void Preprocessor::FindBaseOption(
    MergeTable::MergeGroup* all_options,
    MergeTable::MergeGroup::iterator* best_option) noexcept {
  *best_option = all_options->end();
  std::array<int, 3> best_counts{};  // The number of extra parents.
  for (auto it = all_options->begin(); it != all_options->end(); ++it) {
    int num_parents = it->second.size();
    const GatePtr& parent = *it->second.begin();  // Representative.
    const MergeTable::CommonArgs& args = it->first;
    std::array<int, 3> cur_counts{};
    for (int index : args) {
      NodePtr arg = parent->GetArg(index);
      int extra_count = arg->parents().size() - num_parents;
      if (extra_count > 2)
        continue;                 // Optimal decision criterion.
      ++cur_counts[extra_count];  // Logging extra parents.
      if (cur_counts[0] > 1)
        break;  // Modular option is found.
    }
    if (cur_counts[0] > 1) {  // Special case of modular options.
      *best_option = it;
      break;
    }
    if ((cur_counts[0] > best_counts[0]) ||
        (cur_counts[0] == best_counts[0] && cur_counts[1] > best_counts[1]) ||
        (cur_counts[0] == best_counts[0] && cur_counts[1] == best_counts[1] &&
         cur_counts[2] > best_counts[2])) {
      *best_option = it;
      best_counts = cur_counts;
    }
  }
}

void Preprocessor::TransformCommonArgs(MergeTable::MergeGroup* group) noexcept {
  MergeTable::MergeGroup::iterator it;
  for (it = group->begin(); it != group->end(); ++it) {
    MergeTable::CommonParents& common_parents = it->second;
    MergeTable::CommonArgs& common_args = it->first;
    assert(common_parents.size() > 1);
    assert(common_args.size() > 1);

    LOG(DEBUG5) << "Merging " << common_args.size() << " args into a new gate";
    LOG(DEBUG5) << "The number of common parents: " << common_parents.size();
    const GatePtr& parent = *common_parents.begin();  // To get the arguments.
    assert(parent->args().size() > 1);
    auto merge_gate = std::make_shared<Gate>(parent->type(), graph_);
    for (int index : common_args) {
      parent->ShareArg(index, merge_gate);
      for (const GatePtr& common_parent : common_parents) {
        common_parent->EraseArg(index);
      }
    }
    for (const GatePtr& common_parent : common_parents) {
      common_parent->AddArg(merge_gate);
      if (common_parent->args().size() == 1) {
        common_parent->type(kNull);  // Assumes AND/OR gates only.
      }
      assert(!common_parent->constant());
    }
    // Substitute args in superset common args with the new gate.
    MergeTable::MergeGroup::iterator it_rest = it;
    for (++it_rest; it_rest != group->end(); ++it_rest) {
      MergeTable::CommonArgs& set_args = it_rest->first;
      assert(set_args.size() > common_args.size());
      // Note: it is assumed that common_args is a proper subset of set_args.
      std::vector<int> diff;
      boost::set_difference(set_args, common_args, std::back_inserter(diff));
      assert(diff.size() == (set_args.size() - common_args.size()));
      assert(merge_gate->index() > diff.back());
      diff.push_back(merge_gate->index());  // Assumes sequential indexing.
      set_args = diff;
      assert(it_rest->first.size() == diff.size());
    }
  }
}

bool Preprocessor::DetectDistributivity() noexcept {
  TIMER(DEBUG3, "Processing Distributivity");
  assert(!graph_->HasNullGates());
  graph_->Clear<Pdag::kGateMark>();
  bool changed = DetectDistributivity(graph_->root());
  assert(!graph_->HasConstants());
  graph_->RemoveNullGates();
  return changed;
}

bool Preprocessor::DetectDistributivity(const GatePtr& gate) noexcept {
  if (gate->mark())
    return false;
  gate->mark(true);
  assert(!gate->constant());
  bool changed = false;
  Operator distr_type = kNull;  // Implicit flag of no operation!
  switch (gate->type()) {
    case kAnd:
    case kNand:
      distr_type = kOr;
      break;
    case kOr:
    case kNor:
      distr_type = kAnd;
      break;
    default:
      distr_type = kNull;
  }
  std::vector<GatePtr> candidates;
  // Collect child gates of distributivity type.
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    const GatePtr& child_gate = arg.second;
    changed |= DetectDistributivity(child_gate);
    assert(!child_gate->constant() && "Impossible state.");
    if (distr_type == kNull)
      continue;  // Distributivity is not possible.
    if (arg.first < 0)
      continue;  // Does not work on negation.
    if (child_gate->module())
      continue;  // Can't have common arguments.
    if (child_gate->type() == distr_type)
      candidates.push_back(child_gate);
  }
  changed |= HandleDistributiveArgs(gate, distr_type, &candidates);
  return changed;
}

bool Preprocessor::HandleDistributiveArgs(
    const GatePtr& gate,
    Operator distr_type,
    std::vector<GatePtr>* candidates) noexcept {
  if (candidates->empty())
    return false;
  assert(gate->args().size() > 1 && "Malformed parent gate.");
  assert(distr_type == kAnd || distr_type == kOr);
  bool changed = FilterDistributiveArgs(gate, candidates);
  if (candidates->size() < 2)
    return changed;
  // Detecting a combination
  // that gives the most optimization is combinatorial.
  // The problem is similar to merging common arguments of gates.
  MergeTable::Candidates all_candidates;
  for (const GatePtr& candidate : *candidates) {
    all_candidates.emplace_back(
        candidate,
        std::vector<int>(candidate->args().begin(), candidate->args().end()));
  }
  LOG(DEBUG5) << "Considering " << all_candidates.size() << " candidates...";
  MergeTable::Collection options;
  GroupCommonParents(1, all_candidates, &options);
  if (options.empty())
    return false;
  LOG(DEBUG4) << "Got " << options.size() << " distributive option(s).";

  MergeTable table;
  GroupDistributiveArgs(options, &table);
  assert(!table.groups.empty());
  LOG(DEBUG4) << "Found " << table.groups.size() << " distributive group(s).";
  // Sanitize the table with single parent gates only.
  for (MergeTable::MergeGroup& group : table.groups) {
    MergeTable::Option& base_option = group.front();
    std::vector<std::pair<GatePtr, GatePtr>> to_swap;
    for (const GatePtr& member : base_option.second) {
      assert(!member->parents().empty());
      if (member->parents().size() > 1) {
        GatePtr clone = member->Clone();
        clone->mark(true);
        to_swap.emplace_back(member, clone);
      }
    }
    for (const auto& gates : to_swap) {
      gate->EraseArg(gates.first->index());
      gate->AddArg(gates.second);
      for (MergeTable::Option& option : group) {
        if (option.second.erase(gates.first)) {
          option.second.insert(gates.second);
        }
      }
    }
  }

  for (MergeTable::MergeGroup& group : table.groups) {
    TransformDistributiveArgs(gate, distr_type, &group);
  }
  assert(!gate->args().empty());
  return true;
}

bool Preprocessor::FilterDistributiveArgs(
    const GatePtr& gate,
    std::vector<GatePtr>* candidates) noexcept {
  assert(!candidates->empty());
  // Handling a special case of fast constant propagation.
  std::vector<int> to_erase;  // Late erase for more opportunities.
  for (const GatePtr& candidate : *candidates) {  // All of them are positive.
    if (ext::intersects(candidate->args(), gate->args()))
      to_erase.push_back(candidate->index());
  }
  bool changed = !to_erase.empty();
  boost::remove_erase_if(*candidates, [&to_erase](const GatePtr& candidate) {
    return boost::find(to_erase, candidate->index()) != to_erase.end();
  });
  for (int index : to_erase)
    gate->EraseArg(index);

  // Sort in descending size of gate arguments.
  boost::sort(*candidates, [](const GatePtr& lhs, const GatePtr rhs) {
    return lhs->args().size() > rhs->args().size();
  });
  std::vector<GatePtr> exclusive;  // No candidate is a subset of another.
  while (!candidates->empty()) {
    GatePtr sub = std::move(candidates->back());
    candidates->pop_back();
    exclusive.push_back(sub);
    for (const GatePtr& super : *candidates) {
      if (boost::includes(super->args(), sub->args())) {
        changed = true;
        gate->EraseArg(super->index());
      }
    }
    boost::remove_erase_if(*candidates, [&sub](const GatePtr& super) {
      return boost::includes(super->args(), sub->args());
    });
  }
  *candidates = std::move(exclusive);
  assert(!gate->args().empty());
  if (gate->args().size() == 1) {
    switch (gate->type()) {
      case kAnd:
      case kOr:
        gate->type(kNull);
        break;
      case kNand:
      case kNor:
        gate->type(kNot);
        break;
      default:
        assert(false);
    }
    return true;
  }
  return changed;
}

void Preprocessor::GroupDistributiveArgs(const MergeTable::Collection& options,
                                         MergeTable* table) noexcept {
  assert(!options.empty());
  MergeTable::MergeGroup all_options(options.begin(), options.end());
  boost::stable_sort(all_options, [](const MergeTable::Option& lhs,
                                     const MergeTable::Option& rhs) {
    return lhs.first.size() < rhs.first.size();
  });

  while (!all_options.empty()) {
    MergeTable::OptionGroup best_group;
    FindOptionGroup(&all_options, &best_group);
    MergeTable::MergeGroup merge_group;  // The group to go into the table.
    for (MergeTable::Option* member : best_group) {
      merge_group.push_back(*member);
      member->second.clear();  // To remove the best group from the all options.
    }
    table->groups.push_back(merge_group);

    const MergeTable::CommonParents& gates = merge_group.front().second;
    for (MergeTable::Option& option : all_options) {
      MergeTable::CommonParents& parents = option.second;
      for (const GatePtr& gate : gates)
        parents.erase(gate);
    }
    boost::remove_erase_if(all_options, [](const MergeTable::Option& option) {
      return option.second.size() < 2;
    });
  }
}

void Preprocessor::TransformDistributiveArgs(
    const GatePtr& gate,
    Operator distr_type,
    MergeTable::MergeGroup* group) noexcept {
  if (group->empty())
    return;
  assert(distr_type == kAnd || distr_type == kOr);
  const MergeTable::Option& base_option = group->front();
  const MergeTable::CommonArgs& args = base_option.first;
  const MergeTable::CommonParents& gates = base_option.second;

  GatePtr new_parent;
  if (gate->args().size() == gates.size()) {
    new_parent = gate;  // Reuse the gate to avoid extra merging operations.
    switch (gate->type()) {
      case kAnd:
      case kOr:
        gate->type(distr_type);
        break;
      case kNand:
        gate->type(kNor);
        break;
      case kNor:
        gate->type(kNand);
        break;
      default:
        assert(false && "Gate is not suited for distributive operations.");
    }
  } else {
    new_parent = std::make_shared<Gate>(distr_type, graph_);
    new_parent->mark(true);
    gate->AddArg(new_parent);
  }

  auto sub_parent =
      std::make_shared<Gate>(distr_type == kAnd ? kOr : kAnd, graph_);
  sub_parent->mark(true);
  new_parent->AddArg(sub_parent);

  const GatePtr& rep = *gates.begin();  // Representative of common parents.
  // Getting the common part of the distributive equation.
  for (int index : args) rep->ShareArg(index, new_parent);  // May be negative.

  // Removing the common part from the sub-equations.
  for (const GatePtr& member : gates) {
    assert(member->parents().size() == 1);
    gate->EraseArg(member->index());

    sub_parent->AddArg(member);
    for (int index : args)
      member->EraseArg(index);

    assert(!member->args().empty());  // Assumes that filtering is done.
    if (member->args().size() == 1) {
      member->type(kNull);
    }
  }
  // Cleaning the arguments from the group.
  for (auto it = std::next(group->begin()); it != group->end(); ++it) {
    MergeTable::Option& super = *it;
    MergeTable::CommonArgs& super_args = super.first;
    boost::remove_erase_if(super_args, [&args](int index) {
      return boost::binary_search(args, index);
    });
  }
  group->erase(group->begin());
  TransformDistributiveArgs(sub_parent, distr_type, group);
}

void Preprocessor::BooleanOptimization() noexcept {
  TIMER(DEBUG3, "Boolean optimization");
  assert(!graph_->HasNullGates());
  graph_->Clear<Pdag::kGateMark>();
  graph_->Clear<Pdag::kOptiValue>();
  graph_->Clear<Pdag::kDescendant>();
  if (graph_->root()->module() == false)
    graph_->root()->module(true);

  std::vector<GateWeakPtr> common_gates;
  std::vector<std::weak_ptr<Variable>> common_variables;
  GatherCommonNodes(&common_gates, &common_variables);
  for (const auto& gate : common_gates)
    ProcessCommonNode(gate);
  for (const auto& var : common_variables)
    ProcessCommonNode(var);
}

void Preprocessor::GatherCommonNodes(
      std::vector<GateWeakPtr>* common_gates,
      std::vector<std::weak_ptr<Variable>>* common_variables) noexcept {
  graph_->Clear<Pdag::kVisit>();
  std::queue<Gate*> gates_queue;
  gates_queue.push(graph_->root().get());
  while (!gates_queue.empty()) {
    Gate* gate = gates_queue.front();
    gates_queue.pop();
    for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
      const GatePtr& arg_gate = arg.second;
      assert(!arg_gate->constant());
      if (arg_gate->Visited())
        continue;
      arg_gate->Visit(1);
      gates_queue.push(arg_gate.get());
      if (arg_gate->parents().size() > 1)
        common_gates->push_back(arg_gate);
    }
    for (const Gate::Arg<Variable>& arg : gate->args<Variable>()) {
      const VariablePtr& var = arg.second;
      if (var->Visited())
        continue;
      var->Visit(1);
      if (var->parents().size() > 1)
        common_variables->push_back(var);
    }
  }
}

template <class N>
void Preprocessor::ProcessCommonNode(
    const std::weak_ptr<N>& common_node) noexcept {
  assert(!graph_->HasNullGates());
  if (common_node.expired())
    return;  // The node has been deleted.

  std::shared_ptr<N> node = common_node.lock();

  if (node->parents().size() == 1)
    return;  // The extra parent is deleted.
  GatePtr root;
  MarkAncestors(node, &root);
  assert(root && "Marking ancestors ended without guaranteed module.");
  assert(root->mark() && "Graph gate marks are not cleaned.");
  assert(!root->opti_value() && "Optimization values are corrupted.");
  assert(!node->opti_value() && "Optimization values are corrupted.");
  node->opti_value(1);  // Setting for failure.

  int mult_tot = node->parents().size();  // Total multiplicity.
  assert(mult_tot > 1);
  mult_tot += PropagateState(root, node);
  assert(!root->mark() && "Partial unmarking failed.");
  assert(root->descendant() == node->index() && "Ancestors are not indexed.");

  // The results of the failure propagation.
  std::unordered_map<int, GateWeakPtr> destinations;
  int num_dest = 0;  // This is not the same as the size of destinations.
  if (root->opti_value()) {  // The root gate received the state.
    destinations.emplace(root->index(), root);
    num_dest = 1;
  } else {
    num_dest = CollectStateDestinations(root, node->index(), &destinations);
  }

  if (num_dest > 0 && num_dest < mult_tot) {  // Redundancy detection criterion.
    assert(!destinations.empty());
    std::vector<GateWeakPtr> redundant_parents;
    CollectRedundantParents(node, &destinations, &redundant_parents);
    if (!redundant_parents.empty()) {  // Note: empty destinations is OK!
      LOG(DEBUG4) << "Node " << node->index() << ": "
                  << redundant_parents.size() << " redundant parent(s) and "
                  << destinations.size() << " failure destination(s)";
      ProcessRedundantParents(node, redundant_parents);
      ProcessStateDestinations(node, destinations);
    }
  }
  ClearStateMarks(root);
  node->opti_value(0);
  graph_->RemoveNullGates();
}

void Preprocessor::MarkAncestors(const NodePtr& node,
                                 GatePtr* module) noexcept {
  for (const Node::Parent& member : node->parents()) {
    assert(!member.second.expired());
    GatePtr parent = member.second.lock();
    if (parent->mark())
      continue;
    parent->mark(true);
    if (parent->module()) {  // Do not mark further than independent subgraph.
      assert(!*module);
      *module = parent;
      continue;
    }
    MarkAncestors(parent, module);
  }
}

int Preprocessor::PropagateState(const GatePtr& gate,
                                 const NodePtr& node) noexcept {
  if (!gate->mark())
    return 0;
  gate->mark(false);  // Cleaning up the marks of the ancestors.
  assert(!gate->descendant() && "Descendant marks are corrupted.");
  gate->descendant(node->index());  // Setting ancestorship mark.
  assert(!gate->opti_value());
  int mult_tot = 0;  // The total multiplicity of the subgraph.
  int num_failure = 0;  // The number of failed arguments.
  int num_success = 0;  // The number of success arguments.
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    const GatePtr& arg_gate = arg.second;
    mult_tot += PropagateState(arg_gate, node);
    assert(!arg_gate->mark());
    int failed = arg_gate->opti_value() * boost::math::sign(arg.first);
    assert(!failed || failed == -1 || failed == 1);
    if (failed == 1) {
      ++num_failure;
    } else if (failed == -1) {
      ++num_success;
    }  // Ignore when 0.
  }
  if (node->parents().count(gate->index())) {  // Try to find the basic event.
    int failed = gate->args<Variable>().count(node->index());
    if (!failed)
      failed = -gate->args<Variable>().count(-node->index());
    failed *= node->opti_value();
    if (failed == 1) {
      ++num_failure;
    } else if (failed == -1) {
      ++num_success;
    }  // Ignore when 0.
  }
  assert(!gate->constant());
  DetermineGateState(gate, num_failure, num_success);
  int mult_add = gate->parents().size();
  if (!gate->opti_value() || mult_add < 2)
    mult_add = 0;
  return mult_tot + mult_add;
}

void Preprocessor::DetermineGateState(const GatePtr& gate, int num_failure,
                                      int num_success) noexcept {
  assert(!gate->opti_value() && "Unclear initial optimization value.");
  assert(num_failure >= 0 && "Illegal arguments or corrupted state.");
  assert(num_success >= 0 && "Illegal arguments or corrupted state.");
  assert(!(num_success && graph_->coherent()) && "Impossible state.");

  if (!(num_success + num_failure))
    return;  // Undetermined 0 state.
  auto compute_state = [&num_failure, &num_success](int req_failure,
                                                    int req_success) {
    if (num_failure >= req_failure)
      return 1;
    if (num_success >= req_success)
      return -1;
    return 0;
  };
  switch (gate->type()) {
    case kNull:
      assert((num_failure + num_success) == 1);
      gate->opti_value(compute_state(1, 1));
      break;
    case kOr:
      gate->opti_value(compute_state(1, gate->args().size()));
      break;
    case kAnd:
      gate->opti_value(compute_state(gate->args().size(), 1));
      break;
    case kVote:
      assert(gate->args().size() > gate->vote_number());
      gate->opti_value(
          compute_state(gate->vote_number(),
                        gate->args().size() - gate->vote_number() + 1));
      break;
    case kXor:
      if (num_failure == 1 && num_success == 1) {
        gate->opti_value(1);
      } else if (num_success == 2 || num_failure == 2) {
        gate->opti_value(-1);
      }
      break;
    case kNot:
      assert((num_failure + num_success) == 1);
      gate->opti_value(-compute_state(1, 1));
      break;
    case kNand:
      gate->opti_value(-compute_state(gate->args().size(), 1));
      break;
    case kNor:
      gate->opti_value(-compute_state(1, gate->args().size()));
      break;
  }
}

int Preprocessor::CollectStateDestinations(
    const GatePtr& gate,
    int index,
    std::unordered_map<int, GateWeakPtr>* destinations) noexcept {
  if (!gate->descendant())
    return 0;  // Deal with ancestors only.
  assert(gate->descendant() == index && "Corrupted descendant marks.");
  if (gate->opti_value())
    return 0;  // Don't change failure information.
  gate->opti_value(2);
  int num_dest = 0;
  for (const Gate::Arg<Gate>& member : gate->args<Gate>()) {
    const GatePtr& arg = member.second;
    num_dest += CollectStateDestinations(arg, index, destinations);
    if (arg->index() == index)
      continue;  // The state source.
    if (!arg->opti_value())
      continue;  // Indeterminate branches.
    if (arg->opti_value() > 1)
      continue;  // Not a state destination.
    ++num_dest;  // Optimization value is 1 or -1.
    destinations->emplace(arg->index(), arg);
  }
  return num_dest;
}

void Preprocessor::CollectRedundantParents(
    const NodePtr& node,
    std::unordered_map<int, GateWeakPtr>* destinations,
    std::vector<GateWeakPtr>* redundant_parents) noexcept {
  for (const Node::Parent& member : node->parents()) {
    assert(!member.second.expired());
    GatePtr parent = member.second.lock();
    assert(!parent->mark());
    if (parent->opti_value() == 2)
      continue;  // Non-redundant parent.
    if (parent->opti_value()) {
      assert(parent->opti_value() == 1 || parent->opti_value() == -1);
      if (auto it = ext::find(*destinations, parent->index())) {
        Operator type = parent->opti_value() == 1 ? kOr : kAnd;
        if (parent->type() == type &&
            parent->opti_value() == parent->GetArgSign(node)) {
          destinations->erase(it);
          continue;  // Destination and redundancy collision.
        }
        assert(!(graph_->coherent() && parent->type() == type));
      }
    }
    redundant_parents->push_back(parent);
  }
}

void Preprocessor::ProcessRedundantParents(
    const NodePtr& node,
    const std::vector<GateWeakPtr>& redundant_parents) noexcept {
  // The node behaves like a constant False for redundant parents.
  for (const GateWeakPtr& ptr : redundant_parents) {
    if (ptr.expired())
      continue;
    GatePtr parent = ptr.lock();
    parent->ProcessConstantArg(node, node->opti_value() != 1);
  }
}

template <class N>
void Preprocessor::ProcessStateDestinations(
    const std::shared_ptr<N>& node,
    const std::unordered_map<int, GateWeakPtr>& destinations) noexcept {
  for (const auto& ptr : destinations) {
    if (ptr.second.expired())
      continue;
    GatePtr target = ptr.second.lock();
    assert(!target->mark());
    assert(target->opti_value() == 1 || target->opti_value() == -1);
    Operator type = target->opti_value() == 1 ? kOr : kAnd;
    if (target->type() == type) {  // Reuse of an existing gate.
      if (target->constant())
        continue;  // No need to process.
      target->AddArg(node, target->opti_value() < 0);
      assert(!(!target->constant() && target->type() == kNull));
      continue;
    }
    auto new_gate = std::make_shared<Gate>(type, graph_);
    new_gate->AddArg(node, target->opti_value() < 0);
    if (target->module()) {  // Transfer modularity.
      target->module(false);
      new_gate->module(true);
    }
    if (target == graph_->root()) {
      graph_->root(new_gate);  // The sign is preserved.
    } else {
      ReplaceGate(target, new_gate);
    }
    new_gate->AddArg(target);  // Only after replacing target!
    new_gate->descendant(node->index());  // Preserve continuity.
  }
}

void Preprocessor::ClearStateMarks(const GatePtr& gate) noexcept {
  if (!gate->descendant())
    return;  // Clean only 'dirty' gates.
  gate->descendant(0);
  gate->opti_value(0);
  for (const Gate::Arg<Gate>& arg : gate->args<Gate>()) {
    ClearStateMarks(arg.second);
  }
  for (const Node::Parent& member : gate->parents()) {
    ClearStateMarks(member.second.lock());  // Due to replacement.
  }
}

bool Preprocessor::DecomposeCommonNodes() noexcept {
  TIMER(DEBUG3, "Decomposition of common nodes");
  assert(!graph_->HasNullGates());

  std::vector<GateWeakPtr> common_gates;
  std::vector<std::weak_ptr<Variable>> common_variables;
  GatherCommonNodes(&common_gates, &common_variables);

  graph_->Clear<Pdag::kVisit>();
  AssignTiming(0, graph_->root());  // Required for optimization.
  graph_->Clear<Pdag::kDescendant>();  // Used for ancestor detection.
  graph_->Clear<Pdag::kAncestor>();  // Used for sub-graph detection.
  graph_->Clear<Pdag::kGateMark>();  // Important for linear traversal.

  bool changed = false;
  // The processing is done deepest-layer-first.
  // The deepest-first processing avoids generating extra parents
  // for the nodes that are deep in the graph.
  for (auto it = common_gates.rbegin(); it != common_gates.rend(); ++it) {
    changed |= DecompositionProcessor()(*it, this);
  }

  // Variables are processed after gates
  // because, if parent gates are removed,
  // there may be no need to process these variables.
  for (auto it = common_variables.rbegin(); it != common_variables.rend();
       ++it) {
    changed |= DecompositionProcessor()(*it, this);
  }
  return changed;
}

bool Preprocessor::DecompositionProcessor::operator()(
    const std::weak_ptr<Node>& common_node,
    Preprocessor* preprocessor) noexcept {
  assert(preprocessor);
  if (common_node.expired())
    return false;  // The node has been deleted.
  node_ = common_node.lock();
  if (node_->parents().size() < 2)
    return false;  // Not common anymore.
  preprocessor_ = preprocessor;
  assert(!preprocessor_->graph_->HasNullGates());
  // Determines whether decomposition is possible with a given type.
  auto is_decomposition_type = [](Operator type) {
    switch (type) {
      case kAnd:
      case kNand:
      case kOr:
      case kNor:
        return true;
      default:
        return false;
    }
  };
  // Determine if the decomposition setups are possible.
  auto it = boost::find_if(
      node_->parents(), [&is_decomposition_type](const Node::Parent& member) {
        return is_decomposition_type(member.second.lock()->type());
      });
  if (it == node_->parents().end())
    return false;  // No setups possible.

  assert(2 > boost::count_if(node_->parents(), [](const Node::Parent& member) {
           return member.second.lock()->module();
         }));

  // Mark parents and ancestors.
  for (const Node::Parent& member : node_->parents()) {
    assert(!member.second.expired());
    GatePtr parent = member.second.lock();
    MarkDestinations(parent);
  }
  // Find destinations with particular setups.
  // If a parent gets marked upon destination search,
  // the parent is the destination.
  std::vector<GateWeakPtr> dest;
  for (const Node::Parent& member : node_->parents()) {
    assert(!member.second.expired());
    GatePtr parent = member.second.lock();
    if (parent->descendant() == node_->index() &&
        is_decomposition_type(parent->type())) {
      dest.push_back(parent);
    }
  }
  if (dest.empty())
    return false;  // No setups are found.

  bool ret = ProcessDestinations(dest);
  BLOG(DEBUG4, ret) << "Successful decomposition of node " << node_->index();
  return ret;
}

void Preprocessor::DecompositionProcessor::MarkDestinations(
    const GatePtr& parent) noexcept {
  if (parent->module())
    return;  // Limited with independent subgraphs.
  for (const Node::Parent& member : parent->parents()) {
    assert(!member.second.expired());
    GatePtr ancestor = member.second.lock();
    if (ancestor->descendant() == node_->index())
      continue;  // Already marked.
    ancestor->descendant(node_->index());
    MarkDestinations(ancestor);
  }
}

bool Preprocessor::DecompositionProcessor::ProcessDestinations(
    const std::vector<GateWeakPtr>& dest) noexcept {
  bool changed = false;
  for (const auto& ptr : dest) {
    if (ptr.expired())
      continue;  // Removed by constant propagation.
    GatePtr parent = ptr.lock();

    // The destination may already be processed
    // in the link of ancestors.
    if (!node_->parents().count(parent->index()))
      continue;

    bool state = false;  // State for the constant propagation.
    switch (parent->type()) {
      case kAnd:
      case kNand:
        state = true;
        break;
      case kOr:
      case kNor:
        state = false;
        break;
      default:
        assert(false && "Complex gates cannot be decomposition destinations.");
    }
    if (parent->GetArgSign(node_) < 0)
      state = !state;
    assert(!parent->mark() && "Subgraph is not clean!");
    bool ret = ProcessAncestors(parent, state, parent);
    changed |= ret;
    // Keep the graph clean.
    preprocessor_->graph_->Clear<Pdag::kGateMark>(parent);
    BLOG(DEBUG5, ret) << "Successful decomposition is in G" << parent->index();
  }
  // Actual propagation of the constant.
  preprocessor_->graph_->RemoveNullGates();
  return changed;
}

bool Preprocessor::DecompositionProcessor::ProcessAncestors(
    const GatePtr& ancestor,
    bool state,
    const GatePtr& root) noexcept {
  if (ancestor->mark())
    return false;
  ancestor->mark(true);
  bool changed = false;
  std::vector<std::pair<int, GatePtr>> to_swap;  // For common gates.
  for (const Gate::Arg<Gate>& arg : ancestor->args<Gate>()) {
    GatePtr gate = arg.second;
    if (node_->parents().count(gate->index())) {
      LOG(DEBUG5) << "Reached decomposition sub-parent G" << gate->index();
      if (IsAncestryWithinGraph(gate, root)) {
        changed = true;
        gate->ProcessConstantArg(node_, state);
        preprocessor_->RegisterToClear(gate);
      } else {
        GatePtr clone = gate->Clone();
        if (preprocessor_->RegisterToClear(clone)) {
          to_swap.emplace_back(arg.first, clone);
          changed = true;
          clone->descendant(gate->descendant());
          clone->ancestor(root->index());
          clone->Visit(gate->EnterTime());
          clone->Visit(gate->ExitTime());
          clone->Visit(gate->LastVisit());
          clone->ProcessConstantArg(node_, state);
          gate = clone;  // Continue with the new parent.
        }
      }
    }
    if (gate->descendant() != node_->index())
      continue;
    if (!IsAncestryWithinGraph(gate, root))
      continue;
    changed |= ProcessAncestors(gate, state, root);
  }
  for (const auto& arg : ancestor->args<Gate>()) {
    ClearAncestorMarks(arg.second, root);
  }
  for (const auto& arg : to_swap) {
    ancestor->EraseArg(arg.first);
    ancestor->AddArg(arg.second, arg.first < 0);
  }
  if (!node_->parents().count(ancestor->index()) &&
      ext::none_of(ancestor->args<Gate>(), [this](const Gate::Arg<Gate>& arg) {
        return arg.second->descendant() == this->node_->index();
      })) {
    ancestor->descendant(0);  // Lose ancestorship if the descendant is gone.
  }
  return changed;
}

bool Preprocessor::DecompositionProcessor::IsAncestryWithinGraph(
    const GatePtr& gate,
    const GatePtr& root) noexcept {
  if (gate == root)
    return true;
  if (gate->ancestor() == root->index())
    return true;
  if (gate->ancestor() == -root->index())
    return false;

  if (IsNodeWithinGraph(gate, root->EnterTime(), root->ExitTime()) &&
      ext::all_of(gate->parents(), [&root](const Node::Parent& member) {
        return IsAncestryWithinGraph(member.second.lock(), root);
      })) {
    gate->ancestor(root->index());
    return true;
  }

  gate->ancestor(-root->index());
  return false;
}

void Preprocessor::DecompositionProcessor::ClearAncestorMarks(
    const GatePtr& gate,
    const GatePtr& root) noexcept {
  assert(root->ancestor() == 0 && "The root mark is dirty.");
  if (gate->ancestor() == 0)
    return;
  assert(std::abs(gate->ancestor()) == root->index() && "Wrong markings.");
  gate->ancestor(0);
  for (const Node::Parent& member : gate->parents()) {
    ClearAncestorMarks(member.second.lock(), root);
  }
}

void Preprocessor::ReplaceGate(const GatePtr& gate,
                               const GatePtr& replacement) noexcept {
  assert(!gate->parents().empty());
  while (!gate->parents().empty()) {
    GatePtr parent = gate->parents().begin()->second.lock();
    int sign = parent->GetArgSign(gate);
    parent->EraseArg(sign * gate->index());
    parent->AddArg(replacement, sign < 0);
  }
}

bool Preprocessor::RegisterToClear(const GatePtr& gate) noexcept {
  return gate->constant() || gate->type() == kNull;  // automatic register.
}

void Preprocessor::GatherNodes(std::vector<GatePtr>* gates,
                               std::vector<VariablePtr>* variables) noexcept {
  graph_->Clear<Pdag::kVisit>();
  GatherNodes(graph_->root(), gates, variables);
}

void Preprocessor::GatherNodes(const GatePtr& gate,
                               std::vector<GatePtr>* gates,
                               std::vector<VariablePtr>* variables) noexcept {
  if (gate->Visited())
    return;
  gate->Visit(1);
  gates->push_back(gate);
  for (const auto& arg : gate->args<Gate>()) {
    GatherNodes(arg.second, gates, variables);
  }
  for (const auto& arg : gate->args<Variable>()) {
    if (!arg.second->Visited()) {
      arg.second->Visit(1);
      variables->push_back(arg.second);
    }
  }
}

void CustomPreprocessor<Bdd>::Run() noexcept {
  Preprocessor::Run();
  pdag::Transform(graph_, &pdag::MarkCoherence, &pdag::TopologicalOrder);
}

void CustomPreprocessor<Zbdd>::Run() noexcept {
  Preprocessor::Run();
  pdag::Transform(graph_,
                  [this](Pdag*) {
                    if (!graph_->coherent())
                      RunPhaseFour();
                  },
                  [this](Pdag*) { RunPhaseFive(); },
                  &pdag::MarkCoherence,
                  &pdag::TopologicalOrder);
}

void CustomPreprocessor<Mocus>::Run() noexcept {
  CustomPreprocessor<Zbdd>::Run();
  pdag::Transform(graph_, [this](Pdag*) { InvertOrder(); });
}

void CustomPreprocessor<Mocus>::InvertOrder() noexcept {
  std::vector<GatePtr> gates;
  std::vector<VariablePtr> variables;
  Preprocessor::GatherNodes(&gates, &variables);
  auto middle = boost::partition(
      gates, [](const GatePtr& gate) { return gate->module(); });

  std::sort(middle, gates.end(), [](const GatePtr& lhs, const GatePtr& rhs) {
    return lhs->order() < rhs->order();
  });
  for (auto it = middle; it != gates.end(); ++it)
    (*it)->order(gates.end() - it);  // Inversion.

  int shift = gates.end() - middle;
  for (auto it = gates.begin(); it != middle; ++it)
    (*it)->order(shift + (*it)->order());

  for (auto var : variables)
    var->order(shift + var->order());
}

}  // namespace core
}  // namespace scram
