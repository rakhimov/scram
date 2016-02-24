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

/// @file preprocessor.cc
/// Implementation of preprocessing algorithms.
/// The main goal of preprocessing algorithms is
/// to make Boolean graphs simpler, modular, easier for analysis.
///
/// If a preprocessing algorithm has
/// its limitations, side-effects, and assumptions,
/// the documentation in the header file
/// must contain all the relevant information within
/// its description, preconditions, postconditions, notes, or warnings.
/// The default assumption for all algorithms is
/// that the Boolean graph is valid and well-formed.
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
/// Assuming that the Boolean graph is provided
/// in the state as described in the contract,
/// the algorithms should never throw an exception.
///
/// The algorithms must guarantee
/// that, given a valid and well-formed Boolean graph,
/// the resulting Boolean graph
/// will at least be valid, well-formed,
/// and semantically equivalent (isomorphic) to the input Boolean graph.
/// Moreover, the algorithms must be deterministic
/// and produce stable results.
///
/// If the contract is violated,
/// the result or behavior of the algorithm can be undefined.
/// There is no requirement
/// to check for the contract violations
/// and to exit gracefully.

#include "preprocessor.h"

#include <algorithm>
#include <list>
#include <queue>
#include <unordered_set>

#include "logger.h"

namespace scram {

Preprocessor::Preprocessor(BooleanGraph* graph) noexcept
    : graph_(graph),
      constant_graph_(false) {}

void Preprocessor::Run() noexcept {
  CLOCK(time_1);
  LOG(DEBUG2) << "Preprocessing Phase I...";
  Preprocessor::PhaseOne();
  LOG(DEBUG2) << "Finished Preprocessing Phase I in " << DUR(time_1);
  if (Preprocessor::CheckRootGate()) return;

  CLOCK(time_2);
  LOG(DEBUG2) << "Preprocessing Phase II...";
  Preprocessor::PhaseTwo();
  LOG(DEBUG2) << "Finished Preprocessing Phase II in " << DUR(time_2);
  if (Preprocessor::CheckRootGate()) return;

  if (!graph_->normal()) {
    CLOCK(time_3);
    LOG(DEBUG2) << "Preprocessing Phase III...";
    Preprocessor::PhaseThree();
    LOG(DEBUG2) << "Finished Preprocessing Phase III in " << DUR(time_3);
    if (Preprocessor::CheckRootGate()) return;
  }
}

namespace {  // Boolean graph structure verification tools.

/// @class TestGateMarks
/// Functor to sanity check the marks of Boolean graph gates.
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
  bool operator()(const std::shared_ptr<const IGate>& gate,
                  bool mark) noexcept {
    if (tested_gates_.count(gate->index())) return false;
    tested_gates_.insert(gate->index());
    assert(gate->mark() == mark && "Found discontinuous gate mark.");
    for (const auto& arg : gate->gate_args()) (*this)(arg.second, mark);
    return true;
  }

 private:
  std::unordered_set<int> tested_gates_;  ///< Alternative to gate marking.
};

/// @class TestGateStructure
/// Functor to sanity check the structure of Boolean graph gates.
class TestGateStructure {
 public:
  /// Helper function to find malformed gates.
  /// The traversal will fail with assert
  /// upon depth-first encounter of a problem.
  ///
  /// @param[in] gate  The starting gate to traverse.
  ///
  /// @returns true if the job is done.
  bool operator()(const std::shared_ptr<const IGate>& gate) noexcept {
    if (tested_gates_.count(gate->index())) return false;
    tested_gates_.insert(gate->index());
    assert(!gate->IsConstant() && "Constant gates are not clear!");
    switch (gate->type()) {
      case kNull:
      case kNot:
        assert(gate->args().size() == 1 && "Malformed one-arg gate!");
        break;
      case kXor:
        assert(gate->args().size() == 2 && "Malformed XOR gate!");
        break;
      case kVote:
        assert(gate->vote_number() > 1 && "K/N has wrong K!");
        assert(gate->args().size() > gate->vote_number() && "K/N has wrong N!");
        break;
      default:
        assert(gate->args().size() > 1 && "Missing arguments!");
    }
    for (const auto& arg : gate->gate_args()) (*this)(arg.second);
    return true;
  }

 private:
  std::unordered_set<int> tested_gates_;  ///< Alternative to gate marking.
};

}  // namespace

/// @def SANITY_ASSERT
/// A collection of sanity checks between preprocessing phases.
#define SANITY_ASSERT                                                         \
  assert(graph_->root() && "Corrupted pointer to the root gate.");            \
  assert(graph_->root()->parents().empty() && "Root can't have parents.");    \
  assert(!(graph_->coherent() && graph_->complement()));                      \
  assert(const_gates_.empty() && "Const gate cleanup contracts are broken!"); \
  assert(null_gates_.empty() && "Null gate cleanup contracts are broken!");   \
  assert(TestGateStructure()(graph_->root()));                                \
  assert(TestGateMarks()(graph_->root(), graph_->root()->mark()))

void Preprocessor::PhaseOne() noexcept {
  SANITY_ASSERT;
  graph_->Log();
  if (!graph_->constants_.empty()) {
    LOG(DEBUG3) << "Removing constants...";
    Preprocessor::RemoveConstants();
    LOG(DEBUG3) << "Constant are removed!";
    if (Preprocessor::CheckRootGate()) return;
  }
  if (!graph_->null_gates_.empty()) {
    LOG(DEBUG3) << "Removing NULL gates...";
    Preprocessor::RemoveNullGates();
    LOG(DEBUG3) << "Finished cleaning NULL gates!";
    if (Preprocessor::CheckRootGate()) return;
  }
  if (!graph_->coherent_) {
    LOG(DEBUG3) << "Partial normalization of gates...";
    Preprocessor::NormalizeGates(/*full=*/false);
    LOG(DEBUG3) << "Finished the partial normalization of gates!";
  }
}

void Preprocessor::PhaseTwo() noexcept {
  SANITY_ASSERT;
  graph_->Log();
  CLOCK(mult_time);
  LOG(DEBUG3) << "Detecting multiple definitions...";
  while (Preprocessor::ProcessMultipleDefinitions()) continue;
  LOG(DEBUG3) << "Finished multi-definition detection in " << DUR(mult_time);

  if (Preprocessor::CheckRootGate()) return;

  LOG(DEBUG3) << "Detecting modules...";
  Preprocessor::DetectModules();
  LOG(DEBUG3) << "Finished module detection!";

  LOG(DEBUG3) << "Coalescing gates...";
  while (Preprocessor::CoalesceGates(/*common=*/false)) continue;
  LOG(DEBUG3) << "Gate coalescence is done!";

  if (Preprocessor::CheckRootGate()) return;

  CLOCK(merge_time);
  LOG(DEBUG3) << "Merging common arguments...";
  Preprocessor::MergeCommonArgs();
  LOG(DEBUG3) << "Finished merging common args in " << DUR(merge_time);

  if (Preprocessor::CheckRootGate()) return;

  LOG(DEBUG3) << "Processing Distributivity...";
  Preprocessor::DetectDistributivity();
  LOG(DEBUG3) << "Distributivity detection is done!";

  if (Preprocessor::CheckRootGate()) return;

  LOG(DEBUG3) << "Detecting modules...";
  Preprocessor::DetectModules();
  LOG(DEBUG3) << "Finished module detection!";

  CLOCK(optim_time);
  LOG(DEBUG3) << "Boolean optimization...";
  Preprocessor::BooleanOptimization();
  LOG(DEBUG3) << "Finished Boolean optimization in " << DUR(optim_time);

  if (Preprocessor::CheckRootGate()) return;

  CLOCK(decom_time);
  LOG(DEBUG3) << "Decomposition of common nodes...";
  Preprocessor::DecomposeCommonNodes();
  LOG(DEBUG3) << "Finished the Decomposition in " << DUR(decom_time);

  if (Preprocessor::CheckRootGate()) return;

  LOG(DEBUG3) << "Detecting modules...";
  Preprocessor::DetectModules();
  LOG(DEBUG3) << "Finished module detection!";

  LOG(DEBUG3) << "Coalescing gates...";
  while (Preprocessor::CoalesceGates(/*common=*/false)) continue;
  LOG(DEBUG3) << "Gate coalescence is done!";

  if (Preprocessor::CheckRootGate()) return;

  LOG(DEBUG3) << "Detecting modules...";
  Preprocessor::DetectModules();
  LOG(DEBUG3) << "Finished module detection!";

  graph_->Log();
}

void Preprocessor::PhaseThree() noexcept {
  SANITY_ASSERT;
  graph_->Log();
  assert(!graph_->normal_);
  LOG(DEBUG3) << "Full normalization of gates...";
  Preprocessor::NormalizeGates(/*full=*/true);
  graph_->normal_ = true;
  LOG(DEBUG3) << "Finished the full normalization of gates!";

  if (Preprocessor::CheckRootGate()) return;
  Preprocessor::PhaseTwo();
}

void Preprocessor::PhaseFour() noexcept {
  SANITY_ASSERT;
  graph_->Log();
  assert(!graph_->coherent());
  LOG(DEBUG3) << "Propagating complements...";
  if (graph_->root_sign_ < 0) {
    IGatePtr root = graph_->root();
    assert(root->type() == kOr || root->type() == kAnd ||
           root->type() == kNull);
    if (root->type() == kOr || root->type() == kAnd)
      root->type(root->type() == kOr ? kAnd : kOr);
    root->InvertArgs();
    graph_->root_sign_ = 1;
  }
  std::unordered_map<int, IGatePtr> complements;
  graph_->ClearGateMarks();
  Preprocessor::PropagateComplements(graph_->root(), false, &complements);
  complements.clear();
  LOG(DEBUG3) << "Complement propagation is done!";

  if (Preprocessor::CheckRootGate()) return;
  Preprocessor::PhaseTwo();
}

