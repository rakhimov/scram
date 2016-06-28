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

/// @file bdd.h
/// Fault tree analysis with the Binary Decision Diagram algorithms.

#ifndef SCRAM_SRC_BDD_H_
#define SCRAM_SRC_BDD_H_

#include <cmath>

#include <algorithm>
#include <forward_list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include "boolean_graph.h"
#include "settings.h"

namespace scram {
namespace core {

/// Control flags and pointers for communication
/// between BDD vertex pointers and BDD tables.
struct ControlBlock {
  void* vertex;  ///< The manager of the control block.
  int weak_count;  ///< Pointers in tables.
};

/// The default management of BDD vertices.
///
/// @tparam T  The type of the main functional BDD vertex.
template <class T>
using IntrusivePtr = boost::intrusive_ptr<T>;

/// A weak pointer to store in BDD unique tables.
/// This weak pointer is unique pointer as well
/// because vertices should not be easily shared among multiple BDDs.
///
/// @tparam T  The type of the main functional BDD vertex.
template <class T>
class WeakIntrusivePtr final {
 public:
  /// Default constructor is to allow initialization in tables.
  WeakIntrusivePtr() noexcept : control_block_(nullptr) {}

  WeakIntrusivePtr(const WeakIntrusivePtr&) = delete;
  WeakIntrusivePtr& operator=(const WeakIntrusivePtr&) = delete;

  /// Constructs from the shared pointer.
  /// However, there is no weak-to-shared constructor.
  ///
  /// @param[in] ptr  Fully initialized intrusive pointer.
  explicit WeakIntrusivePtr(const IntrusivePtr<T>& ptr) noexcept
      : control_block_(get_control_block(ptr.get())) {
    control_block_->weak_count++;
  }

  /// Copy assignment from shared pointers
  /// for convenient initialization with operator[] in hash tables.
  ///
  /// @param[in] ptr  Fully initialized intrusive pointer.
  ///
  /// @returns Reference to this.
  WeakIntrusivePtr& operator=(const IntrusivePtr<T>& ptr) noexcept {
    this->~WeakIntrusivePtr();
    new(this) WeakIntrusivePtr(ptr);
    return *this;
  }

  /// Decrements weak count in the control block.
  /// If this is the last pointer and vertex is gone,
  /// the control block is deleted.
  ~WeakIntrusivePtr() noexcept {
    if (control_block_) {
      if (--control_block_->weak_count == 0 &&
          control_block_->vertex == nullptr)
        delete control_block_;
    }
  }

  /// @returns true if the managed vertex is deleted or not initialized.
  bool expired() const { return !control_block_ || !control_block_->vertex; }

  /// @returns The intrusive pointer of the vertex.
  ///
  /// @warning Hard failure for uninitialized pointers.
  IntrusivePtr<T> lock() const {
    return IntrusivePtr<T>(static_cast<T*>(control_block_->vertex));
  }

  /// @returns The raw pointer to the vertex.
  ///
  /// @warning Hard failure for uninitialized pointers.
  T* get() const { return static_cast<T*>(control_block_->vertex); }

 private:
  ControlBlock* control_block_;  ///< To receive information from vertices.
};

template <class T>
class Terminal;  // Forward declaration for Vertex to manage.

/// Representation of a vertex in BDD graphs.
/// This is a base class for all BDD vertices;
/// however, it is NOT polymorphic for performance reasons.
///
/// @tparam T  The type of the main functional BDD vertex.
///
/// @pre Vertices are managed by reference counted pointers
///      provided by this class' interface.
/// @pre Vertices are not shared among separate BDD instances.
template <class T>
class Vertex {
  /// @param[in] ptr  Vertex pointer managed by intrusive pointers.
  ///
  /// @returns The control block of intrusive counting for tables.
  friend ControlBlock* get_control_block(Vertex<T>* ptr) noexcept {
    return ptr->control_block_;
  }

  /// Increases the reference count for new intrusive pointers.
  ///
  /// @param[in] ptr  Vertex pointer managed by intrusive pointers.
  friend void intrusive_ptr_add_ref(Vertex<T>* ptr) noexcept {
    ptr->use_count_++;
  }

