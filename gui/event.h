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
#include <unordered_map>

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
 */
class Event : public QGraphicsItem
{
public:
    QRectF boundingRect() const final;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) final;

    /// @returns The width of the whole subgraph.
    virtual double width() const;

protected:
    /// The confining size of the Event graphics in characters.
    /// The derived event types should stay within this confinement.
    static const QSize m_size;
    /// The height of the confining space used only by the Event base class.
    static const double m_baseHeight;
    /// The length of the ID box in characters.
    /// The height of the ID box is 1 character.
    static const double m_idBoxLength;
    /// The height of the Label box in characters.
    static const double m_labelBoxHeight;

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
 * @brief Representation of a fault tree house events.
 */
class HouseEvent : public Event
{
public:
    explicit HouseEvent(const mef::HouseEvent &event,
                        QGraphicsItem *parent = nullptr);
};

/**
 * @brief Placeholder for events with a potential to become a gate.
 */
class UndevelopedEvent : public Event
{
public:
    explicit UndevelopedEvent(const mef::BasicEvent &event,
                              QGraphicsItem *parent = nullptr);
};

/**
 * @brief The event used in Inhibit gates.
 */
class ConditionalEvent : public Event
{
public:
    explicit ConditionalEvent(const mef::BasicEvent &event,
                              QGraphicsItem *parent = nullptr);
};

/**
 * An alias pointer to a gate.
 */
class TransferIn : public Event
{
public:
    explicit TransferIn(const mef::Gate &event,
                        QGraphicsItem *parent = nullptr);
};

/**
 * @brief Fault tree intermediate events or gates.
 */
class Gate : public Event
{
public:
    /**
     * @brief Constructs the graph with the transfer symbols for gates.
     *
     * @param event  The event to be converted into a graphics item.
     * @param transfer  The set of already processed gates
     *                  to be shown as transfer event.
     * @param parent  The optional parent graphics item.
     */
    Gate(const mef::Gate &event,
         std::unordered_map<const mef::Gate *, Gate *> *transfer,
         QGraphicsItem *parent = nullptr);

    std::unique_ptr<QGraphicsItem> getGateGraphicsType(mef::Operator type);

    double width() const override;

    /**
     * @brief Adds the transfer-out symbol besides the gate shape.
     */
    void addTransferOut();

private:
    static const QSize m_maxSize; ///< The constraints on type graphics.
    static const double m_space;  ///< The space between children in chars.

    double m_width = 0; ///< Assume the graph does not change its width.
    bool m_transferOut = false;  ///< The indication of the transfer-out.
};

} // namespace diagram
} // namespace gui
} // namespace scram

#endif // EVENT_H
