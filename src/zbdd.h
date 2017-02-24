/*
 * Copyright (C) 2015-2017 Olzhas Rakhimov
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

/// @file zbdd.h
/// Zero-Suppressed Binary Decision Diagram facilities.

#ifndef SCRAM_SRC_ZBDD_H_
#define SCRAM_SRC_ZBDD_H_

#include <cstdint>

#include <array>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/noncopyable.hpp>

#include "bdd.h"

namespace scram {
namespace core {

/// Representation of non-terminal nodes in ZBDD.
/// Complement variables are represented with negative indices.
/// The order of the complement is higher than the order of the variable.
class SetNode : public NonTerminal<SetNode> {
 public:
  using NonTerminal::NonTerminal;

  /// @returns true if the ZBDD is minimized.
  bool minimal() const { return minimal_; }

  /// Sets the indication of a minimized ZBDD.
  ///
  /// @param[in] flag  A flag for minimized ZBDD.
  void minimal(bool flag) { minimal_ = flag; }

  /// @returns The registered order of the largest set in the ZBDD.
  int max_set_order() const { return max_set_order_; }

  /// Registers the order of the largest set in the ZBDD
  /// represented by this vertex.
  ///
  /// @param[in] order  The order/size of the largest set.
  void max_set_order(int order) { max_set_order_ = order; }

  /// @returns Whatever count is stored in this node.
  std::int64_t count() const { return count_; }

  /// Stores numerical value for later retrieval.
  /// This can be a helper functionality
  /// for counting the number of sets or nodes.
  ///
  /// @param[in] number  A number with a meaning for the caller.
  ///
  /// @note This field is provided for ZBDD processing techniques
  ///       that use mutually exclusive meta-data about the node
  ///       in different stages of ZBDD processing.
  ///       Usually, the data is not needed after the technique is done.
  ///       In contrast to providing separate fields or using hash tables
  ///       for each technique metric,
  ///       this general-purpose field saves space and time.
  void count(std::int64_t number) { count_ = number; }

 private:
  bool minimal_ = false;  ///< A flag for minimized collection of sets.
  int max_set_order_ = 0;  ///< The order of the largest set in the ZBDD.
  std::int64_t count_ = 0;  ///< The number of products, nodes, or anything.
};

using SetNodePtr = IntrusivePtr<SetNode>;  ///< Shared ZBDD set nodes.

/// Function for hashing a pair of ordered numbers.
struct PairHash {
  /// Operator overload for hashing two ordered numbers.
  ///
  /// @param[in] p  The pair of numbers.
  ///
  /// @returns Hash value of the pair.
  std::size_t operator()(const std::pair<int, int>& p) const noexcept {
    return boost::hash_value(p);
  }
};

/// Hash table with pairs of numbers as keys.
///
/// @tparam Value  Type of values to be stored in the table.
template <typename Value>
using PairTable = std::unordered_map<std::pair<int, int>, Value, PairHash>;

using Triplet = std::array<int, 3>;  ///< Triplet of numbers for functions.

/// Functor for hashing triplets of ordered numbers.
struct TripletHash {
  /// Operator overload for hashing three ordered numbers.
  ///
  /// @param[in] triplet  Three numbers.
  ///
  /// @returns Hash value of the triplet.
  std::size_t operator()(const Triplet& triplet) const noexcept {
    return boost::hash_range(triplet.begin(), triplet.end());
  }
};

/// Hash table with triplets of numbers as keys.
///
/// @tparam Value  Type of values to be stored in the table.
template <typename Value>
using TripletTable = std::unordered_map<Triplet, Value, TripletHash>;

/// Zero-Suppressed Binary Decision Diagrams for set manipulations.
class Zbdd : private boost::noncopyable {
 public:
  using VertexPtr = IntrusivePtr<Vertex<SetNode>>;  ///< ZBDD vertex base.
  using TerminalPtr = IntrusivePtr<Terminal<SetNode>>;  ///< Terminal vertex.

  /// Iterator over products in a ZBDD container.
  /// The implementation is complicated with the incorporation of modules.
  /// A single stack is used by all consecutive and recursive modules.
  ///
  /// @pre No constant ZBDD modules resulting in the Base set.
  class const_iterator
      : public boost::iterator_facade<const_iterator, const std::vector<int>,
                                      boost::forward_traversal_tag> {
    friend class boost::iterator_core_access;

    /// Iterator over sets in the module ZBDD represented by a proxy node.
    /// Modules within a module (i.e., sub-modules) are recursive,
    /// while consecutive modules are not.
    class module_iterator {
     public:
      /// Constructs module iterator based on the host ZBDD iterator.
      ///
      /// @param[in] node  The proxy node for module.
      ///                  nullptr for the root ZBDD itself.
      /// @param[in] zbdd  The module ZBDD.
      /// @param[in,out] it  The host iterator with product stacks for output.
      /// @param[in] sentinel  The flag for end iterators.
      module_iterator(const SetNode* node, const Zbdd& zbdd, const_iterator* it,
                      bool sentinel = false)
          : sentinel_(sentinel),
            start_pos_(it->product_.size()),
            end_pos_(start_pos_),
            it_(*it),
            node_(node),
            zbdd_(zbdd) {
        if (!sentinel_) {
          sentinel_ = !GenerateProduct(zbdd_.root());
          end_pos_ = it_.product_.size();
        }
      }

      /// Only single iterator per module is allowed.
      module_iterator(module_iterator&&) noexcept = default;

      /// @returns true if a new product has been generated for the host set.
      explicit operator bool() const { return !sentinel_; }

      /// Generates the next set.
      void operator++() {
        if (sentinel_)
          return;
        assert(end_pos_ >= start_pos_ && "Corrupted sentinel.");
        while (start_pos_ != it_.product_.size()) {
          if (!module_stack_.empty() &&
              it_.product_.size() == module_stack_.back().end_pos_) {
            const SetNode* node = module_stack_.back().node_;
            for (++module_stack_.back(); module_stack_.back();
                 ++module_stack_.back()) {
              if (GenerateProduct(node->high()))
                goto outer_break;
            }
            module_stack_.pop_back();
            if (GenerateProduct(node->low()))
              break;

          } else if (GenerateProduct(Pop()->low())) {
            break;
          }
        }
      outer_break:
        end_pos_ = it_.product_.size();
        sentinel_ = start_pos_ == end_pos_;
      }

     private:
      /// Generates a next product in the ZBDD traversal.
      ///
      /// @param[in] vertex  The vertex to start adding into the product.
      ///
      /// @returns true if a new product has been generated.
      ///
      /// @post If the new product is generated,
      ///       the product and stack containers are updated accordingly.
      bool GenerateProduct(const VertexPtr& vertex) noexcept {
        if (vertex->terminal())
          return Terminal<SetNode>::Ref(vertex).value();
        if (it_.product_.size() >= it_.zbdd_.settings().limit_order())
          return false;
        const SetNode& node = SetNode::Ref(vertex);
        if (node.module()) {
          module_stack_.emplace_back(
              &node, *zbdd_.modules_.find(node.index())->second, &it_);
          for (; module_stack_.back(); ++module_stack_.back()) {
            if (GenerateProduct(node.high()))
              return true;
          }
          assert(it_.product_.size() == module_stack_.back().start_pos_);
          module_stack_.pop_back();
          return GenerateProduct(node.low());

        } else {
          Push(&node);
          return GenerateProduct(node.high()) || GenerateProduct(Pop()->low());
        }
      }

      /// Removes the current leaf node from the product.
      ///
      /// @returns The current leaf node in the product.
      const SetNode* Pop() noexcept {
        assert(start_pos_ < it_.product_.size() && "Access beyond the range!");
        const SetNode* leaf = it_.node_stack_.back();
        it_.node_stack_.pop_back();
        it_.product_.pop_back();
        return leaf;
      }

      /// Updates the current product with a literal.
      ///
      /// @param[in] set_node  The current leaf set node to add to the product.
      void Push(const SetNode* set_node) noexcept {
        it_.node_stack_.push_back(set_node);
        it_.product_.push_back(set_node->index());
      }

      bool sentinel_;  ///< The signal to end the iteration.
      const int start_pos_;  ///< The initial position on the start.
      int end_pos_;  ///< The current end position in the product.
      const_iterator& it_;  ///< The host iterator.
      const SetNode* node_;  ///< The proxy node representing the module.
      const Zbdd& zbdd_;  ///< The module ZBDD.
      /// The stack for consecutive modules' iterators.
      std::vector<module_iterator> module_stack_;
    };

   public:
    /// @param[in] zbdd  The container to iterate over.
    /// @param[in] sentinel  The flag to turn the iterator into an end sentinel.
    ///
    /// @pre The ZBDD container is not modified during the iteration.
    explicit const_iterator(const Zbdd& zbdd, bool sentinel = false)
        : sentinel_(sentinel), zbdd_(zbdd), it_(nullptr, zbdd, this, sentinel) {
      sentinel_ = !it_;
    }

    /// The copy constructor is only for begin and end iterators.
    ///
    /// @param[in] other  Begin or End iterator.
    const_iterator(const const_iterator& other) noexcept
        : sentinel_(other.sentinel_),
          zbdd_(other.zbdd_),
          it_(nullptr, zbdd_, this, sentinel_) {
      assert(*this == other && "Copy ctor is only for begin/end iterators.");
    }

   private:
    /// Standard forward iterator functionality returning products.
    /// @{
    void increment() {
      assert(!sentinel_ && "Incrementing an end iterator.");
      ++it_;
      sentinel_ = !it_;
    }
    bool equal(const const_iterator& other) const {
      assert(!(sentinel_ && !product_.empty()) && "Uncleared products.");
      return sentinel_ == other.sentinel_ && &zbdd_ == &other.zbdd_ &&
             product_ == other.product_;
    }
    const std::vector<int>& dereference() const {
      assert(!sentinel_ && "Dereferencing end iterator.");
      return product_;
    }
    /// @}

    bool sentinel_;  ///< The marker for the end of traversal.
    const Zbdd& zbdd_;  ///< The source container for the products.
    std::vector<int> product_;  ///< The current product.
    std::vector<const SetNode*> node_stack_;  ///< The traversal stack.
    module_iterator it_;  ///< The root module iterator for the whole ZBDD.
  };

  /// Converts Reduced Ordered BDD
  /// into Zero-Suppressed BDD.
  ///
  /// @param[in] bdd  ROBDD with the ITE vertices.
  /// @param[in] settings  Settings for analysis.
  ///
  /// @pre BDD has attributed edges with only one terminal (1/True).
  ///
  /// @post The input BDD structure is not changed.
  ///
  /// @note The input BDD is not passed as a constant
  ///       because ZBDD needs BDD facilities to calculate prime implicants.
  ///       However, ZBDD guarantees to preserve the original BDD structure.
  Zbdd(Bdd* bdd, const Settings& settings) noexcept;

  /// Constructor with the analysis target.
  /// ZBDD is directly produced from a PDAG.
  ///
  /// @param[in] graph  Preprocessed and fully normalized PDAG.
  /// @param[in] settings  The analysis settings.
  ///
  /// @pre The passed PDAG already has variable ordering.
  /// @note The construction may take considerable time.
  Zbdd(const Pdag* graph, const Settings& settings) noexcept;

  virtual ~Zbdd() noexcept = default;

  /// Runs the analysis
  /// with the representation of a PDAG as ZBDD.
  void Analyze() noexcept;

  /// @returns Products generated by the analysis.
  const Zbdd& products() const { return *this; }

  /// @returns Iterators over sets in the ZBDD.
  /// @{
  auto begin() const { return const_iterator(*this); }
  auto end() const { return const_iterator(*this, /*sentinel=*/true); }
  /// @}

  /// @returns The number of *products* in the ZBDD.
  ///
  /// @note This is not cheap.
  ///       The complexity is O(N) on the number of *sets* in ZBDD.
  std::size_t size() const { return std::distance(begin(), end()); }

  /// @returns true for ZBDD with no products.
  bool empty() const { return begin() == end(); }

  /// @returns true if the ZBDD represents a base/unity set.
  bool base() const { return root_ == kBase_; }

 protected:
  /// The common constructor to initialize member variables.
  ///
  /// @param[in] settings  Settings that control analysis complexity.
  /// @param[in] coherent  A flag for coherent modular functions.
  /// @param[in] module_index  The index of a module if known.
  explicit Zbdd(const Settings& settings, bool coherent = false,
                int module_index = 0) noexcept;

  /// @returns Current root vertex of the ZBDD.
  const VertexPtr& root() const { return root_; }

  /// Sets a new root vertex for ZBDD.
  ///
  /// @param[in] vertex  A vertex already registered in this ZBDD.
  void root(const VertexPtr& vertex) { root_ = vertex; }

  /// @returns Analysis setting with this ZBDD.
  const Settings& settings() const { return kSettings_; }

  /// @returns A set of registered and fully processed modules;
  const std::map<int, std::unique_ptr<Zbdd>>& modules() const {
    return modules_;
  }

  /// Logs properties of the Zbdd.
  void Log() noexcept;

  /// Finds or adds a unique SetNode in the ZBDD.
  /// All vertices in the ZBDD must be created with this function.
  /// Otherwise, the ZBDD may not be reduced,
  /// and vertices will miss crucial meta-information about the ZBDD.
  ///
  /// @param[in] index  Positive or negative index of the node.
  /// @param[in] high  The high vertex.
  /// @param[in] low  The low vertex.
  /// @param[in] order The order for the vertex variable.
  /// @param[in] module  The indication of a proxy for a modular ZBDD.
  /// @param[in] coherent  The indication of a coherent proxy.
  ///
  /// @returns Set node with the given parameters.
  ///
  /// @warning This function is not aware of reduction rules.
  SetNodePtr FindOrAddVertex(int index, const VertexPtr& high,
                             const VertexPtr& low, int order,
                             bool module = false,
                             bool coherent = false) noexcept;

  /// Find or adds a ZBDD SetNode vertex using information from gates.
  ///
  /// @param[in] gate  Gate with index, order, and other information.
  /// @param[in] high  The new high vertex.
  /// @param[in] low  The new low vertex.
  ///
  /// @returns SetNode for a replacement.
  ///
  /// @warning This function is not aware of reduction rules.
  SetNodePtr FindOrAddVertex(const Gate& gate, const VertexPtr& high,
                             const VertexPtr& low) noexcept;

  /// Applies Boolean operation to two vertices representing sets.
  /// This is the main function for the operation.
  ///
  /// @tparam Type  The operator enum.
  ///
  /// @param[in] arg_one  First argument ZBDD set.
  /// @param[in] arg_two  Second argument ZBDD set.
  /// @param[in] limit_order  The limit on the order for the computations.
  ///
  /// @returns The resulting ZBDD vertex.
  ///
  /// @post The limit on the set order is guaranteed.
  template <Operator Type>
  VertexPtr Apply(const VertexPtr& arg_one, const VertexPtr& arg_two,
                  int limit_order) noexcept;

  /// Applies Boolean operation to two vertices representing sets.
  /// This is a convenience function
  /// if the operator type cannot be determined at compile time.
  ///
  /// @param[in] type  The operator or type of the gate.
  /// @param[in] arg_one  First argument ZBDD set.
  /// @param[in] arg_two  Second argument ZBDD set.
  /// @param[in] limit_order  The limit on the order for the computations.
  ///
  /// @returns The resulting ZBDD vertex.
  ///
  /// @pre The operator is either AND or OR.
  ///
  /// @post The limit on the set order is guaranteed.
  VertexPtr Apply(Operator type, const VertexPtr& arg_one,
                  const VertexPtr& arg_two, int limit_order) noexcept;

  /// Applies Boolean operation to ZBDD graph non-terminal vertices.
  ///
  /// @tparam Type  The operator enum.
  ///
  /// @param[in] arg_one  First argument set vertex.
  /// @param[in] arg_two  Second argument set vertex.
  /// @param[in] limit_order  The limit on the order for the computations.
  ///
  /// @returns The resulting ZBDD vertex.
  ///
  /// @pre Argument vertices are ordered.
  template <Operator Type>
  VertexPtr Apply(const SetNodePtr& arg_one, const SetNodePtr& arg_two,
                  int limit_order) noexcept;

  /// Removes complements of variables from products.
  /// This procedure only needs to be performed for non-coherent graphs
  /// with minimal cut sets as output.
  ///
  /// @param[in] vertex  The variable vertex in the ZBDD.
  /// @param[in,out] wide_results  Memoisation of the processed vertices.
  ///
  /// @returns Processed vertex.
  ///
  /// @post Sub-modules are not processed.
  /// @post Complements of modules are not eliminated.
  VertexPtr EliminateComplements(
      const VertexPtr& vertex,
      std::unordered_map<int, VertexPtr>* wide_results) noexcept;

  /// Removes constant modules from products.
  /// Constant modules are likely to happen after complement elimination.
  /// This procedure is inherently bottom-up,
  /// so the caller must make sure
  /// that the modules have already been pre-processed.
  ///
  /// @pre All modules have been processed.
  void EliminateConstantModules() noexcept;

  /// Removes subsets in ZBDD.
  ///
  /// @param[in] vertex  The variable node in the set.
  ///
  /// @returns Processed vertex.
  VertexPtr Minimize(const VertexPtr& vertex) noexcept;

  /// Traverses ZBDD to find modules and adjusted cut-offs.
  /// Modules within modules are not gathered.
  ///
  /// @param[in] vertex  The root vertex to start with.
  /// @param[in] current_order  The product order from the top to the module.
  /// @param[in,out] modules  A map of module indices, coherence, and cut-offs.
  ///
  /// @returns The minimum product order from the bottom.
  /// @returns -1 if the vertex is terminal Empty on low branch only.
  ///
  /// @pre The ZBDD is minimal.
  int GatherModules(const VertexPtr& vertex,
                    int current_order,
                    std::map<int, std::pair<bool, int>>* modules) noexcept;

  /// Clears all memoization tables.
  void ClearTables() noexcept {
    and_table_.clear();
    or_table_.clear();
    minimal_results_.clear();
    subsume_table_.clear();
    prune_results_.clear();
  }

  /// Freezes the graph.
  /// Releases all possible memory from memoization and unique tables.
  ///
  /// @pre No more graph modifications after the freeze.
  void Freeze() noexcept {
    unique_table_.Release();
    Zbdd::ClearTables();
    and_table_.reserve(0);
    or_table_.reserve(0);
    minimal_results_.reserve(0);
    subsume_table_.reserve(0);
  }

  /// Joins a ZBDD representing a module gate.
  ///
  /// @param[in] index  The index of the module.
  /// @param[in] container  The container of the module graph.
  ///
  /// @pre The module products are final,
  ///      and no more processing or sanitizing is needed.
  void JoinModule(int index, std::unique_ptr<Zbdd> container) noexcept {
    assert(!modules_.count(index));
    assert(container->root()->terminal() ||
           SetNode::Ref(container->root()).minimal());
    modules_.emplace(index, std::move(container));
  }

  /// @todo Redesign vertex management and creation.
  ///       The management mechanism must be encapsulated.
  ///       Invariants must be private.
  const TerminalPtr kBase_;  ///< Terminal Base (Unity/1) set.
  const TerminalPtr kEmpty_;  ///< Terminal Empty (Null/0) set.

 private:
  using SetNodeWeakPtr = WeakIntrusivePtr<SetNode>;  ///< Pointer for tables.
  using ComputeTable = TripletTable<VertexPtr>;  ///< General computation table.
  /// Module entry in the tables with its original gate index.
  using ModuleEntry = std::pair<const int, std::unique_ptr<Zbdd>>;

  /// Converts a modular BDD function
  /// into Zero-Suppressed BDD.
  ///
  /// @param[in] module  Modular BDD function.
  /// @param[in] coherent  A flag for coherent modular functions.
  /// @param[in] bdd  ROBDD with the ITE vertices.
  /// @param[in] settings  Settings for analysis.
  /// @param[in] module_index  The of a module if known.
  ///
  /// @pre BDD has attributed edges with only one terminal (1/True).
  ///
  /// @post The input BDD structure is not changed.
  ///
  /// @note The input BDD is not passed as a constant
  ///       because ZBDD needs BDD facilities to calculate prime implicants.
  ///       However, ZBDD guarantees to preserve the original BDD structure.
  Zbdd(const Bdd::Function& module, bool coherent, Bdd* bdd,
       const Settings& settings, int module_index = 0) noexcept;

  /// Constructs ZBDD from modular PDAGs.
  /// This constructor does not handle constant or single variable graphs.
  /// These cases are expected to be handled
  /// after calling this constructor.
  /// In these special cases,
  /// this constructor guarantees
  /// all initialization except for the root.
  ///
  /// @param[in] gate  The root gate of a module.
  /// @param[in] settings  Analysis settings.
  ///
  /// @post The root vertex pointer is uninitialized
  ///       if the PDAG is constant or single variable.
  Zbdd(const Gate& gate, const Settings& settings) noexcept;

  /// Finds a replacement for an existing node
  /// or adds a new node based on an existing node.
  ///
  /// @param[in] node  An existing node.
  /// @param[in] high  The new high vertex.
  /// @param[in] low  The new low vertex.
  ///
  /// @returns Set node for a replacement.
  ///
  /// @warning This function is not aware of reduction rules.
  SetNodePtr FindOrAddVertex(const SetNodePtr& node, const VertexPtr& high,
                             const VertexPtr& low) noexcept;

  /// Adds a new or finds an existing reduced ZBDD vertex
  /// with parameters of a prototype BDD ITE vertex.
  ///
  /// @param[in] ite  The prototype BDD ITE vertex.
  /// @param[in] complement  Vertex represents complement of a variable.
  /// @param[in] high  The high ZBDD vertex.
  /// @param[in] low  The low ZBDD vertex.
  ///
  /// @returns Resultant reduced vertex.
  VertexPtr GetReducedVertex(const ItePtr& ite, bool complement,
                             const VertexPtr& high,
                             const VertexPtr& low) noexcept;

  /// Finds a replacement reduced vertex for an existing node,
  /// or adds a new node based on an existing node.
  ///
  /// @param[in] node  An existing node.
  /// @param[in] high  The new high vertex.
  /// @param[in] low  The new low vertex.
  ///
  /// @returns Set node for a replacement.
  VertexPtr GetReducedVertex(const SetNodePtr& node,
                             const VertexPtr& high,
                             const VertexPtr& low) noexcept;

  /// Computes the key for computation results.
  /// The key is used in computation memoisation tables.
  ///
  /// @param[in] arg_one  First argument.
  /// @param[in] arg_two  Second argument.
  /// @param[in] limit_order  The limit on the order for the computations.
  ///
  /// @returns A triplet of integers for the computation key.
  ///
  /// @pre The arguments are not the same functions.
  ///      Equal ID functions are handled by the reduction.
  /// @pre Even though the arguments are not SetNodePtr type,
  ///      they are ZBDD SetNode vertices.
  Triplet GetResultKey(const VertexPtr& arg_one, const VertexPtr& arg_two,
                       int limit_order) noexcept;

  /// Converts BDD graph into ZBDD graph.
  ///
  /// @param[in] vertex  Vertex of the ROBDD graph.
  /// @param[in] complement  Interpretation of the vertex as complement.
  /// @param[in] bdd_graph  The main ROBDD as helper database.
  /// @param[in] limit_order  The maximum size of requested sets.
  /// @param[in,out] ites  Processed function graphs with ids and limit order.
  ///
  /// @returns Pointer to the root vertex of the ZBDD graph.
  ///
  /// @post The input BDD structure is not changed.
  VertexPtr ConvertBdd(const Bdd::VertexPtr& vertex, bool complement,
                       Bdd* bdd_graph, int limit_order,
                       PairTable<VertexPtr>* ites) noexcept;

  /// Converts BDD if-then-else vertex into ZBDD graph.
  /// This overload differs in that
  /// it does not register the results.
  /// It is used by the BDD vertex to ZBDD converter,
  /// and this function should not be called directly.
  ///
  /// @param[in] ite  ITE vertex of the ROBDD graph.
  /// @param[in] complement  Interpretation of the vertex as complement.
  /// @param[in] bdd_graph  The main ROBDD as helper database.
  /// @param[in] limit_order  The maximum size of requested sets.
  /// @param[in,out] ites  Processed function graphs with ids and limit order.
  ///
  /// @returns Pointer to the root vertex of the ZBDD graph.
  VertexPtr ConvertBdd(const ItePtr& ite, bool complement,
                       Bdd* bdd_graph, int limit_order,
                       PairTable<VertexPtr>* ites) noexcept;

  /// Converts BDD if-then-else vertex into ZBDD graph for prime implicants.
  /// This is used by the BDD vertex to ZBDD converter,
  /// and this function should not be called directly.
  ///
  /// @param[in] ite  ITE vertex of the ROBDD graph.
  /// @param[in] complement  Interpretation of the vertex as complement.
  /// @param[in] bdd_graph  The main ROBDD as helper database.
  /// @param[in] limit_order  The maximum size of requested sets.
  /// @param[in,out] ites  Processed function graphs with ids and limit order.
  ///
  /// @returns Pointer to the root vertex of the ZBDD graph.
  VertexPtr ConvertBddPrimeImplicants(const ItePtr& ite, bool complement,
                                      Bdd* bdd_graph, int limit_order,
                                      PairTable<VertexPtr>* ites) noexcept;

  /// Transforms a PDAG gate into a Zbdd set graph.
  ///
  /// @param[in] gate  The root gate of the PDAG.
  /// @param[in,out] gates  Processed gates with use counts.
  /// @param[out] module_gates  Sub-module gates.
  ///
  /// @returns The top vertex of the Zbdd graph.
  ///
  /// @pre The memoization container is not used outside of this function.
  ///
  /// @post Sub-module gates are not processed.
  VertexPtr ConvertGraph(
      const Gate& gate,
      std::unordered_map<int, std::pair<VertexPtr, int>>* gates,
      std::unordered_map<int, const Gate*>* module_gates) noexcept;

  /// Processes complements in a SetNode with processed high/low edges.
  ///
  /// @param[in] node  SetNode to be processed.
  /// @param[in] high  Processed high edge.
  /// @param[in] low  Processed low edge.
  ///
  /// @returns Processed ZBDD vertex without complements.
  ///
  /// @post Sub-modules are not processed.
  /// @post Complements of modules are not eliminated.
  VertexPtr EliminateComplement(const SetNodePtr& node, const VertexPtr& high,
                                const VertexPtr& low) noexcept;

  /// Removes constant modules from products.
  ///
  /// @param[in] vertex  The variable vertex in the ZBDD.
  /// @param[in,out] results  Memoisation of the processed vertices.
  ///
  /// @returns Processed vertex.
  ///
  /// @pre All sub-modules are already processed.
  VertexPtr EliminateConstantModules(
      const VertexPtr& vertex,
      std::unordered_map<int, VertexPtr>* results) noexcept;

  /// Processes constant modules in a SetNode with processed high/low edges.
  ///
  /// @param[in] node  SetNode to be processed.
  /// @param[in] high  Processed high edge.
  /// @param[in] low  Processed low edge.
  ///
  /// @returns Processed ZBDD vertex without constant modules.
  ///
  /// @pre All sub-modules are already processed.
  VertexPtr EliminateConstantModule(const SetNodePtr& node,
                                    const VertexPtr& high,
                                    const VertexPtr& low) noexcept;

  /// Applies subsume operation on two sets.
  /// Subsume operation removes
  /// paths that exist in Low branch from High branch.
  ///
  /// @param[in] high  True/then/high branch of a variable.
  /// @param[in] low  False/else/low branch of a variable.
  ///
  /// @returns Minimized high branch for a variable.
  VertexPtr Subsume(const VertexPtr& high, const VertexPtr& low) noexcept;

  /// Prunes the ZBDD graph with the cut-off.
  ///
  /// @param[in] vertex  The root vertex of the ZBDD.
  /// @param[in] limit_order  The cut-off order for the sets.
  ///
  /// @returns The root vertex of the pruned ZBDD.
  ///
  /// @post If the ZBDD is minimal,
  ///       the resultant pruned ZBDD is minimal.
  VertexPtr Prune(const VertexPtr& vertex, int limit_order) noexcept;

  /// Checks if a set node represents a gate.
  /// Apply operations and truncation operations
  /// should avoid accounting non-module gates
  /// if they exist in ZBDD.
  ///
  /// @param[in] node  A node to be tested.
  ///
  /// @returns true for modules by default.
  virtual bool IsGate(const SetNode& node) noexcept {
    return node.module();
  }

  /// Checks if a node have a possibility to represent Unity.
  ///
  /// @param[in] node  SetNode to test for possibility of Unity.
  ///
  /// @returns false if the passed node can never be Unity.
  bool MayBeUnity(const SetNode& node) noexcept;

  /// Counts the number of SetNodes
  /// excluding the nodes in the modules.
  ///
  /// @param[in] vertex  The root vertex to start counting.
  ///
  /// @returns The total number of SetNode vertices
  ///          including vertices in modules.
  ///
  /// @pre SetNode marks are clear (false).
  int CountSetNodes(const VertexPtr& vertex) noexcept;

  /// Counts the total number of sets in ZBDD.
  ///
  /// @param[in] vertex  The root vertex of ZBDD.
  /// @param[in] modules  Unroll sets with modules.
  ///
  /// @returns The number of products in ZBDD.
  ///
  /// @pre SetNode marks are clear (false).
  std::int64_t CountProducts(const VertexPtr& vertex, bool modules) noexcept;

  /// Cleans up non-terminal vertex marks
  /// by setting them to "false".
  ///
  /// @param[in] vertex  The root vertex of the graph.
  /// @param[in] modules  Clear marks in modules as well.
  ///
  /// @pre The graph is marked "true" contiguously.
  void ClearMarks(const VertexPtr& vertex, bool modules) noexcept;

  /// Cleans up non-terminal vertex count fields
  /// by setting them to 0.
  ///
  /// @param[in] vertex  The root vertex of the graph.
  /// @param[in] modules  Clear counts in modules as well.
  ///
  /// @pre The graph node marks are clear.
  ///
  /// @post Traversed marks are set to 'true'.
  void ClearCounts(const VertexPtr& vertex, bool modules) noexcept;

  /// Checks ZBDD graphs for errors in the structure.
  /// Errors are assertions that fail at runtime.
  ///
  /// @param[in] vertex  The root vertex of ZBDD.
  /// @param[in] modules  Test modules as well.
  ///
  /// @pre SetNode marks are clear (false).
  void TestStructure(const VertexPtr& vertex, bool modules) noexcept;

  const Settings kSettings_;  ///< Analysis settings.
  VertexPtr root_;  ///< The root vertex of ZBDD.
  bool coherent_;  ///< Inherited coherence from BDD.
  int module_index_;  ///< Identifier for a module if any.

  /// Table of unique SetNodes denoting sets.
  /// The key consists of (index, id_high, id_low) triplet.
  UniqueTable<SetNode> unique_table_;

  /// Table of processed computations over sets.
  /// The argument sets are recorded with their IDs (not vertex indices).
  /// In order to keep only unique computations,
  /// the argument IDs must be ordered.
  /// The key is {min_id, max_id, max_order}.
  /// @{
  ComputeTable and_table_;
  ComputeTable or_table_;
  /// @}

  /// Memoization of minimal ZBDD vertices.
  std::unordered_map<int, VertexPtr> minimal_results_;
  /// The results of subsume operations over sets.
  PairTable<VertexPtr> subsume_table_;
  /// The results of pruning operations.
  PairTable<VertexPtr> prune_results_;

  std::map<int, std::unique_ptr<Zbdd>> modules_;  ///< Module graphs.
  int set_id_;  ///< Identification assignment for new set graphs.
};

