/*
 * Copyright (C) 2015 Olzhas Rakhimov
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

#include "bdd.h"

namespace scram {

/// @class SetNode
/// Representation of non-terminal nodes in ZBDD.
class SetNode : public NonTerminal<SetNode> {
 public:
  using TerminalPtr = std::shared_ptr<Terminal>;
  using SetNodePtr = std::shared_ptr<SetNode>;
  using NonTerminal::NonTerminal;  ///< Constructor with index and order.
};

/// @class Zbdd
/// Zero-Suppressed Binary Desicision Diagrams for set manipulations.
class Zbdd {
 public:
  Zbdd();

 private:
  using TerminalPtr = std::shared_ptr<Terminal>;
  using SetNodePtr = std::shared_ptr<SetNode>;
  using HashTable = TripletTable<SetNodePtr>;

  /// Table of unique SetNodes denoting sets.
  /// The key consists of (index, id_high, id_low) triplet.
  HashTable unique_table_;

  /// Table of processed computations over sets.
  /// The key must convey the semantics of the operation over sets.
  /// The argument functions are recorded with their IDs (not vertex indices).
  /// In order to keep only unique computations,
  /// the argument IDs must be ordered.
  HashTable compute_table_;

  const TerminalPtr kBase_;  ///< Terminal Base (Unity/1) set.
  const TerminalPtr kEmpty_;  ///< Terminal Empty (Null/0) set.
  int set_id_;  ///< Identification assignment for new set graphs.
};

}  // namespace scram

#endif  // SCRAM_SRC_ZBDD_H_