  /// Decrements the reference count for removed intrusive pointers.
  /// If no more intrusive pointers left,
  /// the object is deleted.
  ///
  /// @param[in] ptr  Vertex pointer managed by intrusive pointers.
  friend void intrusive_ptr_release(Vertex<T>* ptr) noexcept {
    assert(ptr->use_count_ > 0 && "Missing reference counts.");
    if (--ptr->use_count_ == 0) {
      if (!ptr->terminal()) {  // Likely.
        delete static_cast<T*>(ptr);
      } else {
        delete static_cast<Terminal<T>*>(ptr);
      }
    }
  }

 public:
  /// @param[in] id  Identifier of the BDD graph.
  explicit Vertex(int id)
      : id_(id),
        use_count_(0),
        control_block_(new ControlBlock{this}) {}

  Vertex(const Vertex&) = delete;
  Vertex& operator=(const Vertex&) = delete;

  /// @returns Identifier of the BDD graph rooted by this vertex.
  int id() const { return id_; }

  /// @returns true if this vertex is terminal.
  bool terminal() const { return id_ < 2; }

  /// @returns The number of registered intrusive pointers.
  int use_count() const { return use_count_; }

  /// @returns true if there is only one registered shared pointer.
  bool unique() const {
    assert(use_count_ && "No registered shared pointers.");
    return use_count_ == 1;
  }

 protected:
  /// Communicates the destruction via the control block
  /// if there's anyone left to care.
  ~Vertex() noexcept {
    if (control_block_->weak_count == 0) {
      delete control_block_;
    } else {
      control_block_->vertex = nullptr;
    }
  }

 private:
  int id_;  ///< Unique identifier of the BDD graph with this vertex.
  int use_count_;  ///< Reference count for the intrusive pointer.
  ControlBlock* control_block_;  ///< Communication channel for pointers.
};

/// Representation of terminal vertices in BDD graphs.
/// It is expected
/// that in reduced BDD graphs,
/// there are at most two terminal vertices of value 1 or 0.
/// If the BDD graph has attributed edges,
/// only single terminal vertex is expected with value 1.
///
/// @tparam T  The type of the main functional BDD vertex.
template <class T>
class Terminal : public Vertex<T> {
 public:
  /// @param[in] value  True or False (1 or 0) terminal.
  explicit Terminal(bool value) : Vertex<T>(value) {}

  /// @returns The value of the terminal vertex.
  ///
  /// @note The value serves as an id for this terminal vertex.
  ///       Non-terminal if-then-else vertices should never have
  ///       identifications of value 0 or 1.
  bool value() const { return Vertex<T>::id(); }

  /// Recovers a shared pointer to Terminal from a pointer to Vertex.
  ///
  /// @param[in] vertex  Pointer to a Vertex known to be a Terminal.
  ///
  /// @return Casted pointer to Terminal.
  static IntrusivePtr<Terminal<T>> Ptr(const IntrusivePtr<Vertex<T>>& vertex) {
    return boost::static_pointer_cast<Terminal<T>>(vertex);
  }
};

/// Representation of non-terminal vertices in BDD graphs.
/// This class is a base class for various BDD-specific vertices.
/// however, as Vertex, NonTerminal is not polymorphic.
///
/// @tparam T  The type of the main functional BDD vertex.
template <class T>
class NonTerminal : public Vertex<T> {
  using VertexPtr = IntrusivePtr<Vertex<T>>;  ///< Convenient change point.

  /// Default logic for getting signature high and low ids.
  ///
  /// @param[in] vertex  Non-terminal vertex.
  ///
  /// @returns Numbers that can be used to uniquely identify the arg vertex.
  /// @{
  friend int get_high_id(const NonTerminal<T>& vertex) noexcept {
    return vertex.high_->id();
  }
  friend int get_low_id(const NonTerminal<T>& vertex) noexcept {
    return vertex.low_->id();
  }
  /// @}

 public:
  /// @param[in] index  Index of this non-terminal vertex.
  /// @param[in] order  Specific ordering number for BDD graphs.
  /// @param[in] id  Unique identifier of the ROBDD graph.
  ///                The identifier should not collide
  ///                with the identifiers (0/1) of terminal nodes.
  /// @param[in] high  A vertex for the (1/True/then/left) branch.
  /// @param[in] low  A vertex for the (0/False/else/right) branch.
  NonTerminal(int index, int order, int id, const VertexPtr& high,
              const VertexPtr& low)
      : Vertex<T>(id),
        high_(high),
        low_(low),
        order_(order),
        index_(index),
        module_(false),
        coherent_(false),
        mark_(false) {}

