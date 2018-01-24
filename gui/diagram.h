/*
 * Copyright (C) 2016-2018 Olzhas Rakhimov
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
/// Graphics classes to draw diagrams.

#pragma once

#include <memory>
#include <unordered_map>

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QSize>

#include "src/event.h"

#include "model.h"

namespace scram::gui::diagram {

/// The base class for probabilistic events in a fault tree.
///
/// The base event item provides
/// only the boxes containing the name and description of the event.
/// A derived class must provide
/// the symbolic representation of its kind.
///
/// The sizes are measured in units of character height and average width.
/// This class provides the reference units for derived classes to use.
/// All derived class shapes should stay within the allowed box limits
/// to make the fault tree structure layered.
class Event : public QGraphicsItem
{
public:
    ~Event() noexcept override;

    /// Required standard member functions of QGraphicsItem interface.
    /// @{
    QRectF boundingRect() const final;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) final;
    /// @}

    /// @returns The width of the whole subgraph.
    virtual double width() const;

    /// @returns The model data event.
    model::Element *data() const { return m_event; }

protected:
    /// The confining size of the Event graphics in characters.
    /// The derived event types should stay within this confinement.
    static const QSizeF m_size;
    /// The height of the confining space used only by the Event base class.
    static const double m_baseHeight;
    /// The length of the ID box in characters.
    /// The height of the ID box is 1 character.
    static const double m_idBoxLength;
    /// The height of the Label box in characters.
    static const double m_labelBoxHeight;

    /// Assigns an event to a presentation view.
    ///
    /// @param event  The model event.
    /// @param parent  The parent Graphics event.
    explicit Event(model::Element *event, QGraphicsItem *parent = nullptr);

    /// @returns The graphics of the derived class.
    QGraphicsItem *getTypeGraphics() const { return m_typeGraphics; }

    /// Releases the current derived class item, and sets the new one.
    ///
    /// @param item  The new item to represent the derived type.
    void setTypeGraphics(QGraphicsItem *item);

    /// @returns Unit width (x) and height (y) for shapes.
    QSizeF units() const;

    model::Element *m_event; ///< The model data.

private:
    QGraphicsItem *m_typeGraphics; ///< The graphics of the derived type.
    QMetaObject::Connection m_labelConnection; ///< Tracks the label changes.
    QMetaObject::Connection m_idConnection;    ///< Tracks the ID changes.
};

/// Representation of a fault tree basic event.
class BasicEvent : public Event
{
public:
    /// @copydoc Event::Event
    explicit BasicEvent(model::BasicEvent *event,
                        QGraphicsItem *parent = nullptr);
};

/// Representation of a fault tree house events.
class HouseEvent : public Event
{
public:
    /// @copydoc Event::Event
    explicit HouseEvent(model::HouseEvent *event,
                        QGraphicsItem *parent = nullptr);
};

/// Placeholder for events with a potential to become a gate.
class UndevelopedEvent : public Event
{
public:
    /// @copydoc Event::Event
    explicit UndevelopedEvent(model::BasicEvent *event,
                              QGraphicsItem *parent = nullptr);
};

/// The event used in Inhibit gates.
class ConditionalEvent : public Event
{
public:
    /// @copydoc Event::Event
    explicit ConditionalEvent(model::BasicEvent *event,
                              QGraphicsItem *parent = nullptr);
};

/// An alias pointer to a gate.
class TransferIn : public Event
{
public:
    /// @copydoc Event::Event
    explicit TransferIn(model::Gate *event, QGraphicsItem *parent = nullptr);
};

/// Fault tree intermediate events or gates.
class Gate : public Event
{
public:
    /// Constructs the graph with the transfer symbols for gates.
    ///
    /// @param event  The event to be converted into a graphics item.
    /// @param model  The model with wrapper object with signals.
    /// @param transfer  The set of already processed gates
    ///                  to be shown as transfer event.
    /// @param parent  The optional parent graphics item.
    Gate(model::Gate *event, model::Model *model,
         std::unordered_map<const mef::Gate *, Gate *> *transfer,
         QGraphicsItem *parent = nullptr);

    /// Constructs graphics object representing the given gate type.
    std::unique_ptr<QGraphicsItem> getGateGraphicsType(mef::Connective type);

    double width() const override;

    /// Adds the transfer-out symbol besides the gate shape.
    void addTransferOut();

private:
    static const QSizeF m_maxSize; ///< The constraints on type graphics.
    static const double m_space;   ///< The space between children in chars.

    double m_width = 0;         ///< Assume the graph does not change its width.
    bool m_transferOut = false; ///< The indication of the transfer-out.
};

/// The scene for the fault tree diagram.
class DiagramScene : public QGraphicsScene
{
    Q_OBJECT

public:
    /// Recursively populates the scene with fault tree object graphics.
    ///
    /// @param[in] event  The root proxy gate of the fault tree.
    /// @param[in] model  The model proxy managing the data.
    /// @param[in,out] parent  The optional owner of this object.
    DiagramScene(model::Gate *event, model::Model *model,
                 QObject *parent = nullptr);
signals:
    /// @param[in] event  The event which graphics received activation.
    void activated(model::Element *event);

private:
    /// Triggers activation with mouse double click.
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

    /// Redraws the scene whenever the fault tree changes.
    ///
    /// @todo Track changes more accurately.
    void redraw();

    model::Gate *m_root;   ///< The root gate for signals and redrawing.
    model::Model *m_model; ///< The proxy model providing change signals.
};

} // namespace scram::gui::diagram
