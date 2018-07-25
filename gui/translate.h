/*
 * Copyright (C) 2018 Olzhas Rakhimov
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
/// Translation helper facilities to workaround Qt Linguist shortcomings.

#pragma once

#include <QObject>

namespace scram::gui {

/// Forwards to translate function with a default global context.
template <typename... Ts>
decltype(auto) _(Ts &&... args)
{
    return QObject::tr(std::forward<Ts>(args)...);
}

} // namespace scram::gui