void Preprocessor::PhaseFive() noexcept {
  SANITY_ASSERT;
  graph_->Log();
  LOG(DEBUG3) << "Coalescing gates...";  // Make layered.
  while (Preprocessor::CoalesceGates(/*common=*/true)) continue;
  LOG(DEBUG3) << "Gate coalescence is done!";

  if (Preprocessor::CheckRootGate()) return;
  Preprocessor::PhaseTwo();
  if (Preprocessor::CheckRootGate()) return;

  LOG(DEBUG3) << "Coalescing gates...";  // Final coalescing before analysis.
  while (Preprocessor::CoalesceGates(/*common=*/true)) continue;
  LOG(DEBUG3) << "Gate coalescence is done!";

  if (Preprocessor::CheckRootGate()) return;
  graph_->Log();
}

#undef SANITY_ASSERT

namespace {  // Helper functions for all preprocessing algorithms.

/// @param[in] num  Signed non-zero integer or index.
///
/// @returns -1 for negative numbers and 1 for positive ones.
int GetSign(int num) noexcept {
  assert(num);
  return num > 0 ? 1 : -1;
}

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
  return std::max(a_min, b_min) <= std::min(a_max, b_max);
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
bool IsSubgraphWithinGraph(const IGatePtr& root, int enter_time,
                           int exit_time) noexcept {
  assert(enter_time > 0);
  assert(exit_time > enter_time);
  assert(root->min_time() > 0);
  assert(root->max_time() > root->min_time());
  return root->min_time() > enter_time && root->max_time() < exit_time;
}

}  // namespace

bool Preprocessor::CheckRootGate() noexcept {
  IGatePtr root = graph_->root();
  if (root->IsConstant()) {
    LOG(DEBUG3) << "The root gate has become constant!";
    assert(!(graph_->coherent() && !constant_graph_) &&
          "Impossible state of the root gate in coherent graphs.");
    if (graph_->root_sign_ < 0) {
      State orig_state = root->state();
      root = std::make_shared<IGate>(kNull);
      graph_->root(root);
      if (orig_state == kNullState) {
        root->MakeUnity();
      } else {
        assert(orig_state == kUnityState);
        root->Nullify();
      }
      graph_->root_sign_ = 1;
    }
    return true;  // No more processing is needed.
  }
  if (root->type() == kNull) {  // Special case of preprocessing.
    LOG(DEBUG3) << "The root NULL gate is processed!";
    assert(root->args().size() == 1);
    if (!root->gate_args().empty()) {
      int signed_index = root->gate_args().begin()->first;
      root = root->gate_args().begin()->second;
      graph_->root(root);  // Destroy the previous root.
      assert(root->parents().empty());
      graph_->root_sign_ *= GetSign(signed_index);
    } else {
      LOG(DEBUG4) << "The root NULL gate has only single variable!";
      assert(root->variable_args().size() == 1);
      if (graph_->root_sign_ < 0) root->InvertArgs();
      graph_->root_sign_ = 1;
      root->variable_args().begin()->second->order(1);
      return true;  // Only one variable argument.
    }
  }
  return false;
}

void Preprocessor::RemoveNullGates() noexcept {
  assert(null_gates_.empty());
  assert(!graph_->null_gates_.empty());
  null_gates_ = graph_->null_gates_;  // Transferring for internal uses.
  graph_->null_gates_.clear();
  Preprocessor::ClearNullGates();
  assert(null_gates_.empty());
}

void Preprocessor::RemoveConstants() noexcept {
  assert(const_gates_.empty());
  assert(!graph_->constants_.empty());
  for (const std::weak_ptr<Constant>& ptr : graph_->constants_) {
    if (ptr.expired()) continue;
    Preprocessor::PropagateConstant(ptr.lock());
    assert(ptr.expired());
  }
  assert(const_gates_.empty());
  graph_->constants_.clear();
  constant_graph_ = graph_->root()->IsConstant();
}

void Preprocessor::PropagateConstant(const ConstantPtr& constant) noexcept {
  while (!constant->parents().empty()) {
    IGatePtr parent = constant->parents().begin()->second.lock();
    parent->ProcessConstantArg(constant, constant->state());
    if (parent->IsConstant()) {
      Preprocessor::PropagateConstant(parent);
    } else if (parent->type() == kNull) {
      Preprocessor::PropagateNullGate(parent);
    }
  }
}

void Preprocessor::PropagateConstant(const IGatePtr& gate) noexcept {
  assert(gate->IsConstant());
  while (!gate->parents().empty()) {
    IGatePtr parent = gate->parents().begin()->second.lock();
    bool state = gate->state() == kUnityState;
    parent->ProcessConstantArg(gate, state);
    if (parent->IsConstant()) {
      Preprocessor::PropagateConstant(parent);
    } else if (parent->type() == kNull) {
      Preprocessor::PropagateNullGate(parent);
    }
  }
}

void Preprocessor::PropagateNullGate(const IGatePtr& gate) noexcept {
  assert(gate->type() == kNull);
  while (!gate->parents().empty()) {
    IGatePtr parent = gate->parents().begin()->second.lock();
    int sign = parent->GetArgSign(gate);
    parent->JoinNullGate(sign * gate->index());
    if (parent->IsConstant()) {
      Preprocessor::PropagateConstant(parent);
    } else if (parent->type() == kNull) {
      Preprocessor::PropagateNullGate(parent);
    }
  }
}

void Preprocessor::ClearConstGates() noexcept {
  graph_->ClearGateMarks();  // New gates may get created without marks!
  BLOG(DEBUG5, !const_gates_.empty()) << "Got CONST gates to clear!";
  for (const IGateWeakPtr& ptr : const_gates_) {
    if (ptr.expired()) continue;
    Preprocessor::PropagateConstant(ptr.lock());
  }
  const_gates_.clear();
}

void Preprocessor::ClearNullGates() noexcept {
  graph_->ClearGateMarks();  // New gates may get created without marks!
  BLOG(DEBUG5, !null_gates_.empty()) << "Got NULL gates to clear!";
  for (const IGateWeakPtr& ptr : null_gates_) {
    if (ptr.expired()) continue;
    Preprocessor::PropagateNullGate(ptr.lock());
  }
  null_gates_.clear();
}

void Preprocessor::NormalizeGates(bool full) noexcept {
  assert(const_gates_.empty());
  assert(null_gates_.empty());
  if (full) {
    graph_->ClearNodeOrders();
    Preprocessor::AssignOrder();  // K/N gates need order.
  }
  IGatePtr root_gate = graph_->root();
  Operator type = root_gate->type();
  switch (type) {  // Handle special case for the root gate.
    case kNor:
    case kNand:
    case kNot:
      graph_->root_sign_ *= -1;
      break;
    default:  // All other types keep the sign of the root.
      assert((type == kAnd || type == kOr || type == kVote ||
              type == kXor || type == kNull) &&
             "Update the logic if new gate types are introduced.");
  }
  // Process negative gates.
  // Note that root's negative gate is processed in the above lines.
  graph_->ClearGateMarks();
  Preprocessor::NotifyParentsOfNegativeGates(root_gate);

  graph_->ClearGateMarks();
  Preprocessor::NormalizeGate(root_gate, full);  // Registers null gates only.

  assert(const_gates_.empty());
  Preprocessor::ClearNullGates();
}

void Preprocessor::NotifyParentsOfNegativeGates(const IGatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  std::vector<int> to_negate;  // Args to get the negation.
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    Preprocessor::NotifyParentsOfNegativeGates(arg.second);
    Operator type = arg.second->type();
    switch (type) {
      case kNor:
      case kNand:
      case kNot:
        to_negate.push_back(arg.first);
        break;
      default:  // No notification for other types.
        assert(type != kNull && "NULL gates should have been cleared.");
        assert((type == kAnd || type == kOr || type == kVote ||
                type == kXor) &&
               "Update the logic if new gate types are introduced.");
    }
  }
  for (int index : to_negate) gate->InvertArg(index);  // No constants or NULL.
}

void Preprocessor::NormalizeGate(const IGatePtr& gate, bool full) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  assert(!gate->IsConstant());
  assert(!gate->args().empty());
  // Depth-first traversal before the arguments may get changed.
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    Preprocessor::NormalizeGate(arg.second, full);
  }

  switch (gate->type()) {  // Negation is already processed.
    case kNot:
      assert(gate->args().size() == 1);
      gate->type(kNull);
      null_gates_.push_back(gate);  // Register for removal.
      break;
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
      if (!full) break;
      Preprocessor::NormalizeXorGate(gate);
      break;
    case kVote:
      assert(gate->args().size() > 2);
      assert(gate->vote_number() > 1);
      if (!full) break;
      Preprocessor::NormalizeVoteGate(gate);
      break;
    case kNull:
      null_gates_.push_back(gate);  // Register for removal.
      break;
    default:  // Already normal gates.
      assert(gate->type() == kAnd || gate->type() == kOr);
      assert(gate->args().size() > 1);
  }
}

void Preprocessor::NormalizeXorGate(const IGatePtr& gate) noexcept {
  assert(gate->args().size() == 2);
  auto gate_one = std::make_shared<IGate>(kAnd);
  auto gate_two = std::make_shared<IGate>(kAnd);
  gate_one->mark(true);
  gate_two->mark(true);

  gate->type(kOr);
  std::set<int>::const_iterator it = gate->args().begin();
  gate->ShareArg(*it, gate_one);
  gate->ShareArg(*it, gate_two);
  gate_two->InvertArg(*it);

  ++it;  // Handling the second argument.
  gate->ShareArg(*it, gate_one);
  gate_one->InvertArg(*it);
  gate->ShareArg(*it, gate_two);

  gate->EraseAllArgs();
  gate->AddArg(gate_one->index(), gate_one);
  gate->AddArg(gate_two->index(), gate_two);
}

void Preprocessor::NormalizeVoteGate(const IGatePtr& gate) noexcept {
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

  auto it = std::max_element(gate->args().cbegin(), gate->args().cend(),
                             [&gate](int lhs, int rhs) {
    return gate->GetArg(lhs)->order() < gate->GetArg(rhs)->order();
  });
  assert(it != gate->args().cend());
  auto first_arg = std::make_shared<IGate>(kAnd);
  gate->TransferArg(*it, first_arg);

  auto grand_arg = std::make_shared<IGate>(kVote);
  first_arg->AddArg(grand_arg->index(), grand_arg);
  grand_arg->vote_number(vote_number - 1);

  auto second_arg = std::make_shared<IGate>(kVote);
  second_arg->vote_number(vote_number);

  for (it = gate->args().cbegin(); it != gate->args().cend(); ++it) {
    gate->ShareArg(*it, grand_arg);
    gate->ShareArg(*it, second_arg);
  }

  first_arg->mark(true);
  second_arg->mark(true);
  grand_arg->mark(true);

  gate->type(kOr);
  gate->EraseAllArgs();
  gate->AddArg(first_arg->index(), first_arg);
  gate->AddArg(second_arg->index(), second_arg);

  Preprocessor::NormalizeVoteGate(grand_arg);
  Preprocessor::NormalizeVoteGate(second_arg);
}

