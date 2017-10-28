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

/// @file substitution.h
/// The MEF Substitution constructs.

#ifndef SCRAM_SRC_SUBSTITUTION_H_
#define SCRAM_SRC_SUBSTITUTION_H_

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/variant.hpp>

#include "element.h"
#include "event.h"

namespace scram {
namespace mef {

/// The general representation for
/// Delete Terms, Recovery Rules, and Exchange Events.
class Substitution : public Element, private boost::noncopyable {
 public:
  using Target = boost::variant<BasicEvent*, bool>;  ///< The target type.

  /// The substitution types.
  enum Type {
    kDelete,
    kRecovery,
    kExchange
  };

  using Element::Element;

  /// @returns The formula hypothesis of the substitution.
  ///
  /// @pre The required hypothesis formula has been set.
  const Formula& hypothesis() const {
    assert(hypothesis_ && "Substitution hypothesis is not set.");
    return *hypothesis_;
  }

  /// Sets the substitution hypothesis formula.
  ///
  /// @param[in] formula  Simple Boolean formula built over basic events only.
  void hypothesis(FormulaPtr formula) {
    assert(formula && "Cannot unset the hypothesis of substitution.");
    hypothesis_ = std::move(formula);
  }

  /// @returns The target of the substitution.
  ///
  /// @pre The target has been set.
  const Target& target() const { return target_; }

  /// Sets the target of the substitution.
  ///
  /// @param[in] target_event  Basic event or Boolean constant.
  void target(Target target_event) { target_ = std::move(target_event); }

  /// @returns The source events of the substitution.
  const std::vector<BasicEvent*>& source() const { return source_; }

  /// Adds a source event to the substitution container.
  ///
  /// @param[in] source_event  The event to be replaced by the target event.
  ///
  /// @throws DuplicateArgumentError  The source event is duplicate.
  void Add(BasicEvent* source_event);

 private:
  FormulaPtr hypothesis_;  ///< The formula to be satisfied.
  std::vector<BasicEvent*> source_;  ///< The source events to be replaced.
  Target target_;  ///< The target event to replace the source events.
};

}  // namespace mef
}  // namespace scram

#endif  // SCRAM_SRC_SUBSTITUTION_H_