namespace zbdd {

/// Storage for generated cut sets in MOCUS.
/// The semantics is similar to a set of cut sets.
/// The container assumes special variable ordering.
/// Gates to be processed must be ordered to the top.
class CutSetContainer : public Zbdd {
 public:
  /// Default constructor to initialize member variables.
  ///
  /// @param[in] settings  Settings that control analysis complexity.
  /// @param[in] module_index  The of a module if known.
  /// @param[in] gate_index_bound  The exclusive lower bound for gate indices.
  ///
  /// @pre No complements of gates.
  /// @pre Gates are indexed sequentially
  ///      starting from a number larger than the lower bound.
  /// @pre Basic events are indexed sequentially
  ///      up to a number less than or equal to the given lower bound.
  CutSetContainer(const Settings& settings, int module_index,
                  int gate_index_bound) noexcept;

  /// Converts a PDAG gate into intermediate cut sets.
  ///
  /// @param[in] gate  The target AND/OR gate with arguments.
  ///
  /// @returns The root vertex of the ZBDD representing the gate cut sets.
  VertexPtr ConvertGate(const Gate& gate) noexcept;

  /// Finds a gate in intermediate cut sets.
  ///
  /// @returns The index of the gate in intermediate cut sets.
  /// @returns 0 if no gates are found.
  ///
  /// @pre Variable ordering puts the gate to the top (root).
  int GetNextGate() noexcept {
    if (Zbdd::root()->terminal())
      return 0;
    SetNode& node = SetNode::Ref(Zbdd::root());
    return CutSetContainer::IsGate(node) && !node.module() ? node.index() : 0;
  }