void Preprocessor::PropagateComplements(
    const IGatePtr& gate,
    bool keep_modules,
    std::unordered_map<int, IGatePtr>* complements) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  // If the argument gate is complement,
  // then create a new gate
  // that propagates its sign to its arguments
  // and itself becomes non-complement.
  // Keep track of complement gates
  // for optimization of repeated complements.
  std::vector<std::pair<int, IGatePtr>> to_swap;  // Gate args with negation.
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    IGatePtr arg_gate = arg.second;
    if ((arg.first < 0) && !(keep_modules && arg_gate->IsModule())) {
      IGatePtr complement;
      if (complements->count(arg_gate->index())) {
        complement = complements->find(arg_gate->index())->second;
        to_swap.emplace_back(arg.first, complement);
        assert(complement->mark());
        continue;  // Existing complements are already processed.
      }
      Operator type = arg_gate->type();
      assert(type == kAnd || type == kOr);
      Operator complement_type = type == kOr ? kAnd : kOr;
      if (arg_gate->parents().size() == 1) {  // Optimization. Reuse.
        arg_gate->type(complement_type);
        arg_gate->InvertArgs();
        complement = arg_gate;
      } else {
        complement = arg_gate->Clone();
        if (arg_gate->IsModule()) arg_gate->DestroyModule();  // Very bad.
        complement->type(complement_type);
        complement->InvertArgs();
        complements->emplace(arg_gate->index(), complement);
      }
      to_swap.emplace_back(arg.first, complement);
      arg_gate = complement;  // Needed for further propagation.
    }
    Preprocessor::PropagateComplements(arg_gate, keep_modules, complements);
  }

  for (const auto& arg : to_swap) {
    assert(arg.first < 0);
    gate->EraseArg(arg.first);
    gate->AddArg(arg.second->index(), arg.second);
    assert(!gate->IsConstant());  // No duplicates.
  }
}

bool Preprocessor::CoalesceGates(bool common) noexcept {
  assert(const_gates_.empty());
  assert(null_gates_.empty());
  if (graph_->root()->IsConstant()) return false;
  graph_->ClearGateMarks();
  bool ret = Preprocessor::CoalesceGates(graph_->root(), common);

  assert(null_gates_.empty());
  Preprocessor::ClearConstGates();
  return ret;
}

bool Preprocessor::CoalesceGates(const IGatePtr& gate, bool common) noexcept {
  if (gate->mark()) return false;
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
  std::vector<IGatePtr> to_join;  // Gate arguments of the same logic.
  bool changed = false;  // Indication if the graph is changed.
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    IGatePtr arg_gate = arg.second;
    if (Preprocessor::CoalesceGates(arg_gate, common)) changed = true;

    if (target_type == kNull) continue;  // Coalescing is impossible.
    if (arg_gate->IsConstant()) continue;  // No args to join.
    if (arg.first < 0) continue;  // Cannot coalesce a negative arg gate.
    if (arg_gate->IsModule()) continue;  // Preserve modules.
    if (!common && arg_gate->parents().size() > 1) continue;  // Check common.

    if (arg_gate->type() == target_type) to_join.push_back(arg_gate);
  }

  for (const IGatePtr& arg : to_join) {
    gate->CoalesceGate(arg);
    changed = true;
    if (gate->IsConstant()) {
      const_gates_.push_back(gate);  // Register for future processing.
      break;  // The parent is constant. No need to join other arguments.
    }
    assert(gate->args().size() > 1);  // Does not produce NULL type gates.
  }
  return changed;
}

bool Preprocessor::ProcessMultipleDefinitions() noexcept {
  assert(null_gates_.empty());
  assert(const_gates_.empty());
  if (graph_->root()->IsConstant()) return false;

  graph_->ClearGateMarks();
  // The original gate and its multiple definitions.
  std::unordered_map<IGatePtr, std::vector<IGateWeakPtr> > multi_def;
  GateSet* unique_gates = new GateSet();
  Preprocessor::DetectMultipleDefinitions(graph_->root(), &multi_def,
                                          unique_gates);
  delete unique_gates;  // To remove extra reference counts.
  graph_->ClearGateMarks();

  if (multi_def.empty()) return false;
  LOG(DEBUG4) << multi_def.size() << " gates are multiply defined.";
  for (const auto& def : multi_def) {
    LOG(DEBUG5) << "Gate " << def.first->index() << ": " << def.second.size()
                << " times.";
    for (const IGateWeakPtr& dup : def.second) {
      if (dup.expired()) continue;
      Preprocessor::ReplaceGate(dup.lock(), def.first);
    }
  }
  Preprocessor::ClearConstGates();
  Preprocessor::ClearNullGates();
  return true;
}

void Preprocessor::DetectMultipleDefinitions(
    const IGatePtr& gate,
    std::unordered_map<IGatePtr, std::vector<IGateWeakPtr> >* multi_def,
    GateSet* unique_gates) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  assert(!gate->IsConstant());

  if (!gate->IsModule()) {  // Modules are unique by definition.
    std::pair<IGatePtr, bool> ret = unique_gates->insert(gate);
    assert(ret.first->mark());
    if (!ret.second) {  // The gate is duplicate.
      (*multi_def)[ret.first].push_back(gate);
      return;
    }
  }
  // No redefinition is found for this gate.
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    Preprocessor::DetectMultipleDefinitions(arg.second, multi_def,
                                            unique_gates);
  }
}

void Preprocessor::DetectModules() noexcept {
  assert(const_gates_.empty());
  assert(null_gates_.empty());
  IGatePtr root_gate = graph_->root();  // Does not change in this algorithm.
  // First stage, traverse the graph depth-first for gates
  // and indicate visit time for each node.
  LOG(DEBUG4) << "Assigning timings to nodes...";
  graph_->ClearNodeVisits();
  Preprocessor::AssignTiming(0, root_gate);
  LOG(DEBUG4) << "Timings are assigned to nodes.";

  graph_->ClearGateMarks();
  Preprocessor::FindModules(root_gate);

  assert(!root_gate->Revisited());  // Sanity checks.
  assert(root_gate->min_time() == 1);
  assert(root_gate->max_time() == root_gate->ExitTime());
}

int Preprocessor::AssignTiming(int time, const IGatePtr& gate) noexcept {
  if (gate->Visit(++time)) return time;  // Revisited gate.
  assert(gate->constant_args().empty());

  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    time = Preprocessor::AssignTiming(time, arg.second);
  }
  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    arg.second->Visit(++time);  // Enter the leaf.
    arg.second->Visit(time);  // Exit at the same time.
  }
  bool re_visited = gate->Visit(++time);  // Exiting the gate in second visit.
  assert(!re_visited);  // No cyclic visiting.
  return time;
}

void Preprocessor::FindModules(const IGatePtr& gate) noexcept {
  if (gate->mark()) return;
  gate->mark(true);
  int enter_time = gate->EnterTime();
  int exit_time = gate->ExitTime();
  int min_time = enter_time;
  int max_time = exit_time;

  std::vector<std::pair<int, NodePtr>> non_shared_args;
  std::vector<std::pair<int, NodePtr>> modular_args;
  std::vector<std::pair<int, NodePtr>> non_modular_args;

  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    IGatePtr arg_gate = arg.second;
    Preprocessor::FindModules(arg_gate);
    if (arg_gate->IsModule() && !arg_gate->Revisited()) {
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

  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    VariablePtr var = arg.second;
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
  if (!gate->IsModule() && min_time == enter_time && max_time == exit_time) {
    LOG(DEBUG4) << "Found original module: G" << gate->index();
    assert(non_modular_args.empty());
    gate->TurnModule();
  }

  max_time = std::max(max_time, gate->LastVisit());
  gate->min_time(min_time);
  gate->max_time(max_time);

  Preprocessor::ProcessModularArgs(gate, non_shared_args, &modular_args,
                                   &non_modular_args);
}

void Preprocessor::ProcessModularArgs(
    const IGatePtr& gate,
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
      Preprocessor::CreateNewModule(gate, non_shared_args);

      Preprocessor::FilterModularArgs(modular_args, non_modular_args);
      assert(modular_args->size() != 1 && "One modular arg is non-shared.");
      std::vector<std::vector<std::pair<int, NodePtr>>> groups;
      Preprocessor::GroupModularArgs(*modular_args, &groups);
      Preprocessor::CreateNewModules(gate, *modular_args, groups);
      break;
    }
    default:
      assert("More complex gates are considered impossible to sub-modularize!");
  }
}

IGatePtr Preprocessor::CreateNewModule(
    const IGatePtr& gate,
    const std::vector<std::pair<int, NodePtr>>& args) noexcept {
  IGatePtr module;  // Empty pointer as an indication of a failure.
  if (args.empty()) return module;
  if (args.size() == 1) return module;
  if (args.size() == gate->args().size()) {
    assert(gate->IsModule());
    return module;
  }
  assert(args.size() < gate->args().size());
  switch (gate->type()) {
    case kNand:
    case kAnd:
      module = std::make_shared<IGate>(kAnd);
      break;
    case kNor:
    case kOr:
      module = std::make_shared<IGate>(kOr);
      break;
    default:
      return module;  // Cannot create sub-modules for other types.
  }
  module->TurnModule();
  assert(gate->mark());
  module->mark(true);  // Keep consistent marking with the subgraph.
  for (const auto& arg : args) {
    gate->TransferArg(arg.first, module);
  }
  gate->AddArg(module->index(), module);
  assert(gate->args().size() > 1);
  LOG(DEBUG4) << "Created a module G" << module->index() << " with "
              << args.size() << " arguments for G" << gate->index();
  return module;
}

