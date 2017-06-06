/*
 * Copyright (C) 2016-2017 Olzhas Rakhimov
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

#include <QApplication>
#include <QFontMetrics>
#include <QGraphicsPathItem>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QStyleOptionGraphicsItem>

#include "guiassert.h"

namespace scram {
namespace gui {
namespace diagram {

const QSize Event::m_size = {16, 11};
const double Event::m_baseHeight = 6.5;
const double Event::m_idBoxLength = 10;
const double Event::m_labelBoxHeight = 4;

Event::Event(const mef::Event &event, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_event(event), m_typeGraphics(nullptr)
{
}

QSize Event::units() const
{
    QFontMetrics font = QApplication::fontMetrics();
    return {font.averageCharWidth(), font.height()};
}

double Event::width() const
{
  return m_size.width() * units().width();
}

void Event::setTypeGraphics(QGraphicsItem *item)
{
    delete m_typeGraphics;
    m_typeGraphics = item;
    m_typeGraphics->setParentItem(this);
    m_typeGraphics->setPos(0, m_baseHeight * units().height());
}

QRectF Event::boundingRect() const
{
    int w = units().width();
    int h = units().height();
    double labelBoxWidth = m_size.width() * w;
    return QRectF(-labelBoxWidth / 2, 0, labelBoxWidth, m_baseHeight * h);
}

void Event::paint(QPainter *painter,
                  const QStyleOptionGraphicsItem * /*option*/,
                  QWidget * /*widget*/)
{
    int w = units().width();
    int h = units().height();
    double labelBoxWidth = m_size.width() * w ;
    QRectF rect(-labelBoxWidth / 2, 0, labelBoxWidth, m_labelBoxHeight * h);
    painter->drawRect(rect);
    painter->drawText(rect, Qt::AlignCenter | Qt::TextWordWrap,
                      QString::fromStdString(m_event.label()));

    painter->drawLine(QPointF(0, m_labelBoxHeight * h),
                      QPointF(0, (m_labelBoxHeight + 1) * h));

    double idBoxWidth = m_idBoxLength * w;
    QRectF nameRect(-idBoxWidth / 2, (m_labelBoxHeight + 1) * h, idBoxWidth, h);
    painter->drawRect(nameRect);
    painter->drawText(nameRect, Qt::AlignCenter,
                      QString::fromStdString(m_event.name()));

    painter->drawLine(QPointF(0, (m_labelBoxHeight + 2) * h),
                      QPointF(0, (m_labelBoxHeight + 2.5) * h));
}

BasicEvent::BasicEvent(const mef::BasicEvent &event, QGraphicsItem *parent)
    : Event(event, parent)
{
    double d = int(m_size.height() - m_baseHeight) * units().height();
    Event::setTypeGraphics(new QGraphicsEllipseItem(-d / 2, 0, d, d));
}

const QSize Gate::m_maxSize = {6, 3};

Gate::Gate(const mef::Gate &event, QGraphicsItem *parent) : Event(event, parent)
{
    double availableHeight
        = m_size.height() - m_baseHeight - m_maxSize.height();
    auto *pathItem = new QGraphicsLineItem(
        0, 0, 0, (availableHeight - 1) * units().height(), this);
    pathItem->setPos(0, (m_baseHeight + m_maxSize.height()) * units().height());
    Event::setTypeGraphics(
        getGateGraphicsType(event.formula().type()).release());
}

std::unique_ptr<QGraphicsItem> Gate::getGateGraphicsType(mef::Operator type)
{
    switch (type) {
    case mef::kNull:
        return std::make_unique<QGraphicsLineItem>(
            0, 0, 0, m_maxSize.height() * units().height());
    case mef::kAnd: {
        QPainterPath paintPath;
        double x1 = m_maxSize.width() * units().width() / 2;
        double maxHeight = m_maxSize.height() * units().height();
        paintPath.moveTo(0, maxHeight);
        paintPath.arcTo(-x1, 0, x1 * 2, maxHeight * 2, 0, 180);
        paintPath.closeSubpath();
        return std::make_unique<QGraphicsPathItem>(paintPath);
    }
    case mef::kOr: {
        QPainterPath paintPath;
        double x1 = m_maxSize.width() * units().width() / 2;
        double maxHeight = m_maxSize.height() * units().height();
        QRectF rectangle(-x1, 0, x1 * 2, maxHeight * 2);
        paintPath.arcMoveTo(rectangle, 0);
        paintPath.arcTo(rectangle, 0, 180);
        double lowerArc = 0.25;
        rectangle.setHeight(rectangle.height() * lowerArc);
        rectangle.moveTop(maxHeight * (1 - lowerArc));
        paintPath.arcMoveTo(rectangle, 0);
        paintPath.arcTo(rectangle, 0, 180);
        paintPath.arcMoveTo(rectangle, 90);
        paintPath.lineTo(0, maxHeight);
        return std::make_unique<QGraphicsPathItem>(paintPath);
    }
    default:
        GUI_ASSERT(false && "Unexpected gate type", nullptr);
    }
}

} // namespace diagram
} // namespace gui
} // namespace scram