  /// Extracts (removes!) intermediate cut sets
  /// containing a node with a given index.
  ///
  /// @param[in] index  The index of the gate.
  ///
  /// @returns The root of the ZBDD containing the intermediate cut sets.
  ///
  /// @pre Variable ordering puts the gate to the top (root).
  ///
  /// @post The extracted cut sets are pre-processed
  ///       by removing the vertex with the index of the gate.
  VertexPtr ExtractIntermediateCutSets(int index) noexcept;

  /// Expands the intermediate ZBDD representation of a gate
  /// in intermediate cut sets containing the gate.
  ///
  /// @param[in] gate_zbdd  The intermediate ZBDD of the gate.
  /// @param[in] cut_sets  A collection of cut sets.
  ///
  /// @returns The root vertex of the resulting ZBDD.
  ///
  /// @pre The intermediate cut sets are pre-processed
  ///      by removing the vertex with the index of the gate.
  VertexPtr ExpandGate(const VertexPtr& gate_zbdd,
                       const VertexPtr& cut_sets) noexcept;

  /// Merges a set of cut sets into the main container.
  ///
  /// @param[in] vertex  The root ZBDD vertex representing the cut sets.
  ///
  /// @pre The argument ZBDD cut sets are managed by this container.
  void Merge(const VertexPtr& vertex) noexcept;