void Preprocessor::FilterModularArgs(
    std::vector<std::pair<int, NodePtr>>* modular_args,
    std::vector<std::pair<int, NodePtr>>* non_modular_args) noexcept {
  if (modular_args->empty() || non_modular_args->empty()) return;
  std::vector<std::pair<int, NodePtr>> new_non_modular;
  std::vector<std::pair<int, NodePtr>> still_modular;
  for (const auto& mod_arg : *modular_args) {
    int min = mod_arg.second->min_time();
    int max = mod_arg.second->max_time();
    bool non_module = false;
    for (const auto& non_mod_arg : *non_modular_args) {
      bool overlap = DetectOverlap(min, max,
                                   non_mod_arg.second->min_time(),
                                   non_mod_arg.second->max_time());
      if (overlap) {
        non_module = true;
        break;
      }
    }
    if (non_module) {
      new_non_modular.push_back(mod_arg);
    } else {
      still_modular.push_back(mod_arg);
    }
  }
  Preprocessor::FilterModularArgs(&still_modular, &new_non_modular);
  *modular_args = still_modular;
  non_modular_args->insert(non_modular_args->end(), new_non_modular.begin(),
                           new_non_modular.end());
}

void Preprocessor::GroupModularArgs(
    const std::vector<std::pair<int, NodePtr>>& modular_args,
    std::vector<std::vector<std::pair<int, NodePtr>>>* groups) noexcept {
  if (modular_args.empty()) return;
  assert(modular_args.size() > 1);
  assert(groups->empty());
  std::list<std::pair<int, NodePtr>> member_list(modular_args.begin(),
                                                 modular_args.end());
  while (!member_list.empty()) {
    std::vector<std::pair<int, NodePtr>> group;
    NodePtr first_member = member_list.front().second;
    group.push_back(member_list.front());
    member_list.pop_front();
    int low = first_member->min_time();
    int high = first_member->max_time();

    int prev_size = 0;  // To track the addition of a new member into the group.
    while (prev_size < group.size()) {
      prev_size = group.size();
      std::list<std::pair<int, NodePtr>>::iterator it;
      for (it = member_list.begin(); it != member_list.end();) {
        int min = it->second->min_time();
        int max = it->second->max_time();
        if (DetectOverlap(min, max, low, high)) {
          group.push_back(*it);
          low = std::min(min, low);
          high = std::max(max, high);
          std::list<std::pair<int, NodePtr> >::iterator it_erase = it;
          ++it;
          member_list.erase(it_erase);
        } else {
          ++it;
        }
      }
    }
    assert(group.size() > 1);
    groups->emplace_back(group);
  }
  LOG(DEBUG4) << "Grouped modular args in " << groups->size() << " group(s).";
  assert(!groups->empty());
}

void Preprocessor::CreateNewModules(
    const IGatePtr& gate,
    const std::vector<std::pair<int, NodePtr>>& modular_args,
    const std::vector<std::vector<std::pair<int, NodePtr>>>& groups) noexcept {
  if (modular_args.empty()) return;
  assert(modular_args.size() > 1);
  assert(!groups.empty());
  if (modular_args.size() == gate->args().size() && groups.size() == 1) {
    assert(gate->IsModule());
    return;
  }
  IGatePtr main_arg;

  if (modular_args.size() == gate->args().size()) {
    assert(groups.size() > 1);
    assert(gate->IsModule());
    main_arg = gate;
  } else {
    main_arg = Preprocessor::CreateNewModule(gate, modular_args);
    assert(main_arg);
  }
  for (const auto& group : groups) {
    Preprocessor::CreateNewModule(main_arg, group);
  }
}

void Preprocessor::GatherModules(std::vector<IGateWeakPtr>* modules) noexcept {
  graph_->ClearGateMarks();
  std::queue<IGatePtr> gates_queue;
  IGatePtr root = graph_->root();
  assert(!root->mark());
  assert(root->IsModule());
  root->mark(true);
  modules->push_back(root);
  gates_queue.push(root);
  while (!gates_queue.empty()) {
    IGatePtr gate = gates_queue.front();
    gates_queue.pop();
    assert(gate->mark());
    for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
      IGatePtr arg_gate = arg.second;
      assert(!arg_gate->IsConstant());
      if (arg_gate->mark()) continue;
      arg_gate->mark(true);
      gates_queue.push(arg_gate);
      if (arg_gate->IsModule()) modules->push_back(arg_gate);
    }
  }
}

bool Preprocessor::MergeCommonArgs() noexcept {
  assert(null_gates_.empty());
  assert(const_gates_.empty());
  bool changed = false;

  LOG(DEBUG4) << "Merging common arguments for AND gates...";
  bool ret = Preprocessor::MergeCommonArgs(kAnd);
  if (ret) changed = true;
  LOG(DEBUG4) << "Finished merging for AND gates!";

  LOG(DEBUG4) << "Merging common arguments for OR gates...";
  ret = Preprocessor::MergeCommonArgs(kOr);
  if (ret) changed = true;
  LOG(DEBUG4) << "Finished merging for OR gates!";

  assert(null_gates_.empty());
  assert(const_gates_.empty());
  return changed;
}

bool Preprocessor::MergeCommonArgs(Operator op) noexcept {
  assert(op == kAnd || op == kOr);
  graph_->ClearNodeCounts();
  graph_->ClearGateMarks();
  // Gather and group gates
  // by their operator types and common arguments.
  Preprocessor::MarkCommonArgs(graph_->root(), op);
  graph_->ClearGateMarks();
  std::vector<IGateWeakPtr> modules;
  Preprocessor::GatherModules(&modules);
  graph_->ClearGateMarks();
  LOG(DEBUG4) << "Working with " << modules.size() << " modules...";
  bool changed = false;
  for (const auto& module : modules) {
    if (module.expired()) continue;
    IGatePtr root = module.lock();
    MergeTable::Candidates candidates;
    Preprocessor::GatherCommonArgs(root, op, &candidates);
    graph_->ClearGateMarks(root);
    if (candidates.size() < 2) continue;
    Preprocessor::FilterMergeCandidates(&candidates);
    if (candidates.size() < 2) continue;
    std::vector<MergeTable::Candidates> groups;
    Preprocessor::GroupCandidatesByArgs(candidates, &groups);
    for (const auto& group : groups) {
      // Finding common parents for the common arguments.
      MergeTable::Collection parents;
      Preprocessor::GroupCommonParents(2, group, &parents);
      if (parents.empty()) continue;  // No candidates for merging.
      changed = true;
      LOG(DEBUG4) << "Merging " << parents.size() << " collection...";
      MergeTable table;
      Preprocessor::GroupCommonArgs(parents, &table);
      LOG(DEBUG4) << "Transforming " << table.groups.size()
                  << " table groups...";
      for (MergeTable::MergeGroup& member_group : table.groups) {
        Preprocessor::TransformCommonArgs(&member_group);
      }
    }
    Preprocessor::ClearConstGates();
    Preprocessor::ClearNullGates();
  }
  return changed;
}

void Preprocessor::MarkCommonArgs(const IGatePtr& gate, Operator op) noexcept {
  if (gate->mark()) return;
  gate->mark(true);

  bool in_group = gate->type() == op;

  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    IGatePtr arg_gate = arg.second;
    assert(!arg_gate->IsConstant());
    Preprocessor::MarkCommonArgs(arg_gate, op);
    if (in_group) arg_gate->AddCount(arg.first > 0);
  }

  if (!in_group) return;  // No need to visit leaf variables.

  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    arg.second->AddCount(arg.first > 0);
  }
  assert(gate->constant_args().empty());
}

void Preprocessor::GatherCommonArgs(const IGatePtr& gate, Operator op,
                                    MergeTable::Candidates* group) noexcept {
  if (gate->mark()) return;
  gate->mark(true);

  bool in_group = gate->type() == op;

  std::vector<int> common_args;
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    IGatePtr arg_gate = arg.second;
    assert(!arg_gate->IsConstant());
    if (!arg_gate->IsModule())
      Preprocessor::GatherCommonArgs(arg_gate, op, group);
    if (!in_group) continue;
    int count = arg.first > 0 ? arg_gate->pos_count() : arg_gate->neg_count();
    if (count > 1) common_args.push_back(arg.first);
  }

  if (!in_group) return;  // No need to check variables.

  for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
    VariablePtr var = arg.second;
    int count = arg.first > 0 ? var->pos_count() : var->neg_count();
    if (count > 1) common_args.push_back(arg.first);
  }
  assert(gate->constant_args().empty());

  if (common_args.size() < 2) return;  // Can't be merged anyway.

  std::sort(common_args.begin(), common_args.end());  // Unique and stable.
  group->emplace_back(gate, common_args);
}

void Preprocessor::FilterMergeCandidates(
    MergeTable::Candidates* candidates) noexcept {
  assert(candidates->size() > 1);
  std::stable_sort(
      candidates->begin(), candidates->end(),
      [](const MergeTable::Candidate& lhs, const MergeTable::Candidate& rhs) {
        return lhs.second.size() < rhs.second.size();
      });
  bool cleanup = false;  // Clean constant or NULL type gates.
  for (auto it = candidates->begin(); it != candidates->end(); ++it) {
    IGatePtr gate = it->first;
    MergeTable::CommonArgs& common_args = it->second;
    if (gate->args().size() == 1) continue;
    if (gate->IsConstant()) continue;
    if (common_args.size() < 2) continue;
    if (gate->args().size() != common_args.size()) continue;  // Not perfect.
    auto it_next = it;
    for (++it_next; it_next != candidates->end(); ++it_next) {
      IGatePtr comp_gate = it_next->first;
      MergeTable::CommonArgs& comp_args = it_next->second;
      if (comp_args.size() < common_args.size()) continue;  // Changed gate.
      assert(!comp_gate->IsConstant());
      if (!std::includes(comp_args.begin(), comp_args.end(),
                         common_args.begin(), common_args.end())) continue;

      MergeTable::CommonArgs diff;
      std::set_difference(comp_args.begin(), comp_args.end(),
                          common_args.begin(), common_args.end(),
                          std::back_inserter(diff));
      diff.push_back(gate->index());
      std::sort(diff.begin(), diff.end());
      diff.erase(std::unique(diff.begin(), diff.end()), diff.end());
      comp_args = diff;
      for (int index : common_args) comp_gate->EraseArg(index);
      comp_gate->AddArg(gate->index(), gate);
      if (comp_gate->IsConstant()) {  // Complement of gate is arg.
        const_gates_.push_back(comp_gate);
        comp_args.clear();
        cleanup = true;
      } else if (comp_gate->args().size() == 1) {  // Perfect substitution.
        comp_gate->type(kNull);
        null_gates_.push_back(comp_gate);
        assert(comp_args.size() == 1);
        cleanup = true;
      } else if (comp_args.size() == 1) {
        cleanup = true;
      }
    }
  }
  if (!cleanup) return;
  candidates->erase(std::remove_if(candidates->begin(), candidates->end(),
                                   [](const MergeTable::Candidate& mem) {
                      return mem.first->IsConstant() ||
                             mem.first->type() == kNull ||
                             mem.second.size() == 1;
                    }),
                    candidates->end());
}

