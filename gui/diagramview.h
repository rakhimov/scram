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
/// Provides a GraphicsView with common functionalities for diagrams.

#pragma once

#include <QMouseEvent>

#include "printable.h"
#include "zoomableview.h"

namespace scram::gui {

/// The default view for graphics views (e.g., fault tree diagram).
class DiagramView : public ZoomableView, public Printable
{
public:
    using ZoomableView::ZoomableView;

    /// Exports the image of the diagram.
    void exportAs();

protected:
    /// Provides support for starting the panning of the window
    /// using the left mouse button.
    ///
    /// @param[in] event  The input mouse press event.
    void mousePressEvent(QMouseEvent *event) override;

    /// Provides support for stopping the panning of the window
    /// using the left mouse button.
    ///
    /// @param[in] event  The input mouse release event.
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void doPrint(QPrinter *printer) override;
};

} // namespace scram::gui
