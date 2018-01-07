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
/// Localization and translation facilities.

#pragma once

#include <string>
#include <vector>

#include <QString>

namespace scram::gui {

/// @returns The path to SCRAM GUI translations directory.
const std::string &translationsPath();

/// @returns Available translations represented with locale with underscores.
///
/// @post The default English is not expected to be in translations.
std::vector<std::string> translations();

/// @param[in] locale  The locale code with underscores.
///
/// @returns The native language name ready to be used in UI.
///
/// @pre The locale string is valid as required by QLocale.
/// @pre The locale language is a valid human language (not "C").
QString nativeLanguageName(const std::string &locale);

} // namespace scram::gui
