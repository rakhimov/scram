/*
 * Copyright (C) 2014-2018 Olzhas Rakhimov
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
/// Implementation of expression parameters.

#include "parameter.h"

#include "error.h"

namespace scram::mef {

MissionTime::MissionTime(double time, Units unit) : unit_(unit) { value(time); }

void MissionTime::value(double time) {
  if (time < 0)
    SCRAM_THROW(LogicError("Mission time cannot be negative."));
  value_ = time;
}

void Parameter::expression(Expression* expression) {
  if (expression_)
    SCRAM_THROW(LogicError("Parameter expression is already set."));
  expression_ = expression;
  Expression::AddArg(expression);
}

}  // namespace scram::mef