void Preprocessor::GroupCandidatesByArgs(
    const MergeTable::Candidates& candidates,
    std::vector<MergeTable::Candidates>* groups) noexcept {
  if (candidates.empty()) return;
  assert(candidates.size() > 1);
  assert(groups->empty());
  std::list<MergeTable::Candidate> member_list(candidates.begin(),
                                               candidates.end());
  while (!member_list.empty()) {
    MergeTable::Candidates group;
    IGatePtr first_member = member_list.front().first;  /// @todo Unused?
    const MergeTable::CommonArgs& common_args = member_list.front().second;
    std::set<int> group_args(common_args.begin(), common_args.end());
    assert(group_args.size() > 1);
    group.push_back(member_list.front());
    member_list.pop_front();

    int prev_size = 0;  // To track the change in group arguments.
    while (prev_size < group_args.size()) {
      prev_size = group_args.size();
      for (auto it = member_list.begin(); it != member_list.end();) {
        std::vector<int> overlap;
        const MergeTable::CommonArgs& member_args = it->second;
        std::set_intersection(member_args.begin(), member_args.end(),
                              group_args.begin(), group_args.end(),
                              std::back_inserter(overlap));
        if (!overlap.empty()) {
          group.push_back(*it);
          group_args.insert(member_args.begin(), member_args.end());
          auto it_erase = it;
          ++it;
          member_list.erase(it_erase);
        } else {
          ++it;
        }
      }
    }
    if (group.size() > 1) groups->push_back(group);  // Discard single members.
  }
  bool any_groups = !groups->empty();
  BLOG(DEBUG4, any_groups) << "Grouped merge candidates in " << groups->size()
                           << " group(s).";
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
      std::set_intersection(args_gate.begin(), args_gate.end(),
                            args_comp.begin(), args_comp.end(),
                            std::back_inserter(common));
      if (common.size() < num_common_args) continue;  // Doesn't satisfy.
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
  std::stable_sort(
      all_options.begin(), all_options.end(),
      [](const MergeTable::Option& lhs, const MergeTable::Option& rhs) {
        return lhs.first.size() < rhs.first.size();
      });

  while (!all_options.empty()) {
    MergeTable::OptionGroup best_group;
    Preprocessor::FindOptionGroup(&all_options, &best_group);
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
      std::vector<int> common;
      std::set_intersection(option.first.begin(), option.first.end(),
                            args.begin(), args.end(),
                            std::back_inserter(common));
      if (common.empty()) continue;  // Doesn't affect this option.
      MergeTable::CommonParents& parents = option.second;
      for (const IGatePtr& gate : gates) parents.erase(gate);
    }
    all_options.erase(std::remove_if(all_options.begin(), all_options.end(),
                                     [](const MergeTable::Option& option) {
                        return option.second.size() < 2;
                      }),
                      all_options.end());
  }
}

void Preprocessor::FindOptionGroup(
    MergeTable::MergeGroup* all_options,
    MergeTable::OptionGroup* best_group) noexcept {
  assert(best_group->empty());
  // Find the best starting option.
  MergeTable::MergeGroup::iterator best_option;
  Preprocessor::FindBaseOption(all_options, &best_option);
  bool best_is_found = best_option != all_options->end();
  auto it = best_is_found ? best_option : all_options->begin();
  for (; it != all_options->end(); ++it) {
    MergeTable::OptionGroup group = {&*it};
    auto it_next = it;
    for (++it_next; it_next != all_options->end(); ++it_next) {
      MergeTable::Option* candidate = &*it_next;
      bool superset = std::includes(candidate->first.begin(),
                                    candidate->first.end(),
                                    group.back()->first.begin(),
                                    group.back()->first.end());
      if (!superset) continue;  // Does not include all the arguments.
      bool parents = std::includes(group.back()->second.begin(),
                                   group.back()->second.end(),
                                   candidate->second.begin(),
                                   candidate->second.end());
      if (!parents) continue;  // Parents do not match.
      group.push_back(candidate);
    }
    if (group.size() > best_group->size()) {  // The more members, the merrier.
      *best_group = group;
    } else if (group.size() == best_group->size()) {  // Optimistic choice.
      if (group.front()->second.size() < best_group->front()->second.size())
        *best_group = group;  // The fewer parents, the more room for others.
    }
    if (best_is_found) break;
  }
}

void Preprocessor::FindBaseOption(
    MergeTable::MergeGroup* all_options,
    MergeTable::MergeGroup::iterator* best_option) noexcept {
  *best_option = all_options->end();
  int best_counts[3] = {};  // The number of extra parents.
  for (auto it = all_options->begin(); it != all_options->end(); ++it) {
    int num_parents = it->second.size();
    IGatePtr parent = *it->second.begin();  // Representative.
    const MergeTable::CommonArgs& args = it->first;
    int cur_counts[3] = {};
    for (int index : args) {
      NodePtr arg = parent->GetArg(index);
      int extra_count = arg->parents().size() - num_parents;
      if (extra_count > 2) continue;  // Optimal decision criterion.
      ++cur_counts[extra_count];  // Logging extra parents.
      if (cur_counts[0] > 1) break;  // Modular option is found.
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
      best_counts[0] = cur_counts[0];
      best_counts[1] = cur_counts[1];
      best_counts[2] = cur_counts[2];
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
    IGatePtr parent = *common_parents.begin();  // To get the arguments.
    assert(parent->args().size() > 1);
    auto merge_gate = std::make_shared<IGate>(parent->type());
    for (int index : common_args) {
      parent->ShareArg(index, merge_gate);
      for (const IGatePtr& common_parent : common_parents) {
        common_parent->EraseArg(index);
      }
    }
    for (const IGatePtr& common_parent : common_parents) {
      common_parent->AddArg(merge_gate->index(), merge_gate);
      if (common_parent->args().size() == 1) {
        common_parent->type(kNull);  // Assumes AND/OR gates only.
        null_gates_.push_back(common_parent);
      }
      assert(!common_parent->IsConstant());
    }
    // Substitute args in superset common args with the new gate.
    MergeTable::MergeGroup::iterator it_rest = it;
    for (++it_rest; it_rest != group->end(); ++it_rest) {
      MergeTable::CommonArgs& set_args = it_rest->first;
      assert(set_args.size() > common_args.size());
      // Note: it is assumed that common_args is a proper subset of set_args.
      std::vector<int> diff;
      std::set_difference(set_args.begin(), set_args.end(),
                          common_args.begin(), common_args.end(),
                          std::back_inserter(diff));
      assert(diff.size() == (set_args.size() - common_args.size()));
      assert(merge_gate->index() > diff.back());
      diff.push_back(merge_gate->index());  // Assumes sequential indexing.
      set_args = diff;
      assert(it_rest->first.size() == diff.size());
    }
  }
}

bool Preprocessor::DetectDistributivity() noexcept {
  assert(const_gates_.empty());
  assert(null_gates_.empty());
  graph_->ClearGateMarks();
  bool changed = Preprocessor::DetectDistributivity(graph_->root());
  assert(const_gates_.empty());
  Preprocessor::ClearNullGates();
  return changed;
}

bool Preprocessor::DetectDistributivity(const IGatePtr& gate) noexcept {
  if (gate->mark()) return false;
  gate->mark(true);
  assert(!gate->IsConstant());
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
  std::vector<IGatePtr> candidates;
  // Collect child gates of distributivity type.
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    IGatePtr child_gate = arg.second;
    bool ret = Preprocessor::DetectDistributivity(child_gate);
    assert(!child_gate->IsConstant() && "Impossible state.");
    if (ret) changed = true;
    if (distr_type == kNull) continue;  // Distributivity is not possible.
    if (arg.first < 0) continue;  // Does not work on negation.
    if (child_gate->IsModule()) continue;  // Can't have common arguments.
    if (child_gate->type() == distr_type) candidates.push_back(child_gate);
  }
  if (Preprocessor::HandleDistributiveArgs(gate, distr_type, &candidates))
    changed = true;
  return changed;
}

bool Preprocessor::HandleDistributiveArgs(
    const IGatePtr& gate,
    Operator distr_type,
    std::vector<IGatePtr>* candidates) noexcept {
  if (candidates->empty()) return false;
  assert(gate->args().size() > 1 && "Malformed parent gate.");
  assert(distr_type == kAnd || distr_type == kOr);
  bool changed = Preprocessor::FilterDistributiveArgs(gate, candidates);
  if (candidates->size() < 2) return changed;
  // Detecting a combination
  // that gives the most optimization is combinatorial.
  // The problem is similar to merging common arguments of gates.
  MergeTable::Candidates all_candidates;
  for (const IGatePtr& candidate : *candidates) {
    all_candidates.emplace_back(
        candidate,
        std::vector<int>(candidate->args().begin(), candidate->args().end()));
  }
  LOG(DEBUG5) << "Considering " << all_candidates.size() << " candidates...";
  MergeTable::Collection options;
  Preprocessor::GroupCommonParents(1, all_candidates, &options);
  if (options.empty()) return false;
  LOG(DEBUG4) << "Got " << options.size() << " distributive option(s).";

  MergeTable table;
  Preprocessor::GroupDistributiveArgs(options, &table);
  assert(!table.groups.empty());
  LOG(DEBUG4) << "Found " << table.groups.size() << " distributive group(s).";
  // Sanitize the table with single parent gates only.
  for (MergeTable::MergeGroup& group : table.groups) {
    MergeTable::Option& base_option = group.front();
    std::vector<std::pair<IGatePtr, IGatePtr>> to_swap;
    for (const IGatePtr& member : base_option.second) {
      assert(!member->parents().empty());
      if (member->parents().size() > 1) {
        IGatePtr clone = member->Clone();
        clone->mark(true);
        to_swap.emplace_back(member, clone);
      }
    }
    for (const auto& gates : to_swap) {
      gate->EraseArg(gates.first->index());
      gate->AddArg(gates.second->index(), gates.second);
      for (MergeTable::Option& option : group) {
        if (option.second.count(gates.first)) {
          option.second.erase(gates.first);
          option.second.insert(gates.second);
        }
      }
    }
  }

  for (MergeTable::MergeGroup& group : table.groups) {
    Preprocessor::TransformDistributiveArgs(gate, distr_type, &group);
  }
  assert(!gate->args().empty());
  return true;
}

