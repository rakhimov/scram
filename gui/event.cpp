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

#include <cmath>

#include <vector>

#include <QApplication>
#include <QFontMetrics>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QStyleOptionGraphicsItem>

#include "src/ext/find_iterator.h"

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

HouseEvent::HouseEvent(const mef::HouseEvent &event, QGraphicsItem *parent)
    : Event(event, parent)
{
    double h = int(m_size.height() - m_baseHeight) * units().height();
    double y0 = h * 0.25;
    Event::setTypeGraphics(new QGraphicsPolygonItem(
        {{{0, 0}, {-h / 2, y0}, {-h / 2, h}, {h / 2, h}, {h / 2, y0}}}));
}

UndevelopedEvent::UndevelopedEvent(const mef::BasicEvent &event,
                                   QGraphicsItem *parent)
    : Event(event, parent)
{
    double h = int(m_size.height() - m_baseHeight) * units().height();
    double a = h / std::sqrt(2);
    auto *diamond = new QGraphicsRectItem(-a / 2, (h - a) / 2, a, a);
    diamond->setTransformOriginPoint(0, h / 2);
    diamond->setRotation(45);
    Event::setTypeGraphics(diamond);
}

ConditionalEvent::ConditionalEvent(const mef::BasicEvent &event,
                                   QGraphicsItem *parent)
    : Event(event, parent)
{
    double d = int(m_size.height() - m_baseHeight) * units().height();
    double minor = 0.70 * d;
    Event::setTypeGraphics(new QGraphicsEllipseItem(-d / 2, 0, d, minor));
}

TransferIn::TransferIn(const mef::Gate &event, QGraphicsItem *parent)
    : Event(event, parent)
{
    double d = int(m_size.height() - m_baseHeight) * units().height();
    Event::setTypeGraphics(
        new QGraphicsPolygonItem({{{0, 0}, {-d / 2, d}, {d / 2, d}}}));
}

const QSize Gate::m_maxSize = {6, 3};
const double Gate::m_space = 1;

Gate::Gate(const mef::Gate &event,
           std::unordered_map<const mef::Gate *, Gate *> *transfer,
           QGraphicsItem *parent)
    : Event(event, parent)
{
    double availableHeight
        = m_size.height() - m_baseHeight - m_maxSize.height();
    auto *pathItem = new QGraphicsLineItem(
        0, 0, 0, (availableHeight - 1) * units().height(), this);
    pathItem->setPos(0, (m_baseHeight + m_maxSize.height()) * units().height());
    Event::setTypeGraphics(
        getGateGraphicsType(event.formula().type()).release());
    struct {
        Event *operator()(const mef::BasicEvent *arg)
        {
            if (arg->HasAttribute("flavor")) {
                const mef::Attribute &flavor = arg->GetAttribute("flavor");
                if (flavor.value == "undeveloped")
                    return new UndevelopedEvent(*arg, m_parent);
                if (flavor.value == "conditional")
                    return new ConditionalEvent(*arg, m_parent);
            }
            return new BasicEvent(*arg, m_parent);
        }
        Event *operator()(const mef::HouseEvent *arg)
        {
            return new HouseEvent(*arg, m_parent);
        }
        Event *operator()(const mef::Gate *arg)
        {
            if (auto it = ext::find(*m_transfer, arg)) {
                it->second->addTransferOut();
                return new TransferIn(*arg, m_parent);
            }
            auto *arg_gate = new Gate(*arg, m_transfer, m_parent);
            m_transfer->emplace(arg, arg_gate);
            return arg_gate;
        }

        QGraphicsItem *m_parent;
        decltype(transfer) m_transfer;
    } formula_visitor{this, transfer};
    double linkY = (m_size.height() - 1) * units().height();
    std::vector<std::pair<Event *, QGraphicsLineItem *>> children;
    for (const mef::Formula::EventArg &eventArg :
         event.formula().event_args()) {
        auto *child = boost::apply_visitor(formula_visitor, eventArg);
        auto *link = new QGraphicsLineItem(0, 0, 0, units().height(), this);
        if (!children.empty())
            m_width += m_space * units().height();
        child->moveBy(m_width + child->width() / 2,
                      m_size.height() * units().height());
        link->moveBy(m_width + child->width() / 2, linkY);
        m_width += child->width();
        children.emplace_back(child, link);
    }
    // Shift the children left.
    for (auto &child : children) {
        child.first->moveBy(-m_width / 2, 0);
        child.second->moveBy(-m_width / 2, 0);
    }
    // Add the planar line to complete the connection.
    if (children.size() > 1)
        new QGraphicsLineItem(children.front().first->pos().x(), linkY,
                              children.back().first->pos().x(), linkY, this);
}

