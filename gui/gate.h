/*
 * Copyright (C) 2016 Olzhas Rakhimov
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

#ifndef GATE_H
#define GATE_H

#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace scram {
namespace gui {

/**
 * @brief The abstract base class for various gate types.
 */
class Gate
{
public:
    Gate(const Gate&) = delete;
    Gate& operator=(const Gate&) = delete;

    virtual ~Gate() = default;

    /**
     * @brief Paints the shape of the gate without its inputs.
     *
     * @param painter  The reciever of the image.
     * @param option  Holder of reference units for the sizes.
     */
    virtual void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option) = 0;
};

} // namespace gui
} // namespace scram

#endif // GATE_H