bool Preprocessor::FilterDistributiveArgs(
    const IGatePtr& gate,
    std::vector<IGatePtr>* candidates) noexcept {
  assert(!candidates->empty());
  // Handling a special case of fast constant propagation.
  std::vector<int> to_erase;  // Late erase for more opportunities.
  for (const IGatePtr& candidate : *candidates) {  // All of them are positive.
    std::vector<int> common;
    std::set_intersection(candidate->args().begin(), candidate->args().end(),
                          gate->args().begin(), gate->args().end(),
                          std::back_inserter(common));
    if (!common.empty()) to_erase.push_back(candidate->index());
  }
  bool changed = !to_erase.empty();
  for (int index : to_erase) {
    gate->EraseArg(index);
    candidates->erase(std::find_if(candidates->begin(), candidates->end(),
                                   [&index](const IGatePtr& candidate) {
      return candidate->index() == index;
    }));
  }
  // Sort in descending size of gate arguments.
  std::sort(candidates->begin(), candidates->end(),
            [](const IGatePtr& lhs, const IGatePtr rhs) {
    return lhs->args().size() > rhs->args().size();
  });
  std::vector<IGatePtr> exclusive;  // No candidate is a subset of another.
  while (!candidates->empty()) {
    IGatePtr sub = candidates->back();
    candidates->pop_back();
    exclusive.push_back(sub);
    for (const IGatePtr& super : *candidates) {
      if (std::includes(super->args().begin(), super->args().end(),
                        sub->args().begin(), sub->args().end())) {
        changed = true;
        gate->EraseArg(super->index());
      }
    }
    candidates->erase(std::remove_if(candidates->begin(), candidates->end(),
                                     [&sub](const IGatePtr& super) {
                        return std::includes(
                            super->args().begin(), super->args().end(),
                            sub->args().begin(), sub->args().end());
                      }),
                      candidates->end());
  }
  *candidates = exclusive;
  assert(!gate->args().empty());
  if (gate->args().size() == 1) {
    switch (gate->type()) {
      case kAnd:
      case kOr:
        gate->type(kNull);
        null_gates_.push_back(gate);
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
  std::stable_sort(
      all_options.begin(), all_options.end(),
      [](const MergeTable::Option& lhs, const MergeTable::Option& rhs) {
        return lhs.first.size() < rhs.first.size();
      });

  while (!all_options.empty()) {
    MergeTable::OptionGroup best_group;
    Preprocessor::FindOptionGroup(&all_options, &best_group);
    MergeTable::MergeGroup merge_group;  // The group to go into the table.
    for (MergeTable::Option* member : best_group) {
      merge_group.push_back(*member);
      member->second.clear();  // To remove the best group from the all options.
    }
    table->groups.push_back(merge_group);

    const MergeTable::CommonParents& gates = merge_group.front().second;
    for (MergeTable::Option& option : all_options) {
      MergeTable::CommonParents& parents = option.second;
      for (const IGatePtr& gate : gates) parents.erase(gate);
    }
    all_options.erase(std::remove_if(all_options.begin(), all_options.end(),
                                     [](const MergeTable::Option& option) {
                        return option.second.size() < 2;
                      }),
                      all_options.end());
  }
}

void Preprocessor::TransformDistributiveArgs(
    const IGatePtr& gate,
    Operator distr_type,
    MergeTable::MergeGroup* group) noexcept {
  if (group->empty()) return;
  assert(distr_type == kAnd || distr_type == kOr);
  const MergeTable::Option& base_option = group->front();
  const MergeTable::CommonArgs& args = base_option.first;
  const MergeTable::CommonParents& gates = base_option.second;

  IGatePtr new_parent;
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
    new_parent = std::make_shared<IGate>(distr_type);
    new_parent->mark(true);
    gate->AddArg(new_parent->index(), new_parent);
  }

  auto sub_parent =
      std::make_shared<IGate>(distr_type == kAnd ? kOr : kAnd);
  sub_parent->mark(true);
  new_parent->AddArg(sub_parent->index(), sub_parent);

  IGatePtr rep = *gates.begin();  // Representative of common parents.
  // Getting the common part of the distributive equation.
  for (int index : args) {  // May be negative.
    if (rep->gate_args().count(index)) {
      IGatePtr common = rep->gate_args().find(index)->second;
      new_parent->AddArg(index, common);
    } else {
      VariablePtr common = rep->variable_args().find(index)->second;
      new_parent->AddArg(index, common);
    }
  }

  // Removing the common part from the sub-equations.
  for (const IGatePtr& member : gates) {
    assert(member->parents().size() == 1);
    gate->EraseArg(member->index());

    sub_parent->AddArg(member->index(), member);
    for (int index : args) {
      member->EraseArg(index);
    }
    assert(!member->args().empty());  // Assumes that filtering is done.
    if (member->args().size() == 1) {
      member->type(kNull);
      null_gates_.push_back(member);
    }
  }
  // Cleaning the arguments from the group.
  MergeTable::MergeGroup::iterator it = group->begin();
  for (++it; it != group->end(); ++it) {
    MergeTable::Option& super = *it;
    MergeTable::CommonArgs& super_args = super.first;
    for (int index : args) {
      std::vector<int>::iterator it_index =
          std::lower_bound(super_args.begin(), super_args.end(), index);
      assert(it_index != super_args.end());  // The index should exist.
      super_args.erase(it_index);
    }
  }
  group->erase(group->begin());
  Preprocessor::TransformDistributiveArgs(sub_parent, distr_type, group);
}

void Preprocessor::BooleanOptimization() noexcept {
  assert(const_gates_.empty());
  assert(null_gates_.empty());
  graph_->ClearGateMarks();
  graph_->ClearOptiValues();
  graph_->ClearDescendantMarks();
  if (!graph_->root()->IsModule()) graph_->root()->TurnModule();

  std::vector<IGateWeakPtr> common_gates;
  std::vector<std::weak_ptr<Variable>> common_variables;
  Preprocessor::GatherCommonNodes(&common_gates, &common_variables);
  for (const auto& gate : common_gates) Preprocessor::ProcessCommonNode(gate);
  for (const auto& var : common_variables) Preprocessor::ProcessCommonNode(var);
}

void Preprocessor::GatherCommonNodes(
      std::vector<IGateWeakPtr>* common_gates,
      std::vector<std::weak_ptr<Variable>>* common_variables) noexcept {
  graph_->ClearNodeVisits();
  std::queue<IGatePtr> gates_queue;
  gates_queue.push(graph_->root());
  while (!gates_queue.empty()) {
    IGatePtr gate = gates_queue.front();
    gates_queue.pop();
    for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
      IGatePtr arg_gate = arg.second;
      assert(!arg_gate->IsConstant());
      if (arg_gate->Visited()) continue;
      arg_gate->Visit(1);
      gates_queue.push(arg_gate);
      if (arg_gate->parents().size() > 1) common_gates->push_back(arg_gate);
    }
    for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
      VariablePtr var = arg.second;
      if (var->Visited()) continue;
      var->Visit(1);
      if (var->parents().size() > 1) common_variables->push_back(var);
    }
  }
}

template <class N>
void Preprocessor::ProcessCommonNode(
    const std::weak_ptr<N>& common_node) noexcept {
  assert(const_gates_.empty());
  assert(null_gates_.empty());
  if (common_node.expired()) return;  // The node has been deleted.

  std::shared_ptr<N> node = common_node.lock();

  if (node->parents().size() == 1) return;  // The extra parent is deleted.
  IGatePtr root;
  Preprocessor::MarkAncestors(node, &root);
  assert(root && "Marking ancestors ended without guaranteed module.");
  assert(root->mark() && "Graph gate marks are not cleaned.");
  assert(!root->opti_value() && "Optimization values are corrupted.");
  assert(!node->opti_value() && "Optimization values are corrupted.");
  node->opti_value(1);  // Setting for failure.

  int mult_tot = node->parents().size();  // Total multiplicity.
  assert(mult_tot > 1);
  mult_tot += Preprocessor::PropagateState(root, node);
  assert(!root->mark() && "Partial unmarking failed.");
  assert(root->descendant() == node->index() && "Ancestors are not indexed.");

  // The results of the failure propagation.
  std::unordered_map<int, IGateWeakPtr> destinations;
  int num_dest = 0;  // This is not the same as the size of destinations.
  if (root->opti_value()) {  // The root gate received the state.
    destinations.emplace(root->index(), root);
    num_dest = 1;
  } else {
    num_dest = Preprocessor::CollectStateDestinations(root, node->index(),
                                                      &destinations);
  }

  if (num_dest > 0 && num_dest < mult_tot) {  // Redundancy detection criterion.
    assert(!destinations.empty());
    std::vector<IGateWeakPtr> redundant_parents;
    Preprocessor::CollectRedundantParents(node, &destinations,
                                          &redundant_parents);
    if (!redundant_parents.empty()) {  // Note: empty destinations is OK!
      LOG(DEBUG4) << "Node " << node->index() << ": "
                  << redundant_parents.size() << " redundant parent(s) and "
                  << destinations.size() << " failure destination(s)";
      Preprocessor::ProcessRedundantParents(node, redundant_parents);
      Preprocessor::ProcessStateDestinations(node, destinations);
    }
  }
  Preprocessor::ClearStateMarks(root);
  node->opti_value(0);
  Preprocessor::ClearConstGates();
  Preprocessor::ClearNullGates();
}

