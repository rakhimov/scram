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

#ifndef EVENT_H
#define EVENT_H

#include <memory>

#include <QGraphicsItem>
#include <QGraphicsView>

#include "gate.h"

namespace scram {
namespace gui {

/**
 * @brief The base class for probabilistic events in a fault tree.
 */
class Event : public QGraphicsItem
{
public:
    /**
     * @brief Assigns an event to a presentation view.
     *
     * @param view  The host view.
     */
    explicit Event(QGraphicsView *view);

    /**
     * @brief Assigns the short name or ID for the event.
     *
     * @param name  Identifying string name for the event.
     */
    void setName(QString name) { m_name = std::move(name); }

    /**
     * @return Identifying name string.
     *         If the name has not been set,
     *         the string is empty.
     */
    const QString& getName() const { return m_name; }

    /**
     * @brief Adds description to the event.
     *
     * @param desc  Information about the event.
     *              Empty string for events without descriptions.
     */
    void setDescription(QString desc) { m_description = std::move(desc); }

    /**
     * @return Description of the event.
     *         Empty string if no description is provided.
     */
    const QString& getDescription() { return m_description; }

    QRectF boundingRect() const final;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

protected:
    /**
     * @return Unit width (x) and height (y) for shapes.
     */
    QSize units() const;

private:
    QGraphicsView *m_view; ///< The host view.
    QString m_name;        ///< Identifying name of the event.
    QString m_description; ///< Description of the event.
};

/**
 * @brief Representation of a fault tree basic event.
 */
class BasicEvent : public Event
{
public:
    using Event::Event;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;
};

/**
 * @brief Intermediate fault events with gates.
 */
class IntermediateEvent : public Event
{
public:
    using Event::Event;

    /**
     * @brief Sets the Boolean logic for the intermediate event inputs.
     *
     * @param gate  The logic gate of the intermediate event.
     */
    void setGate(std::unique_ptr<Gate> gate) { m_gate = std::move(gate); }

    /**
     * @return The logic gate of the intermediate event.
     */
    Gate* getGate() const { return m_gate.get(); }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

private:
    std::unique_ptr<Gate> m_gate; ///< The logic of the intermediate event.
};

} // namespace gui
} // namespace scram

#endif // EVENT_H
