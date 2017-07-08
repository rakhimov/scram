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

/// @file model.h
/// Wrapper Model classes for the MEF data.

#ifndef MODEL_H
#define MODEL_H

#include <memory>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <QObject>
#include <QString>
#include <QUndoCommand>
#include <QVariant>

#include "src/event.h"
#include "src/model.h"
#include "src/ext/owner_ptr.h"

namespace scram {
namespace gui {
namespace model {

class Element : public QObject
{
    Q_OBJECT

public:
    /// @returns A unique ID string for element within the element type-group.
    ///
    /// @pre The element is public.
    QString id() const { return QString::fromStdString(m_data->name()); }

    /// @returns The additional description for the element.
    QString label() const { return QString::fromStdString(m_data->label()); }

    const mef::Element *data() const { return m_data; }

protected:
    explicit Element(mef::Element *element) : m_data(element) {}

    template <class T = mef::Element>
    T *data() { return static_cast<T *>(m_data); }
    template <class T = mef::Element>
    const T *data() const { return static_cast<const T *>(m_data); }

private:
    mef::Element *const m_data;
};

class BasicEvent : public Element
{
    Q_OBJECT

public:
    enum Flavor {
        Basic = 0,
        Undeveloped,
        Conditional
    };

    explicit BasicEvent(mef::BasicEvent *basicEvent);

    Flavor flavor() const { return m_flavor; }

    /// @returns The probability value of the event.
    ///
    /// @pre The basic event has expression.
    template <typename T = double>
    T probability() const { return data<mef::BasicEvent>()->p(); }

private:
    Flavor m_flavor;
};

/// @returns The optional probability value of the basic event.
template <>
inline QVariant BasicEvent::probability<QVariant>() const
{
    return data<mef::BasicEvent>()->HasExpression() ? QVariant(probability())
                                                    : QVariant();
}

class HouseEvent : public Element
{
    Q_OBJECT

public:
    explicit HouseEvent(mef::HouseEvent *houseEvent) : Element(houseEvent) {}

    template <typename T = bool>
    T state() const { return data<mef::HouseEvent>()->state(); };

    void setState(bool value) {
        if (value == state())
            return;
        data<mef::HouseEvent>()->state(value);
        emit stateChanged(value);
    }

signals:
    void stateChanged(bool value);
};

/// @returns The string representation of the house event state.
template <>
inline QString HouseEvent::state<QString>() const
{
    return state() ? tr("True") : tr("False");
}

/// Table of proxy elements uniquely wrapping the core model element.
///
/// @tparam T  The proxy type.
template <typename T>
using ProxyTable = boost::multi_index_container<
    ext::owner_ptr<T>, boost::multi_index::indexed_by<
           boost::multi_index::hashed_unique<boost::multi_index::const_mem_fun<
               Element, const mef::Element *, &Element::data>>>>;

/// Extracts a value from multi_index container with ownership.
///
/// @param[in] key  The key to lookup the value in the container.
/// @param[in,out] container  The container with the associated value.
///
/// @returns The unique pointer with the extracted value.
///
/// @pre The value for the given key exists.
/// @pre The container currently owns the value object.
template <typename T, typename K>
std::unique_ptr<T> extract(K &&key, ProxyTable<T> *container) noexcept
{
    std::unique_ptr<T> result;
    auto it = container->find(std::forward<K>(key));
    // Note that moving owner_ptr does not invalidate it.
    container->modify(
        it, [&result](ext::owner_ptr<T> &owner) { result = std::move(owner); });
    container->erase(it);
    return result;
}

/// The wrapper around the MEF Model.
class Model : public Element
{
    Q_OBJECT

public:
    /// @param[in] model  The analysis model with all constructs.
    explicit Model(mef::Model *model);

    const ProxyTable<HouseEvent> &houseEvents() const { return m_houseEvents; }
    const ProxyTable<BasicEvent> &basicEvents() const { return m_basicEvents; }

    void addHouseEvent(const mef::HouseEventPtr &houseEvent);
    void addBasicEvent(const mef::BasicEventPtr &basicEvent);
    std::unique_ptr<HouseEvent> removeHouseEvent(mef::HouseEvent *houseEvent);
    std::unique_ptr<BasicEvent> removeBasicEvent(mef::BasicEvent *basicEvent);

signals:
    void addedHouseEvent(HouseEvent *houseEvent);
    void addedBasicEvent(BasicEvent *basicEvent);
    void removedHouseEvent(HouseEvent *houseEvent);
    void removedBasicEvent(BasicEvent *basicEvent);

private:
    mef::Model *m_model;
    ProxyTable<HouseEvent> m_houseEvents;
    ProxyTable<BasicEvent> m_basicEvents;
};

/// Flips the house event state.
class ChangeHouseEventStateCommand : public QUndoCommand
{
public:
    ChangeHouseEventStateCommand(HouseEvent *houseEvent, Model *model);

    void redo() override { flip(); }
    void undo() override { flip(); }

private:
    void flip();

    Model *m_model;
    const mef::HouseEvent *m_houseEvent;
};

class AddHouseEventCommand : public QUndoCommand
{
public:
    AddHouseEventCommand(mef::HouseEventPtr houseEvent, Model *model);

    void redo() override;
    void undo() override;

private:
    Model *m_model;
    mef::HouseEventPtr m_houseEvent;
};

class AddBasicEventCommand : public QUndoCommand
{
public:
    AddBasicEventCommand(mef::BasicEventPtr basicEvent, Model *model);

    void redo() override;
    void undo() override;

private:
    Model *m_model;
    mef::BasicEventPtr m_basicEvent;
};

} // namespace model
} // namespace gui
} // namespace scram

#endif // MODEL_H
