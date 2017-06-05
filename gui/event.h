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

#ifndef EVENT_H
#define EVENT_H

#include <memory>

#include <QGraphicsItem>
#include <QSize>

#include "src/event.h"

namespace scram {
namespace gui {
namespace diagram {

/**
 * @brief The base class for probabilistic events in a fault tree.
 *
 * The base event item provides
 * only the boxes containing the name and description of the event.
 * A derived class must provide
 * the symbolic representation of its kind.
 *
 * The sizes are measured in units of character height and average width.
 * This class provides the reference units for derived classes to use.
 * All derived class shapes should stay within the allowed box limits
 * to make the fault tree structure layered.
 *
 * The derived classes must confine themselves in (10 width x 10 width) box.
 */
class Event : public QGraphicsItem
{
public:
    QRectF boundingRect() const final;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) final;

protected:
    /**
     * @brief Assigns an event to a presentation view.
     *
     * @param event  The data event.
     * @param parent  The parent Graphics event.
     */
    explicit Event(const mef::Event &event, QGraphicsItem *parent = nullptr);

    /**
     * @return The graphics of the derived class.
     */
    QGraphicsItem *getTypeGraphics() const { return m_typeGraphics; }

    /**
     * @brief Releases the current derived class item,
     *        and sets the new one.
     *
     * @param item  The new item to represent the derived type.
     */
    void setTypeGraphics(QGraphicsItem *item);

    /**
     * @return Unit width (x) and height (y) for shapes.
     */
    QSize units() const;

protected:
    const mef::Event &m_event; ///< The data.

private:
    QGraphicsItem *m_typeGraphics; ///< The graphics of the derived type.
};

/**
 * @brief Representation of a fault tree basic event.
 */
class BasicEvent : public Event
{
public:
    explicit BasicEvent(const mef::BasicEvent &event,
                        QGraphicsItem *parent = nullptr);
};

/**
 * @brief The Connective class provides the shape of the gate logic.
 */
class Connective : public QGraphicsItem
{
    using QGraphicsItem::QGraphicsItem;
};

/**
 * @brief Fault tree intermediate events or gates.
 */
class Gate : public Event
{
public:
    explicit Gate(const mef::Gate &event, QGraphicsItem *parent = nullptr);

    /**
     * @brief Sets the Boolean logic for the intermediate event inputs.
     *
     * @param connective  The logic connective of the gate.
     */
    void setConnective(std::unique_ptr<Connective> connective)
    {
        setTypeGraphics(connective.release());
    }

    /**
     * @return The logic of the gate.
     */
    Connective *getConnective() const
    {
        return static_cast<Connective *>(getTypeGraphics());
    }
};

} // namespace diagram
} // namespace gui
} // namespace scram

#endif // EVENT_H