void Preprocessor::MarkAncestors(const NodePtr& node,
                                 IGatePtr* module) noexcept {
  for (const std::pair<const int, IGateWeakPtr>& member : node->parents()) {
    assert(!member.second.expired());
    IGatePtr parent = member.second.lock();
    if (parent->mark()) continue;
    parent->mark(true);
    if (parent->IsModule()) {  // Do not mark further than independent subgraph.
      assert(!*module);
      *module = parent;
      continue;
    }
    Preprocessor::MarkAncestors(parent, module);
  }
}

int Preprocessor::PropagateState(const IGatePtr& gate,
                                 const NodePtr& node) noexcept {
  if (!gate->mark()) return 0;
  gate->mark(false);  // Cleaning up the marks of the ancestors.
  assert(!gate->descendant() && "Descendant marks are corrupted.");
  gate->descendant(node->index());  // Setting ancestorship mark.
  assert(!gate->opti_value());
  int mult_tot = 0;  // The total multiplicity of the subgraph.
  int num_failure = 0;  // The number of failed arguments.
  int num_success = 0;  // The number of success arguments.
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    IGatePtr arg_gate = arg.second;
    mult_tot += Preprocessor::PropagateState(arg_gate, node);
    assert(!arg_gate->mark());
    int failed = arg_gate->opti_value() * GetSign(arg.first);
    assert(!failed || failed == -1 || failed == 1);
    if (failed == 1) {
      ++num_failure;
    } else if (failed == -1) {
      ++num_success;
    }  // Ignore when 0.
  }
  if (node->parents().count(gate->index())) {  // Try to find the basic event.
    int failed = gate->variable_args().count(node->index());
    if (!failed) failed = -gate->variable_args().count(-node->index());
    failed *= node->opti_value();
    if (failed == 1) {
      ++num_failure;
    } else if (failed == -1) {
      ++num_success;
    }  // Ignore when 0.
  }
  assert(gate->constant_args().empty());
  Preprocessor::DetermineGateState(gate, num_failure, num_success);
  int mult_add = gate->parents().size();
  if (!gate->opti_value() || mult_add < 2 ) mult_add = 0;
  return mult_tot + mult_add;
}

