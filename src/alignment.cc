/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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
/// Implementation of alignment and phase classes.

#include "alignment.h"

#include "error.h"
#include "ext/float_compare.h"

namespace scram::mef {

Phase::Phase(std::string name, double time_fraction)
    : Element(std::move(name)), time_fraction_(time_fraction) {
  if (time_fraction_ <= 0 || time_fraction_ > 1)
    SCRAM_THROW(DomainError("The phase fraction must be in (0, 1]."))
        << errinfo_value(std::to_string(time_fraction_))
        << errinfo_element(Element::name(), kTypeString);
}

void Alignment::Validate() {
  double sum = 0;
  for (const Phase& phase : phases())
    sum += phase.time_fraction();
  if (!ext::is_close(1, sum, 1e-4))
    SCRAM_THROW(ValidityError("The phases of the alignment do not sum to 1."))
        << errinfo_value(std::to_string(sum))
        << errinfo_element(Element::name(), kTypeString);
}

}  // namespace scram::mef