  /// @returns The index of this vertex.
  int index() const { return index_; }

  /// @returns The order of the vertex.
  int order() const {
    assert(order_ > 0);
    return order_;
  }

  /// @returns true if this vertex represents a module gate.
  bool module() const { return module_; }

  /// Sets this vertex for representation of a module.
  void module(bool flag) { module_ = flag; }

  /// @returns true if the vertex represents a coherent module.
  bool coherent() const { return coherent_; }

  /// Sets the flag for coherent modules.
  ///
  /// @param[in] flag  true for coherent modules.
  void coherent(bool flag) {
    assert(!(coherent_ && !flag) && "Inverting existing coherence.");
    coherent_ = flag;
  }

  /// @returns (1/True/then/left) branch if-then-else vertex.
  const VertexPtr& high() const { return high_; }

  /// @returns (0/False/else/right) branch vertex.
  const VertexPtr& low() const { return low_; }

  /// @returns The mark of this vertex.
  bool mark() const { return mark_; }

  /// Marks this vertex.
  ///
  /// @param[in] flag  A flag with the meaning for the user of marks.
  void mark(bool flag) { mark_ = flag; }

 protected:
  ~NonTerminal() = default;

  /// Cuts off this node from its high and low branches.
  /// This is for destructive operations on the BDD graph.
  ///
  /// @pre These branches are not going to be used again.
  void CutBranches() {
    high_.reset();
    low_.reset();
  }

 private:
  VertexPtr high_;  ///< 1 (True/then) branch in the Shannon decomposition.
  VertexPtr low_;  ///< O (False/else) branch in the Shannon decomposition.
  int order_;  ///< Order of the variable.
  int index_;  ///< Index of the variable.
  bool module_;  ///< Mark for module variables.
  bool coherent_;  ///< Mark for coherence.
  bool mark_;  ///< Traversal mark.
};

/// Representation of non-terminal if-then-else vertices in BDD graphs.
/// This class is designed to help construct and manipulate BDD graphs.
///
/// This class provides one attributed complement edge.
/// The attributed edge applies to the low/false/0 branch of the vertex.
/// However, there is no logic to check
/// if the complement edge manipulations are valid.
/// Consistency is the responsibility of BDD algorithms and users.
class Ite : public NonTerminal<Ite> {
  /// Special handling of the complement flag in computing low id signature.
  ///
  /// @param[in] ite  Ite vertex.
  ///
  /// @returns The signed number for complement low id.
  friend int get_low_id(const Ite& ite) noexcept {
    return ite.complement_edge_ ? -ite.low()->id() : ite.low()->id();
  }

 public:
  using NonTerminal::NonTerminal;  ///< Constructor with index and order.

  /// @returns true if the low edge is complement.
  bool complement_edge() const { return complement_edge_; }

  /// Sets the complement flag for the low edge.
  ///
  /// @param[in] flag  Indicator to treat the low branch as a complement.
  void complement_edge(bool flag) { complement_edge_ = flag; }

  /// @returns The probability of the function graph.
  double p() const { return p_; }

  /// Sets the probability of the function graph.
  ///
  /// @param[in] value  Calculated value for the probability.
  void p(double value) { p_ = value; }

  /// @returns Saved results of importance factor calculations.
  double factor() const { return factor_; }

  /// Memorized results of importance factor calculations.
  ///
  /// @param[in] value  Calculation results for importance factor.
  void factor(double value) { factor_ = value; }

  /// Recovers a shared pointer to Ite from a pointer to Vertex.
  ///
  /// @param[in] vertex  Pointer to a Vertex known to be an Ite.
  ///
  /// @return Casted pointer to Ite.
  static IntrusivePtr<Ite> Ptr(const IntrusivePtr<Vertex<Ite>>& vertex) {
    return boost::static_pointer_cast<Ite>(vertex);
  }

