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
#include <QSize>

#include "gate.h"

namespace scram {
namespace gui {

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
    const QString &getName() const { return m_name; }

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
    const QString &getDescription() { return m_description; }

    QRectF boundingRect() const final;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) final;

protected:
    /**
     * @brief Assigns an event to a presentation view.
     *
     * The graphical representation of the derived type
     * is deduced by calling getTypeGraphics.
     *
     * @tparam T  The derived class type.
     *
     * @param view  The host view.
     */
    template <class T> Event(const T &, QGraphicsView *view);

    /**
     * @brief Provides the graphical representation of the derived type.
     *
     * @tparam T  The derived class type.
     *
     * @param units  The unit height and width for drawings.
     *
     * @return Graphics item to be attached to the event graphics.
     *         nullptr if the item is undefined upon construction (default).
     *
     * @post The item is unowned and allocated on the heap
     *       in order to be owned by the base event graphics item.
     */
    template <class T>
    static QGraphicsItem *getTypeGraphics(const QSize & /*units*/)
    {
        return nullptr;
    }

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

private:
    QGraphicsView *m_view;         ///< The host view.
    QString m_name;                ///< Identifying name of the event.
    QString m_description;         ///< Description of the event.
    QGraphicsItem *m_typeGraphics; ///< The graphics of the derived type.
};

/**
 * @brief Representation of a fault tree basic event.
 */
class BasicEvent : public Event
{
public:
    /**
     * @param view  The host view.
     */
    explicit BasicEvent(QGraphicsView *view);
};

/**
 * @brief Intermediate fault events with gates.
 */
class IntermediateEvent : public Event
{
public:
    /**
     * @param view  The host view.
     */
    explicit IntermediateEvent(QGraphicsView *view);

    /**
     * @brief Sets the Boolean logic for the intermediate event inputs.
     *
     * @param gate  The logic gate of the intermediate event.
     */
    void setGate(std::unique_ptr<Gate> gate)
    {
        setTypeGraphics(gate.release());
    }

    /**
     * @return The logic gate of the intermediate event.
     */
    Gate *getGate() const { return static_cast<Gate *>(getTypeGraphics()); }
};

} // namespace gui
} // namespace scram

#endif // EVENT_H