void Preprocessor::DetermineGateState(const IGatePtr& gate, int num_failure,
                                      int num_success) noexcept {
  assert(!gate->opti_value() && "Unclear initial optimization value.");
  assert(num_failure >= 0 && "Illegal arguments or corrupted state.");
  assert(num_success >= 0 && "Illegal arguments or corrupted state.");
  assert(!(num_success && graph_->coherent()) && "Impossible state.");

  if (!(num_success + num_failure)) return;  // Undetermined 0 state.
  auto ComputeState = [&num_failure, &num_success](int req_failure,
                                                   int req_success) {
    if (num_failure >= req_failure) return 1;
    if (num_success >= req_success) return -1;
    return 0;
  };
  switch (gate->type()) {
    case kNull:
      assert((num_failure + num_success) == 1);
      gate->opti_value(ComputeState(1, 1));
      break;
    case kOr:
      gate->opti_value(ComputeState(1, gate->args().size()));
      break;
    case kAnd:
      gate->opti_value(ComputeState(gate->args().size(), 1));
      break;
    case kVote:
      assert(gate->args().size() > gate->vote_number());
      gate->opti_value(
          ComputeState(gate->vote_number(),
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
      gate->opti_value(-ComputeState(1, 1));
      break;
    case kNand:
      gate->opti_value(-ComputeState(gate->args().size(), 1));
      break;
    case kNor:
      gate->opti_value(-ComputeState(1, gate->args().size()));
      break;
  }
}

int Preprocessor::CollectStateDestinations(
    const IGatePtr& gate,
    int index,
    std::unordered_map<int, IGateWeakPtr>* destinations) noexcept {
  if (!gate->descendant()) return 0;  // Deal with ancestors only.
  assert(gate->descendant() == index && "Corrupted descendant marks.");
  if (gate->opti_value()) return 0;  // Don't change failure information.
  gate->opti_value(2);
  int num_dest = 0;
  for (const std::pair<const int, IGatePtr>& member : gate->gate_args()) {
    IGatePtr arg = member.second;
    num_dest +=
        Preprocessor::CollectStateDestinations(arg, index, destinations);
    if (arg->index() == index) continue;  // The state source.
    if (!arg->opti_value()) continue;  // Indeterminate branches.
    if (arg->opti_value() > 1) continue;  // Not a state destination.
    ++num_dest;  // Optimization value is 1 or -1.
    destinations->emplace(arg->index(), arg);
  }
  return num_dest;
}

void Preprocessor::CollectRedundantParents(
    const NodePtr& node,
    std::unordered_map<int, IGateWeakPtr>* destinations,
    std::vector<IGateWeakPtr>* redundant_parents) noexcept {
  for (const std::pair<const int, IGateWeakPtr>& member : node->parents()) {
    assert(!member.second.expired());
    IGatePtr parent = member.second.lock();
    assert(!parent->mark());
    if (parent->opti_value() == 2) continue;  // Non-redundant parent.
    if (parent->opti_value()) {
      assert(parent->opti_value() == 1 || parent->opti_value() == -1);
      if (destinations->count(parent->index())) {
        Operator type = parent->opti_value() == 1 ? kOr : kAnd;
        if (parent->type() == type &&
            parent->opti_value() == parent->GetArgSign(node)) {
          destinations->erase(parent->index());
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
    const std::vector<IGateWeakPtr>& redundant_parents) noexcept {
  // The node behaves like a constant False for redundant parents.
  for (const IGateWeakPtr& ptr : redundant_parents) {
    if (ptr.expired()) continue;
    IGatePtr parent = ptr.lock();
    parent->ProcessConstantArg(node, node->opti_value() != 1);
    if (parent->IsConstant()) {
      const_gates_.push_back(parent);
    } else if (parent->type() == kNull) {
      null_gates_.push_back(parent);
    }
  }
}

template <class N>
void Preprocessor::ProcessStateDestinations(
    const std::shared_ptr<N>& node,
    const std::unordered_map<int, IGateWeakPtr>& destinations) noexcept {
  for (const auto& ptr : destinations) {
    if (ptr.second.expired()) continue;
    IGatePtr target = ptr.second.lock();
    assert(!target->mark());
    assert(target->opti_value() == 1 || target->opti_value() == -1);
    Operator type = target->opti_value() == 1 ? kOr : kAnd;
    if (target->type() == type) {  // Reuse of an existing gate.
      if (target->IsConstant()) continue;  // No need to process.
      target->AddArg(target->opti_value() * node->index(), node);
      if (target->IsConstant()) const_gates_.push_back(target);
      assert(!(!target->IsConstant() && target->type() == kNull));
      continue;
    }
    auto new_gate = std::make_shared<IGate>(type);
    new_gate->AddArg(target->opti_value() * node->index(), node);
    if (target->IsModule()) {  // Transfer modularity.
      target->DestroyModule();
      new_gate->TurnModule();
    }
    if (target == graph_->root()) {
      graph_->root(new_gate);  // The sign is preserved.
    } else {
      Preprocessor::ReplaceGate(target, new_gate);
    }
    new_gate->AddArg(target->index(), target);  // Only after replacing target!
    new_gate->descendant(node->index());  // Preserve continuity.
  }
}

void Preprocessor::ClearStateMarks(const IGatePtr& gate) noexcept {
  if (!gate->descendant()) return;  // Clean only 'dirty' gates.
  gate->descendant(0);
  gate->opti_value(0);
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    Preprocessor::ClearStateMarks(arg.second);
  }
  for (const std::pair<const int, IGateWeakPtr>& member : gate->parents()) {
    Preprocessor::ClearStateMarks(member.second.lock());  // Due to replacement.
  }
}

bool Preprocessor::DecomposeCommonNodes() noexcept {
  assert(const_gates_.empty());
  assert(null_gates_.empty());

  std::vector<IGateWeakPtr> common_gates;
  std::vector<std::weak_ptr<Variable> > common_variables;
  Preprocessor::GatherCommonNodes(&common_gates, &common_variables);

  graph_->ClearNodeVisits();
  Preprocessor::AssignTiming(0, graph_->root());  // Required for optimization.
  graph_->ClearDescendantMarks();  // Used for ancestor detection.
  graph_->ClearGateMarks();  // Important for linear traversal.

  bool changed = false;
  // The processing is done deepest-layer-first.
  // The deepest-first processing avoids generating extra parents
  // for the nodes that are deep in the graph.
  for (auto it = common_gates.rbegin(); it != common_gates.rend(); ++it) {
    bool ret = Preprocessor::DecompositionProcessor()(*it, this);
    if (ret) changed = true;
  }

  // Variables are processed after gates
  // because, if parent gates are removed,
  // there may be no need to process these variables.
  for (auto it = common_variables.rbegin(); it != common_variables.rend();
       ++it) {
    bool ret = Preprocessor::DecompositionProcessor()(*it, this);
    if (ret) changed = true;
  }
  return changed;
}

bool Preprocessor::DecompositionProcessor::operator()(
    const std::weak_ptr<Node>& common_node,
    Preprocessor* preprocessor) noexcept {
  assert(preprocessor);
  if (common_node.expired()) return false;  // The node has been deleted.
  node_ = common_node.lock();
  if (node_->parents().size() < 2) return false;  // Not common anymore.
  preprocessor_ = preprocessor;
  assert(preprocessor_->const_gates_.empty());
  assert(preprocessor_->null_gates_.empty());
  auto IsDecompositionType = [](Operator type) {  // Possible types for setups.
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
  auto it =
      std::find_if(node_->parents().begin(), node_->parents().end(),
                   [&IsDecompositionType]
                   (const std::pair<const int, IGateWeakPtr>& member) {
                     return IsDecompositionType(member.second.lock()->type());
                   });
  if (it == node_->parents().end()) return false;  // No setups possible.

  assert(2 >
         std::count_if(node_->parents().begin(), node_->parents().end(),
                       [](const std::pair<const int, IGateWeakPtr>& member) {
               return member.second.lock()->IsModule();
         }));

  // Mark parents and ancestors.
  for (const std::pair<const int, IGateWeakPtr>& member : node_->parents()) {
    assert(!member.second.expired());
    IGatePtr parent = member.second.lock();
    DecompositionProcessor::MarkDestinations(parent);
  }
  // Find destinations with particular setups.
  // If a parent gets marked upon destination search,
  // the parent is the destination.
  std::vector<IGateWeakPtr> dest;
  for (const std::pair<const int, IGateWeakPtr>& member : node_->parents()) {
    assert(!member.second.expired());
    IGatePtr parent = member.second.lock();
    if (parent->descendant() == node_->index() &&
        IsDecompositionType(parent->type())) {
      dest.push_back(parent);
    }
  }
  if (dest.empty()) return false;  // No setups are found.

  bool ret = DecompositionProcessor::ProcessDestinations(dest);
  BLOG(DEBUG4, ret) << "Successful decomposition of node " << node_->index();
  return ret;
}

void Preprocessor::DecompositionProcessor::MarkDestinations(
    const IGatePtr& parent) noexcept {
  if (parent->IsModule()) return;  // Limited with independent subgraphs.
  for (const std::pair<const int, IGateWeakPtr>& member : parent->parents()) {
    assert(!member.second.expired());
    IGatePtr ancestor = member.second.lock();
    if (ancestor->descendant() == node_->index()) continue;  // Already marked.
    ancestor->descendant(node_->index());
    DecompositionProcessor::MarkDestinations(ancestor);
  }
}

bool Preprocessor::DecompositionProcessor::ProcessDestinations(
    const std::vector<IGateWeakPtr>& dest) noexcept {
  bool changed = false;
  for (const auto& ptr : dest) {
    if (ptr.expired()) continue;  // Removed by constant propagation.
    IGatePtr parent = ptr.lock();

    // The destination may already be processed
    // in the link of ancestors.
    if (!node_->parents().count(parent->index())) continue;

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
    if (parent->GetArgSign(node_) < 0) state = !state;
    std::pair<int, int> visit_bounds{parent->EnterTime(), parent->ExitTime()};
    assert(!parent->mark() && "Subgraph is not clean!");
    bool ret =
        DecompositionProcessor::ProcessAncestors(parent, state, visit_bounds);
    if (ret) changed = true;
    preprocessor_->graph_->ClearGateMarks(parent);  // Keep the graph clean.
    BLOG(DEBUG5, ret) << "Successful decomposition is in G" << parent->index();
  }
  preprocessor_->ClearConstGates();  // Actual propagation of the constant.
  preprocessor_->ClearNullGates();
  return changed;
}

bool Preprocessor::DecompositionProcessor::ProcessAncestors(
    const IGatePtr& ancestor,
    bool state,
    const std::pair<int, int>& visit_bounds) noexcept {
  if (ancestor->mark()) return false;
  ancestor->mark(true);
  bool changed = false;
  std::vector<std::pair<int, IGatePtr>> to_swap;  // For common gates.
  for (const std::pair<const int, IGatePtr>& arg : ancestor->gate_args()) {
    IGatePtr gate = arg.second;
    if (node_->parents().count(gate->index())) {
      std::unordered_map<int, IGatePtr>* clones =
          state ? &clones_true_ : &clones_false_;
      LOG(DEBUG5) << "Reached decomposition sub-parent G" << gate->index();
      if (clones->count(gate->index())) {
        to_swap.emplace_back(arg.first, clones->find(gate->index())->second);
        changed = true;
        continue;  // Existing clones are not to be traversed further.
      }
      IGatePtr clone = gate->Clone();
      clone->Visit(gate->EnterTime());
      clone->Visit(gate->ExitTime());
      clone->Visit(gate->LastVisit());
      to_swap.emplace_back(arg.first, clone);
      clone->ProcessConstantArg(node_, state);
      changed = true;
      if (clone->IsConstant()) {
        preprocessor_->const_gates_.push_back(clone);
      } else if (clone->type() == kNull) {
        preprocessor_->null_gates_.push_back(clone);
      }
      continue;  // Avoid processing parents.
    }
    if (!IsNodeWithinGraph(gate, visit_bounds.first, visit_bounds.second))
      continue;
    if (gate->descendant() != node_->index()) continue;
    bool ret =
        DecompositionProcessor::ProcessAncestors(gate, state, visit_bounds);
    if (ret) changed = true;
  }
  for (const auto& arg : to_swap) {
    ancestor->EraseArg(arg.first);
    ancestor->AddArg(GetSign(arg.first) * arg.second->index(), arg.second);
  }
  if (!node_->parents().count(ancestor->index()) &&
      std::none_of(ancestor->gate_args().begin(), ancestor->gate_args().end(),
                   [this](const std::pair<const int, IGatePtr>& arg) {
            return arg.second->descendant() == this->node_->index();
      })) {
    ancestor->descendant(0);  // Lose ancestorship if the descendant is gone.
  }
  return changed;
}

void Preprocessor::MarkCoherence() noexcept {
  graph_->ClearGateMarks();
  Preprocessor::MarkCoherence(graph_->root());
  assert(!(graph_->coherent_ && !graph_->root()->coherent()));
  graph_->coherent_ = !graph_->complement() && graph_->root()->coherent();
}

void Preprocessor::MarkCoherence(const IGatePtr& gate) noexcept {
  if (gate->mark()) return;
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
  for (const std::pair<const int, IGatePtr>& arg : gate->gate_args()) {
    Preprocessor::MarkCoherence(arg.second);
    if (coherent && (arg.first < 0 || !arg.second->coherent()))
      coherent = false;  // Must continue with all gates.
  }
  if (coherent) {
    for (const std::pair<const int, VariablePtr>& arg : gate->variable_args()) {
      if (arg.first < 0) {
        coherent = false;
        break;
      }
    }
  }
  assert(gate->constant_args().empty());
  gate->coherent(coherent);
}

void Preprocessor::ReplaceGate(const IGatePtr& gate,
                               const IGatePtr& replacement) noexcept {
  assert(!gate->parents().empty());
  while (!gate->parents().empty()) {
    IGatePtr parent = gate->parents().begin()->second.lock();
    int sign = parent->GetArgSign(gate);
    parent->EraseArg(sign * gate->index());
    parent->AddArg(sign * replacement->index(), replacement);

    if (parent->IsConstant()) {
      const_gates_.push_back(parent);
    } else if (parent->type() == kNull) {
      null_gates_.push_back(parent);
    }
  }
}

void Preprocessor::AssignOrder() noexcept {
  graph_->ClearNodeOrders();
  Preprocessor::TopologicalOrder(graph_->root(), 0);
}

int Preprocessor::TopologicalOrder(const IGatePtr& root, int order) noexcept {
  if (root->order()) return order;
  for (const std::pair<const int, IGatePtr>& arg : root->gate_args()) {
    order = Preprocessor::TopologicalOrder(arg.second, order);
  }
  for (const std::pair<const int, VariablePtr>& arg : root->variable_args()) {
    if (!arg.second->order()) arg.second->order(++order);
  }
  assert(root->constant_args().empty());
  root->order(++order);
  return order;
}

void Preprocessor::GatherNodes(std::vector<IGatePtr>* gates,
                               std::vector<VariablePtr>* variables) noexcept {
  graph_->ClearNodeVisits();
  Preprocessor::GatherNodes(graph_->root(), gates, variables);
}

void Preprocessor::GatherNodes(
    const IGatePtr& gate,
    std::vector<IGatePtr>* gates,
    std::vector<VariablePtr>* variables) noexcept {
  if (gate->Visited()) return;
  gate->Visit(1);
  gates->push_back(gate);
  for (const auto& arg : gate->gate_args()) {
    Preprocessor::GatherNodes(arg.second, gates, variables);
  }
  for (const auto& arg : gate->variable_args()) {
    if (!arg.second->Visited()) {
      arg.second->Visit(1);
      variables->push_back(arg.second);
    }
  }
}

void CustomPreprocessor<Bdd>::Run() noexcept {
  Preprocessor::Run();
  if (Preprocessor::CheckRootGate()) return;
  Preprocessor::MarkCoherence();
  Preprocessor::AssignOrder();
}

void CustomPreprocessor<Zbdd>::Run() noexcept {
  Preprocessor::Run();
  if (Preprocessor::CheckRootGate()) return;
  if (!Preprocessor::graph().coherent()) {
    CLOCK(time_4);
    LOG(DEBUG2) << "Preprocessing Phase IV...";
    Preprocessor::PhaseFour();
    LOG(DEBUG2) << "Finished Preprocessing Phase IV in " << DUR(time_4);
    if (Preprocessor::CheckRootGate()) return;
  }
  CLOCK(time_5);
  LOG(DEBUG2) << "Preprocessing Phase V...";
  Preprocessor::PhaseFive();
  LOG(DEBUG2) << "Finished Preprocessing Phase V in " << DUR(time_5);
  if (Preprocessor::CheckRootGate()) return;
  Preprocessor::MarkCoherence();
  Preprocessor::AssignOrder();
}

void CustomPreprocessor<Mocus>::Run() noexcept {
  CustomPreprocessor<Zbdd>::Run();
  if (Preprocessor::CheckRootGate()) return;
  CustomPreprocessor<Mocus>::InvertOrder();
}

void CustomPreprocessor<Mocus>::InvertOrder() noexcept {
  std::vector<IGatePtr> gates;
  std::vector<VariablePtr> variables;
  Preprocessor::GatherNodes(&gates, &variables);
  auto middle =
      std::partition(gates.begin(), gates.end(),
                     [](const IGatePtr& gate) { return gate->IsModule(); });

  std::sort(middle, gates.end(), [](const IGatePtr& lhs, const IGatePtr& rhs) {
    assert(lhs->order() != rhs->order());
    return lhs->order() < rhs->order();
  });
  for (auto it = middle; it != gates.end(); ++it)
    (*it)->order(gates.end() - it);  // Inversion.

  int shift = gates.end() - middle;
  for (auto it = gates.begin(); it != middle; ++it)
    (*it)->order(shift + (*it)->order());

  for (auto var : variables) var->order(shift + var->order());
}

}  // namespace scram