 private:
  bool complement_edge_ = false;  ///< Flag for complement edge.
  double p_ = 0;  ///< Probability of the function graph.
  double factor_ = 0;  ///< Importance factor calculation results.
};

using ItePtr = IntrusivePtr<Ite>;  ///< Shared if-then-else vertices.

/// Prime number generation for hash tables.
///
/// @param[in] n  The starting candidate for a prime number.
///
/// @returns Probable prime number >= n.
int GetPrimeNumber(int n);

/// A hash table for keeping BDD reduced.
/// The management of the hash table is intrusive;
/// that is, it relies on BDD vertices to provide necessary information.
///
/// Each vertex must have a unique signature
/// consisting of its index, special high and low ids.
/// This signature is the key of the hash table;
/// however, it is not duplicated in the table.
/// The key is retrieved from the vertex as needed.
///
/// High and low ids are retrieved through unqualified calls
/// to get_high_id(const T&) and get_low_id(const T&).
/// This allows specialization of id calculations with attributed edges
/// where simple calls for high/low ids may miss the edge information.
///
/// @tparam T  The type of the main functional BDD vertex.
template <class T>
class UniqueTable {
  /// Convenient aliases and customization points.
  /// @{
  using Bucket = std::forward_list<WeakIntrusivePtr<T>>;
  using Table = std::vector<Bucket>;
  /// @}

 public:
  /// Constructor for small graphs.
  ///
  /// @param[in] init_capacity  The starting capacity for the table.
  explicit UniqueTable(int init_capacity = 1000)
      : capacity_(GetPrimeNumber(init_capacity)),
        size_(0),
        max_load_factor_(0.75),
        table_(capacity_) {}

  /// @returns The current number of entries.
  int size() const { return size_; }

  /// Erases all entries.
  void clear() {
    for (Bucket& chain : table_) chain.clear();
  }

  /// Releases all the memory associated with managing this table with BDD.
  ///
  /// @post No use after release.
  //
  /// @note The call for release is not mandatory.
  ///       This functionality is experimental
  ///       to discover best points that minimize memory usage
  ///       considering the responsibilities of the BDD.
  ///       The release keeps the data about the table,
  ///       such as its size and capacity.
  void Release() {
    Table().swap(table_);
  }

  /// Finds an existing BDD vertex or
  /// inserts a default constructed weak pointer for a new vertex.
  /// Proper initialization of the new vertex is responsibility of the BDD.
  ///
  /// Insertion operation may trigger resizing and rehashing.
  /// Rehashing eliminates expired weak pointers.
  ///
  /// Collision resolution may also (opportunistically) remove
  /// expired pointers in the chain.
  ///
  /// @param[in] index  Index of the variable.
  /// @param[in] high_id  The id of the high vertex.
  /// @param[in] low_id  The id of the low vertex.
  ///
  /// @returns Reference to the weak pointer.
  WeakIntrusivePtr<T>& FindOrAdd(int index, int high_id, int low_id) noexcept {
    if (size_ >= (max_load_factor_ * capacity_))
      UniqueTable::Rehash(UniqueTable::GetNextCapacity(capacity_));

    int bucket_number = UniqueTable::Hash(index, high_id, low_id) % capacity_;
    Bucket& chain = table_[bucket_number];
    auto it_prev = chain.before_begin();  // Parent.
    for (auto it_cur = chain.begin(), it_end = chain.end(); it_cur != it_end;) {
      if (it_cur->expired()) {
        it_cur = chain.erase_after(it_prev);
        --size_;
      } else {
        T* vertex = it_cur->get();
        if (index == vertex->index() && high_id == get_high_id(*vertex) &&
            low_id == get_low_id(*vertex)) {
          return *it_cur;
        }
        it_prev = it_cur;
        ++it_cur;
      }
    }
    ++size_;
    return *chain.emplace_after(it_prev);
  }

 private:
  /// Rehashes the table for the new number of buckets.
  /// Upon rehashing the expired nodes are not moved to the new table.
  ///
  /// @param[in] new_capacity  The desired number of buckets.
  void Rehash(int new_capacity) {
    int new_size = 0;
    Table new_table(new_capacity);
    for (Bucket& chain : table_) {
      for (auto it_prev = chain.before_begin(), it_cur = chain.begin(),
                it_end = chain.end();
           it_cur != it_end;) {
        if (it_cur->expired()) {
          it_prev = it_cur;
          ++it_cur;
          continue;
        }
        ++new_size;
        T* vertex = it_cur->get();
        int bucket_number =
            UniqueTable::Hash(vertex->index(), get_high_id(*vertex),
                              get_low_id(*vertex)) %
            new_capacity;
        Bucket& new_chain = new_table[bucket_number];
        new_chain.splice_after(new_chain.before_begin(), chain, it_prev,
                               ++it_cur);
      }
    }
    table_.swap(new_table);
    size_ = new_size;
    capacity_ = new_capacity;
  }

