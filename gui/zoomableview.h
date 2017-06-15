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

/// @file zoomableview.h
/// Provides a GraphicsView with zoom in/out and other convenience features.

#ifndef ZOOMABLEVIEW_H
#define ZOOMABLEVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>

namespace scram {
namespace gui {

class ZoomableView : public QGraphicsView
{
    Q_OBJECT

public:
    using QGraphicsView::QGraphicsView;

    /// @returns The zoom value in percentages.
    int getZoom() const { return m_zoom; }

signals:
    void zoomChanged(int level);
    void zoomEnabled(int level);
    void zoomDisabled();

public slots:
    void setZoom(int level);
    void zoomIn(int deltaLevel) { setZoom(m_zoom + deltaLevel); }
    void zoomOut(int deltaLevel) { setZoom(m_zoom - deltaLevel); }

protected:
    void wheelEvent(QWheelEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    static const int m_minZoomLevel;

    int m_zoom = 100; ///< The value is in percents.
};

} // namespace gui
} // namespace scram

#endif // ZOOMABLEVIEW_H
