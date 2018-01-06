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
/// Provides a GraphicsView with zoom in/out and other convenience features.

#pragma once

#include <QGraphicsView>
#include <QWheelEvent>

namespace scram::gui {

/// The base class for graphics views with default zoom logic.
/// The zoom level is given as percents.
class ZoomableView : public QGraphicsView
{
    Q_OBJECT

public:
    using QGraphicsView::QGraphicsView;

    /// @returns The zoom value in percentages.
    int getZoom() const { return m_zoom; }

signals:
    /// @param[in] level  The current zoom level.
    void zoomChanged(int level);

public slots:
    /// Accepts requests to change the zoom to the given level.
    void setZoom(int level);

    /// Incrementally zooms in by the absolute increment in the level.
    void zoomIn(int deltaLevel) { setZoom(m_zoom + deltaLevel); }

    /// Incrementally zooms out by the absolute decrement in the level.
    void zoomOut(int deltaLevel) { setZoom(m_zoom - deltaLevel); }

    /// Automatically adjusts the zoom to fit the view into the current scene.
    void zoomBestFit();

protected:
    /// Provides support for zoom-in/out with a mouse while holding Control key.
    void wheelEvent(QWheelEvent *event) override;

private:
    static const int m_minZoomLevel = 10; ///< The minimum allowed zoom level.

    int m_zoom = 100; ///< The zoom level value in percents.
};

} // namespace scram::gui
