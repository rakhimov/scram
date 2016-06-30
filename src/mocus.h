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

/// @file mocus.h
/// Fault tree analysis with the MOCUS algorithm.
/// This algorithm requires a fault tree in negation normal form.
/// The fault tree must only contain AND and OR gates.
/// All gates must be positive.
/// That is, negations must be pushed down to leaves, basic events.
/// The fault tree should not contain constants or house events.

#ifndef SCRAM_SRC_MOCUS_H_
#define SCRAM_SRC_MOCUS_H_

#include <unordered_map>
#include <vector>

#include "boolean_graph.h"
#include "settings.h"
#include "zbdd.h"

namespace scram {
namespace core {

/// This class analyzes normalized, preprocessed, and indexed fault trees
/// to generate minimal cut sets with the MOCUS algorithm.
class Mocus {
 public:
  /// Prepares a Boolean graph for analysis with the MOCUS algorithm.
  ///
  /// @param[in] fault_tree  Preprocessed, normalized, and indexed fault tree.
  /// @param[in] settings  The analysis settings.
  ///
  /// @pre The passed Boolean graph already has variable ordering.
  /// @pre The Boolean graph is in negation normal form;
  ///      that is, it contains only positive AND/OR gates.
  Mocus(const BooleanGraph* fault_tree, const Settings& settings);

  Mocus(const Mocus&) = delete;
  Mocus& operator=(const Mocus&) = delete;

  /// Finds minimal cut sets from the Boolean graph.
  void Analyze();

  /// @returns Generated minimal cut sets with basic event indices.
  const std::vector<std::vector<int>>& products() const;

 private:
  /// Runs analysis on a module gate.
  /// All sub-modules are analyzed and joined recursively.
  ///
  /// @param[in] gate  A Boolean graph gate for analysis.
  /// @param[in] settings  Settings for analysis.
  ///
  /// @returns Fully processed, minimized Zbdd cut set container.
  std::unique_ptr<zbdd::CutSetContainer>
  AnalyzeModule(const Gate& gate, const Settings& settings) noexcept;

  bool constant_graph_;  ///< No need for analysis.
  const BooleanGraph* graph_;  ///< The analysis PDAG.
  const Settings kSettings_;  ///< Analysis settings.
  std::unique_ptr<Zbdd> zbdd_;  ///< ZBDD as a result of analysis.
};

}  // namespace core
}  // namespace scram

#endif  // SCRAM_SRC_MOCUS_H_