  /// Computes the hash value of the key.
  ///
  /// @param[in] index  Index of the variable.
  /// @param[in] high_id  The id of the high vertex.
  /// @param[in] low_id  The id of the low vertex.
  ///
  /// @returns The combined hash value of the argument numbers.
  std::size_t Hash(int index, int high_id, int low_id) {
    std::size_t seed = 0;
    boost::hash_combine(seed, index);
    boost::hash_combine(seed, high_id);
    boost::hash_combine(seed, low_id);
    return seed;
  }

  /// Computes a new capacity for resizing.
  ///
  /// @param[in] prev_capacity  The current capacity.
  ///
  /// @returns The new capacity scaled by the growth factor function.
  ///
  /// @note The growth tries to take into account the growth patterns of BDD.
  int GetNextCapacity(int prev_capacity) {
    const int kMaxScaleCapacity = 1e8;
    int scale_power = 1;  // The default power after the max scale capacity.
    if (prev_capacity < kMaxScaleCapacity) {
      scale_power += std::log10(kMaxScaleCapacity / prev_capacity);
    }
    int growth_factor = std::pow(2, scale_power);
    int new_capacity =  prev_capacity * growth_factor;
    return GetPrimeNumber(new_capacity);
  }

  int capacity_;  ///< The total number of buckets in the table.
  int size_;  ///< The total number of elements in the table.
  double max_load_factor_;  ///< The limit on the avg. # of elements per bucket.

  /// A table of unique vertices is stored with weak pointers
  /// so that this hash table does not interfere
  /// with BDD node management with shared pointers.
  Table table_;
};

/// A hash table without collision resolution.
/// Instead of resolving the collision,
/// the existing value is purged and replaced by the new entry.
///
/// This hash table is designed to store computation results of BDD Apply.
/// The implementation of the table
/// is very much coupled with the BDD use cases.
///
/// @tparam V  The type of the value/result of BDD Apply.
///            The type must provide swap(), reset(), and operator bool().
///
/// @note The API is designed after STL maps as drop-in replacement for BDD.
///       This approach allows performance testing with the baseline.
///       However, only necessary and sufficient functions are provided.
///
/// @warning The behavior is very different from standard maps.
///          References can easily be invalidated upon rehashing or insertion.
template <class V>
class CacheTable {
 public:
  /// Public typedefs similar to the standard maps.
  ///
  /// @{
  using key_type = std::pair<int, int>;
  using mapped_type = V;
  using value_type = std::pair<key_type, mapped_type>;
  using container_type = std::vector<value_type>;
  using iterator = typename container_type::iterator;
  /// @}

  /// Constructor with average expectations for computations.
  ///
  /// @param[in] init_capacity
  explicit CacheTable(int init_capacity = 1000)
      : size_(0),
        max_load_factor_(0.75),
        table_(GetPrimeNumber(init_capacity)) {}

  /// @returns The number of entires in the table.
  int size() const { return size_; }

  /// Removes all entries from the table.
  void clear() {
    for (value_type& entry : table_) {
      if (entry.second) entry.second.reset();
    }
    size_ = 0;
  }

  /// Prepares the table for more entries.
  ///
  /// @param[in] n  The number of expected entries.
  ///
  /// @post If n is 0 and the table is empty,
  ///       the memory is freed as much as possible.
  ///       Using after release of memory is undefined.
  void reserve(int n) {
    if (size_ == 0 && n == 0) {
      decltype(table_)().swap(table_);
      return;
    }
    if (n <= size_) return;
    CacheTable::Rehash(GetPrimeNumber(n / max_load_factor_ + 1));
  }

  /// Searches for existing entry.
  ///
  /// @param[in] key  Ordered unique ids of BDD Apply argument vertices.
  ///
  /// @returns Iterator pointing to the found entry.
  /// @returns end() if no entry with the given key is found.
  iterator find(const key_type& key) {
    int index = boost::hash_value(key) % table_.size();
    value_type& entry = table_[index];
    if (!entry.second || entry.first != key) return table_.end();
    return table_.begin() + index;
  }

  /// @returns Iterator to the end.
  iterator end() { return table_.end(); }

