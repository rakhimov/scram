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

/// @file

#include "zoomableview.h"

#include <QMatrix>

namespace scram {
namespace gui {

void ZoomableView::setZoom(int level)
{
    if (level == m_zoom)
        return;
    if (level < m_minZoomLevel)
        level = m_minZoomLevel;

    double scaleValue = 0.01 * level;
    QMatrix matrix;
    matrix.scale(scaleValue, scaleValue);
    this->setMatrix(matrix);
    m_zoom = level;

    emit zoomChanged(level);
}

void ZoomableView::zoomBestFit()
{
    QSize viewSize = size();
    QSize sceneSize = scene()->sceneRect().size().toSize();
    double ratioHeight =
        static_cast<double>(viewSize.height()) / sceneSize.height();
    double ratioWidth =
        static_cast<double>(viewSize.width()) / sceneSize.width();
    setZoom(std::min(ratioHeight, ratioWidth) * 100);
}

void ZoomableView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->delta() > 0)
            zoomIn(5);
        else
            zoomOut(5);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

} // namespace gui
} // namespace scram
