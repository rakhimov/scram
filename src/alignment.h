/*
 * Copyright (C) 2017 Olzhas Rakhimov
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

/// @file alignment.h
/// Mission and phase constructs.

#ifndef SCRAM_SRC_ALIGNMENT_H_
#define SCRAM_SRC_ALIGNMENT_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

#include "element.h"
#include "instruction.h"

namespace scram {
namespace mef {

/// Phases of alignments the models spends its time fraction.
class Phase : public Element, private boost::noncopyable {
 public:
  /// @copydoc Element::Element
  ///
  /// @param[in] time_fraction  The fraction of mission-time spent in the phase.
  ///
  /// @throws DomainError  The fraction is not a valid value in (0, 1].
  Phase(std::string name, double time_fraction);

  /// @returns The positive fraction of mission-time spent in this phase.
  double time_fraction() const { return time_fraction_; }

  /// @returns The instructions applied in this phase.
  const std::vector<SetHouseEvent*>& instructions() const {
    return instructions_;
  }

  /// @param[in] instructions  Zero or more instructions for this phase.
  void instructions(std::vector<SetHouseEvent*> instructions) {
    instructions_ = std::move(instructions);
  }

 public:
  double time_fraction_;  ///< The positive fraction of the mission time.
  std::vector<SetHouseEvent*> instructions_;  ///< The phase modifiers.
};

using PhasePtr = std::unique_ptr<Phase>;  ///< Phases are unique to alignments.

/// Alignment configuration for the whole model per analysis.
class Alignment : public Element, private boost::noncopyable {
 public:
  using Element::Element;

  /// @returns The phases defined in the alignment.
  const ElementTable<PhasePtr>& phases() const { return phases_; }

  /// Adds a phase into alignment.
  ///
  /// @param[in] phase  One of the unique phases for the alignment.
  ///
  /// @throws DuplicateArgumentError  The phase is duplicate.
  void Add(PhasePtr phase);

  /// Ensures that all phases add up to be valid for the alignment.
  ///
  /// @throws ValidityError  Phases are incomplete (e.g., don't sum to 1).
  void Validate();

 private:
  ElementTable<PhasePtr> phases_;  ///< The partitioning of the alignment.
};

using AlignmentPtr = std::unique_ptr<Alignment>;  ///< Unique model alignments.

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_ALIGNMENT_H_