  /// Emplaces a new entry.
  ///
  /// @param[in] key  Ordered unique ids of BDD Apply argument vertices.
  /// @param[in] value  Non-empty result of BDD Apply computations.
  ///
  /// @warning API deviation from STL maps.
  ///          This function does not return an iterator
  ///          because it is not needed by computations in BDD.
  void emplace(const key_type& key, const mapped_type& value) {
    assert(value && "Empty computation results!");

    if (size_ >= (max_load_factor_ * table_.size()))
      CacheTable::Rehash(GetPrimeNumber(table_.size() * 2));

    int index = boost::hash_value(key) % table_.size();
    value_type& entry = table_[index];
    if (!entry.second) ++size_;
    entry.first = key;  // Key equality is unlikely for the use case.
    entry.second = value;  // Might be purging another value.
  }

 private:
  /// Rehashes the table with a new capacity.
  ///
  /// @param[in] new_capacity  Desired size of the underlying container.
  void Rehash(int new_capacity) {
    int new_size = 0;
    std::vector<value_type> new_table(new_capacity);
    for (value_type& entry : table_) {
      if (!entry.second) continue;
      int new_index = boost::hash_value(entry.first) % new_table.size();
      value_type& new_entry = new_table[new_index];
      new_entry.first = entry.first;
      if (!new_entry.second) ++new_size;
      new_entry.second.swap(entry.second);
    }
    size_ = new_size;
    table_.swap(new_table);
  }

  int size_;  ///< The total number of elements in the table.
  double max_load_factor_;  ///< The limit on (size / capacity) ratio.
  std::vector<value_type> table_;  ///< The main container.
};

class Zbdd;  // For analysis purposes.

/// Analysis of Boolean graphs with Binary Decision Diagrams.
/// This binary decision diagram data structure
/// represents Reduced Ordered BDD with attributed edges.
///
/// @note The low/else edge is chosen to have the attribute for an ITE vertex.
///       There is only one terminal vertex of value 1/True.
class Bdd {
 public:
  using VertexPtr = IntrusivePtr<Vertex<Ite>>;  ///< BDD vertex base.
  using TerminalPtr = IntrusivePtr<Terminal<Ite>>;  ///< Terminal vertices.

  /// Holder of computation resultant functions and gate representations.
  struct Function {
    bool complement;  ///< The interpretation of the function.
    VertexPtr vertex;  ///< The root vertex of the BDD function graph.

    /// @returns true if the function is initialized.
    operator bool() const { return vertex != nullptr; }

    /// Clears the function's root vertex pointer.
    void reset() { vertex = nullptr; }

    /// Swaps with another function.
    void swap(Function& other) noexcept {
      std::swap(complement, other.complement);
      vertex.swap(other.vertex);
    }
  };

  /// Provides access to consensus calculation private facilities.
  class Consensus {
    friend class Zbdd;  // Access for calculation of prime implicants.

    /// Calculates consensus of high and low of an if-then-else BDD vertex.
    ///
    /// @param[in,out] bdd  The host BDD.
    /// @param[in] ite  The BDD vertex with the input.
    /// @param[in] complement  Interpretation of the BDD vertex.
    ///
    /// @returns The consensus BDD function.
    static Function Calculate(Bdd* bdd, const ItePtr& ite,
                              bool complement) noexcept {
      return bdd->CalculateConsensus(ite, complement);
    }
  };

  /// Constructor with the analysis target.
  /// Reduced Ordered BDD is produced from a Boolean graph.
  ///
  /// @param[in] fault_tree  Preprocessed, partially normalized,
  ///                        and indexed fault tree.
  /// @param[in] settings  The analysis settings.
  ///
  /// @pre The Boolean graph has variable ordering.
  ///
  /// @note BDD construction may take considerable time.
  Bdd(const BooleanGraph* fault_tree, const Settings& settings);

  Bdd(const Bdd&) = delete;
  Bdd& operator=(const Bdd&) = delete;

  /// To handle incomplete ZBDD type with unique pointers.
  ~Bdd() noexcept;

  /// @returns The root function of the ROBDD.
  const Function& root() const { return root_; }

  /// @returns Mapping of Boolean graph modules and BDD graph vertices.
  const std::unordered_map<int, Function>& modules() const { return modules_; }

  /// @returns Mapping of variable indices to their orders.
  const std::unordered_map<int, int>& index_to_order() const {
    return index_to_order_;
  }

  /// @returns true if the BDD has been constructed from a coherent PDAG.
  bool coherent() const { return coherent_; }

