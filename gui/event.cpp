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

#include "event.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QStyleOptionGraphicsItem>

namespace scram {
namespace gui {

template <class T>
Event::Event(const T &, QGraphicsView *view)
    : m_view(view), m_typeGraphics(Event::getTypeGraphics<T>(units()))
{
    m_view->scene()->addItem(this);
    if (m_typeGraphics) {
        m_typeGraphics->setParentItem(this);
        m_typeGraphics->setPos(0, 5.5 * units().height());
    }
}

QSize Event::units() const
{
    QFontMetrics font = m_view->fontMetrics();
    return {font.averageCharWidth(), font.height()};
}

void Event::setTypeGraphics(QGraphicsItem *item)
{
    delete m_typeGraphics;
    m_typeGraphics = item;
    m_typeGraphics->setParentItem(this);
    m_typeGraphics->setPos(0, 5.5 * units().height());
}

QRectF Event::boundingRect() const
{
    int w = units().width();
    int h = units().height();
    return QRectF(-8 * w, 0, 16 * w, 5.5 * h);
}

void Event::paint(QPainter *painter,
                  const QStyleOptionGraphicsItem * /*option*/,
                  QWidget * /*widget*/)
{
    int w = units().width();
    int h = units().height();
    QRectF rect(-8 * w, 0, 16 * w, 3 * h);
    painter->drawRect(rect);
    painter->drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, m_description);

    painter->drawLine(QPointF(0, 3 * h), QPointF(0, 4 * h));

    QRectF nameRect(-5 * w, 4 * h, 10 * w, h);
    painter->drawRect(nameRect);
    painter->drawText(nameRect, Qt::AlignCenter, m_name);

    painter->drawLine(QPointF(0, 5 * h), QPointF(0, 5.5 * h));
}

template <>
QGraphicsItem *Event::getTypeGraphics<BasicEvent>(const QSize &units)
{
    double r = 5 * units.width();
    double d = 2 * r;
    return new QGraphicsEllipseItem(-r, 0, d, d);
}

BasicEvent::BasicEvent(QGraphicsView *view) : Event(*this, view) {}

IntermediateEvent::IntermediateEvent(QGraphicsView *view) : Event(*this, view)
{
}

} // namespace gui
} // namespace scram
