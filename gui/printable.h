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

/// @file printable.h
/// Interface for printable objects.

#ifndef PRINTABLE_H
#define PRINTABLE_H

#include <QPrinter>

namespace scram {
namespace gui {

class Printable {
public:
    virtual ~Printable() = default;

    /// Prints with a print dialog.
    void print();
    /// Prints with a print preview dialog.
    void printPreview();

private:
    /// The actual printing must be implemented by derived classes.
    virtual void doPrint(QPrinter *printer) = 0;

};

} // namespace gui
} // namespace scram

#endif // PRINTABLE_H