  /// Helper function to clear and set vertex marks.
  ///
  /// @param[in] mark  Desired mark for BDD vertices.
  ///
  /// @note Marks will propagate to modules as well.
  ///
  /// @warning If the graph is discontinuously and partially marked,
  ///          this function will not help with the mess.
  void ClearMarks(bool mark) { Bdd::ClearMarks(root_.vertex, mark); }

  /// Runs the Qualitative analysis
  /// with the representation of a Boolean graph as ROBDD.
  void Analyze() noexcept;

  /// @returns Products generated by the analysis.
  ///
  /// @pre Analysis is done.
  const std::vector<std::vector<int>>& products() const;

 private:
  using IteWeakPtr = WeakIntrusivePtr<Ite>;  ///< Pointer in containers.
  using ComputeTable = CacheTable<Function>;  ///< Computation results.

  /// Finds or adds a unique if-then-else vertex in BDD.
  /// All vertices in the BDD must be created with this functions.
  /// Otherwise, the BDD may not be reduced.
  ///
  /// @param[in] index  Positive index of the variable.
  /// @param[in] high  The high vertex.
  /// @param[in] low  The low vertex.
  /// @param[in] complement_edge  Interpretation of the low vertex.
  /// @param[in] order The order for the vertex variable.
  ///
  /// @returns If-then-else node with the given parameters.
  ///
  /// @pre Non-expired pointers in the unique table are
  ///      either in the BDD or in the computation table.
  ItePtr FindOrAddVertex(int index, const VertexPtr& high, const VertexPtr& low,
                         bool complement_edge, int order) noexcept;

  /// Finds or adds a replacement for an existing node
  /// or a new node based on an existing node.
  ///
  /// @param[in] ite  An existing vertex.
  /// @param[in] high  The new high vertex.
  /// @param[in] low  The new low vertex.
  /// @param[in] complement_edge  Interpretation of the low vertex.
  ///
  /// @returns Ite for a replacement.
  ///
  /// @warning This function is not aware of reduction rules.
  ItePtr FindOrAddVertex(const ItePtr& ite, const VertexPtr& high,
                         const VertexPtr& low, bool complement_edge) noexcept;

  /// Find or adds a BDD ITE vertex using information from gates.
  ///
  /// @param[in] gate  Gate with index, order, and other information.
  /// @param[in] high  The new high vertex.
  /// @param[in] low  The new low vertex.
  /// @param[in] complement_edge  Interpretation of the low vertex.
  ///
  /// @returns Ite for a replacement.
  ///
  /// @pre The gate is a module.
  ///
  /// @warning This function is not aware of reduction rules.
  ItePtr FindOrAddVertex(const Gate& gate, const VertexPtr& high,
                         const VertexPtr& low, bool complement_edge) noexcept;

  /// Converts all gates in the Boolean graph
  /// into function BDD graphs.
  /// Registers processed gates.
  ///
  /// @param[in] gate  The root or current parent gate of the graph.
  /// @param[in,out] gates  Processed gates with use counts.
  ///
  /// @returns The BDD function representing the gate.
  ///
  /// @pre The memoization container is not used outside of this function.
  Function ConvertGraph(
      const Gate& gate,
      std::unordered_map<int, std::pair<Function, int>>* gates) noexcept;

  /// Computes minimum and maximum ids for keys in computation tables.
  ///
  /// @param[in] arg_one  First argument function graph.
  /// @param[in] arg_two  Second argument function graph.
  /// @param[in] complement_one  Interpretation of arg_one as complement.
  /// @param[in] complement_two  Interpretation of arg_two as complement.
  ///
  /// @returns A pair of min and max integers with a sign for a complement.
  ///
  /// @pre The arguments are not be the same function.
  ///      Equal ID functions are handled by the reduction.
  /// @pre Even though the arguments are not ItePtr type,
  ///      they are if-then-else vertices.
  std::pair<int, int> GetMinMaxId(const VertexPtr& arg_one,
                                  const VertexPtr& arg_two, bool complement_one,
                                  bool complement_two) noexcept;

