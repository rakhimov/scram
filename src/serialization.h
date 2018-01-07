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
/// The MEF Model serialization facilities.
///
/// @note This facility currently caters only models representable in the GUI.
/// @todo Implement serialization for all MEF constructs.

#pragma once

#include <cstdio>

#include <string>

#include "model.h"

namespace scram::mef {

/// Serializes the model and its data into stream as XML.
///
/// @param[in] model  Fully initialized and valid model.
/// @param[in,out] out  The stream for XML data.
///
/// @throws IOError  The write operation has failed.
void Serialize(const Model& model, std::FILE* out);

/// Convenience function for serialization into a file.
///
/// @param[in] model  Fully initialized and valid model.
/// @param[out] file  The output destination.
///
/// @throws IOError  The output file is not accessible,
///                  or the write operation has failed.
void Serialize(const Model& model, const std::string& file);

}  // namespace scram::mef