std::unique_ptr<QGraphicsItem> Gate::getGateGraphicsType(mef::Operator type)
{
    static_assert(mef::kNumOperators == 8, "Unexpected operator changes");
    switch (type) {
    case mef::kNull:
        return std::make_unique<QGraphicsLineItem>(
            0, 0, 0, m_maxSize.height() * units().height());
    case mef::kAnd: {
        double h = m_maxSize.height() * units().height();
        if (m_event.HasAttribute("flavor")
            && m_event.GetAttribute("flavor").value == "inhibit") {
            double a = h / 2;
            double x1 = a * sqrt(3) / 2;
            return std::make_unique<QGraphicsPolygonItem>(
                QPolygonF{{{0, 0},
                           {-x1, a / 2},
                           {-x1, 1.5 * a},
                           {0, h},
                           {x1, 1.5 * a},
                           {x1, a / 2}}});
        }
        QPainterPath paintPath;
        double maxHeight = m_maxSize.height() * units().height();
        paintPath.moveTo(0, maxHeight);
        paintPath.arcTo(-h / 2, 0, h, maxHeight * 2, 0, 180);
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
    case mef::kVote: {
        double h = m_maxSize.height() * units().height();
        double a = h / sqrt(3);
        auto polygon
            = std::make_unique<QGraphicsPolygonItem>(QPolygonF{{{-a / 2, 0},
                                                                {a / 2, 0},
                                                                {a, h / 2},
                                                                {a / 2, h},
                                                                {-a / 2, h},
                                                                {-a, h / 2}}});
        const mef::Formula &formula
            = static_cast<const mef::Gate &>(m_event).formula();
        auto *text = new QGraphicsTextItem(QString::fromLatin1("%1/%2")
                                               .arg(formula.vote_number())
                                               .arg(formula.num_args()),
                                           polygon.get());
        QFont font = text->font();
        font.setPointSizeF(1.5 * font.pointSizeF());
        text->setFont(font);
        text->setPos(-text->boundingRect().width() / 2,
                     (h - text->boundingRect().height()) / 2);
        return std::move(polygon);
    }
    case mef::kNot: {
        double h = m_maxSize.height() * units().height();
        QPainterPath paintPath;
        paintPath.addEllipse(-units().height() / 2, 0, units().height(),
                             units().height());
        paintPath.moveTo(0, units().height());
        double a = h - units().height();
        paintPath.lineTo(-a / 2, h);
        paintPath.lineTo(a / 2, h);
        paintPath.closeSubpath();
        return std::make_unique<QGraphicsPathItem>(paintPath);
    }
    case mef::kXor: {
        auto orItem = getGateGraphicsType(mef::kOr);
        double x1 = m_maxSize.width() * units().width() / 2;
        double h = m_maxSize.height() * units().height();
        QPainterPath paintPath;
        paintPath.lineTo(-x1, h);
        paintPath.moveTo(x1, h);
        paintPath.lineTo(0, 0);
        new QGraphicsPathItem(paintPath, orItem.get());
        return orItem;
    }
    case mef::kNor: {
        auto orItem = getGateGraphicsType(mef::kOr);
        auto circle = std::make_unique<QGraphicsEllipseItem>(
            -units().height() / 2, 0, units().height(), units().height());
        double orHeight = orItem->boundingRect().height();
        orItem->setScale((orHeight - units().height()) / orHeight);
        orItem->setPos(0, units().height());
        orItem.release()->setParentItem(circle.get());
        return std::move(circle);
    }
    case mef::kNand: {
        auto andItem = getGateGraphicsType(mef::kAnd);
        auto circle = std::make_unique<QGraphicsEllipseItem>(
            -units().height() / 2, 0, units().height(), units().height());
        double andHeight = andItem->boundingRect().height();
        andItem->setScale((andHeight - units().height()) / andHeight);
        andItem->setPos(0, units().height());
        andItem.release()->setParentItem(circle.get());
        return std::move(circle);
    }
    }
    GUI_ASSERT(false && "Unexpected gate type", nullptr);
}

double Gate::width() const { return m_width; }

void Gate::addTransferOut()
{
    if (m_transferOut)
        return;
    m_transferOut = true;
    QPainterPath paintPath;
    double x1 = m_maxSize.width() * units().width() / 2;
    double h = units().height() * std::sqrt(3) / 2;
    paintPath.lineTo(x1 + units().height(), 0);
    paintPath.lineTo(x1 + 0.5 * units().height(), h);
    paintPath.lineTo(x1 + 1.5 * units().height(), h);
    paintPath.lineTo(x1 + units().height(), 0);
    new QGraphicsPathItem(paintPath, Event::getTypeGraphics());
}

} // namespace diagram
} // namespace gui
} // namespace scram