  /// Eliminates all complements from cut sets.
  /// This can only be done
  /// if the cut set generation is certain not to have conflicts.
  ///
  /// @pre The cut sets have negative literals, i.e., non-coherent.
  ///
  /// @post Sub-modules are not processed.
  void EliminateComplements() noexcept {
    std::unordered_map<int, VertexPtr> wide_results;
    Zbdd::root(Zbdd::EliminateComplements(Zbdd::root(), &wide_results));
  }

  /// Removes constant modules from cut sets.
  /// Constant modules are likely to happen after complement elimination.
  ///
  /// @pre All modules have been processed.
  void EliminateConstantModules() noexcept { Zbdd::EliminateConstantModules(); }

  /// Minimizes cut sets in the container.
  ///
  /// @post Sub-modules are not processed.
  void Minimize() noexcept { Zbdd::root(Zbdd::Minimize(Zbdd::root())); }

  /// Gathers all module indices in the cut sets.
  ///
  /// @returns An unordered map module of indices, coherence, and cut-offs.
  std::map<int, std::pair<bool, int>> GatherModules() noexcept {
    assert(Zbdd::modules().empty() && "Unexpected call with defined modules?!");
    std::map<int, std::pair<bool, int>> modules;
    Zbdd::GatherModules(Zbdd::root(), 0, &modules);
    return modules;
  }

  using Zbdd::JoinModule;  ///< Joins fully processed modules.
  using Zbdd::Log;  ///< Logs properties of the container.

 private:
  /// Checks if a set node represents a gate.
  ///
  /// @param[in] node  A node to be tested.
  ///
  /// @returns true if the index of the node belongs to a gate.
  ///
  /// @pre There are no complements of gates.
  /// @pre Gate indices have a lower bound.
  bool IsGate(const SetNode& node) noexcept override {
    return node.index() > gate_index_bound_;
  }

  int gate_index_bound_;  ///< The exclusive lower bound for the gate indices.
};

}  // namespace zbdd

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_ZBDD_H_