  /// Applies Boolean operation to BDD graphs.
  /// This is the main function for the operation.
  /// The application is specialized with the operator.
  ///
  /// @tparam Type  The operator enum.
  ///
  /// @param[in] arg_one  First argument function graph.
  /// @param[in] arg_two  Second argument function graph.
  /// @param[in] complement_one  Interpretation of arg_one as complement.
  /// @param[in] complement_two  Interpretation of arg_two as complement.
  ///
  /// @returns The BDD function as a result of operation.
  ///
  /// @note The order of arguments does not matter for two variable operators.
  template <Operator Type>
  Function Apply(const VertexPtr& arg_one, const VertexPtr& arg_two,
                 bool complement_one, bool complement_two) noexcept;

  /// Applies Boolean operation to BDD ITE graphs.
  ///
  /// @tparam Type  The operator enum.
  ///
  /// @param[in] ite_one  First argument function graph.
  /// @param[in] ite_two  Second argument function graph.
  /// @param[in] complement_one  Interpretation of arg_one as complement.
  /// @param[in] complement_two  Interpretation of arg_two as complement.
  ///
  /// @returns The BDD function as a result of operation.
  template <Operator Type>
  Function Apply(ItePtr ite_one, ItePtr ite_two, bool complement_one,
                 bool complement_two) noexcept;

  /// Applies Boolean operation to BDD graphs.
  /// This is a convenience function
  /// if the operator type cannot be determined at compile time.
  ///
  /// @param[in] type  The operator or type of the gate.
  /// @param[in] arg_one  First argument function graph.
  /// @param[in] arg_two  Second argument function graph.
  /// @param[in] complement_one  Interpretation of arg_one as complement.
  /// @param[in] complement_two  Interpretation of arg_two as complement.
  ///
  /// @returns The BDD function as a result of operation.
  ///
  /// @pre The operator is either AND or OR.
  ///
  /// @note The order of arguments does not matter for two variable operators.
  Function Apply(Operator type,
                 const VertexPtr& arg_one, const VertexPtr& arg_two,
                 bool complement_one, bool complement_two) noexcept;

  /// Calculates consensus of high and low of an if-then-else BDD vertex.
  ///
  /// @param[in] ite  The BDD vertex with the input.
  /// @param[in] complement  Interpretation of the BDD vertex.
  ///
  /// @returns The consensus BDD function.
  Function CalculateConsensus(const ItePtr& ite, bool complement) noexcept;

  /// Counts the number of if-then-else nodes.
  ///
  /// @param[in] vertex  The starting root vertex of BDD.
  ///
  /// @returns The number of ITE nodes in the BDD.
  ///
  /// @pre Non-terminal node marks are clear (false).
  int CountIteNodes(const VertexPtr& vertex) noexcept;

  /// Clears marks of vertices in BDD graph.
  ///
  /// @param[in] vertex  The starting root vertex of the graph.
  /// @param[in] mark  The desired mark for the vertices.
  ///
  /// @note Marks will propagate to modules as well.
  void ClearMarks(const VertexPtr& vertex, bool mark) noexcept;

  /// Checks BDD graphs for errors in the structure.
  /// Errors are assertions that fail at runtime.
  ///
  /// @param[in] vertex  The root vertex of BDD.
  ///
  /// @pre Non-terminal node marks are clear (false).
  void TestStructure(const VertexPtr& vertex) noexcept;

  /// Clears all memoization tables.
  void ClearTables() noexcept {
    and_table_.clear();
    or_table_.clear();
  }

  const Settings kSettings_;  ///< Analysis settings.
  Function root_;  ///< The root function of this BDD.
  bool coherent_;  ///< Inherited coherence from PDAG.

  /// Table of unique if-then-else nodes denoting function graphs.
  /// The key consists of ite(index, id_high, id_low),
  /// where IDs are unique (id_high != id_low) identifications of
  /// unique reduced-ordered function graphs.
  UniqueTable<Ite> unique_table_;

  /// Tables of processed computations over functions.
  /// The argument functions are recorded with their IDs (not vertex indices).
  /// In order to keep only unique computations,
  /// the argument IDs must be ordered.
  /// The key is {min_id, max_id}.
  /// @{
  ComputeTable and_table_;
  ComputeTable or_table_;
  /// @}

  std::unordered_map<int, Function> modules_;  ///< Module graphs.
  std::unordered_map<int, int> index_to_order_;  ///< Indices and orders.
  const TerminalPtr kOne_;  ///< Terminal True.
  int function_id_;  ///< Identification assignment for new function graphs.
  std::unique_ptr<Zbdd> zbdd_;  ///< ZBDD as a result of analysis.
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_BDD_H_
