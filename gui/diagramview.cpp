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

#include "diagramview.h"

#include <QFileDialog>
#include <QSvgGenerator>

#include "translate.h"

namespace scram::gui {

void DiagramView::exportAs()
{
    QString filename =
        QFileDialog::getSaveFileName(this, _("Export As"), QDir::homePath(),
                                     _("SVG files (*.svg);;All files (*.*)"));
    QSize sceneSize = scene()->sceneRect().size().toSize();

    QSvgGenerator generator;
    generator.setFileName(filename);
    generator.setSize(sceneSize);
    generator.setViewBox(QRect(0, 0, sceneSize.width(), sceneSize.height()));
    generator.setTitle(filename);
    QPainter painter;
    painter.begin(&generator);
    scene()->render(&painter);
    painter.end();
}

void DiagramView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        this->setDragMode(QGraphicsView::ScrollHandDrag);

    QGraphicsView::mousePressEvent(event);
}

void DiagramView::mouseReleaseEvent(QMouseEvent *event)
{
    this->setDragMode(QGraphicsView::NoDrag);
    QGraphicsView::mouseReleaseEvent(event);
}

void DiagramView::doPrint(QPrinter *printer)
{
    QPainter painter(printer);
    painter.setRenderHint(QPainter::Antialiasing);
    scene()->render(&painter);
}

} // namespace scram::gui
