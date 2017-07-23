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
#include <string>

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

    class SetLabel : public QUndoCommand
    {
    public:
        SetLabel(Element *element, QString label);

        void redo() override;
        void undo() override { redo(); }

    private:
        QString m_label;
        Element *m_element;
    };

    const mef::Element *data() const { return m_data; }

signals:
    void labelChanged(const QString &label);

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

    static QString flavorToString(Flavor flavor)
    {
        switch (flavor) {
        case Basic:
            return tr("Basic");
        case Undeveloped:
            return tr("Undeveloped");
        case Conditional:
            return tr("Conditional");
        }
        assert(false);
    }

    explicit BasicEvent(mef::BasicEvent *basicEvent);

    Flavor flavor() const { return m_flavor; }

    /// @returns The current expression of this basic event.
    ///          nullptr if no expression has been set.
    mef::Expression *expression() const
    {
        return data<mef::BasicEvent>()->HasExpression()
                   ? &data<mef::BasicEvent>()->expression()
                   : nullptr;
    }

    /// @returns The probability value of the event.
    ///
    /// @pre The basic event has expression.
    template <typename T = double>
    T probability() const { return data<mef::BasicEvent>()->p(); }

    /// Sets the basic event expression.
    ///
    /// @note Currently, the expression change
    ///       is detected with address comparison,
    ///       which may fail if the current expression has been changed.
    class SetExpression : public QUndoCommand
    {
    public:
        /// @param[in] basicEvent  The basic event to receive an expression.
        /// @param[in] expression  The valid expression for the basic event.
        ///                        nullptr to unset the expression.
        SetExpression(BasicEvent *basicEvent, mef::Expression *expression);

        void redo() override;
        void undo() override { redo(); }

    private:
        mef::Expression *m_expression;
        BasicEvent *m_basicEvent;
    };

    /// Sets the flavor of the basic event.
    class SetFlavor : public QUndoCommand
    {
    public:
        SetFlavor(BasicEvent *basicEvent, Flavor flavor);

        void redo() override;
        void undo() override { redo(); }

    private:
        Flavor m_flavor;
        BasicEvent *m_basicEvent;
    };

signals:
    void expressionChanged(mef::Expression *expression);
    void flavorChanged(Flavor flavor);

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
    T state() const { return data<mef::HouseEvent>()->state(); }

    /// Flips the house event state.
    class SetState : public QUndoCommand
    {
    public:
        SetState(HouseEvent *houseEvent, bool state);

        void redo() override;
        void undo() override { redo(); }

    private:
        bool m_state;
        HouseEvent *m_houseEvent;
    };

signals:
    void stateChanged(bool value);
};

/// @returns String representation for the Boolean value.
inline QString boolToString(bool value)
{
    return value ? QObject::tr("True") : QObject::tr("False");
}

/// @returns The string representation of the house event state.
template <>
inline QString HouseEvent::state<QString>() const
{
    return boolToString(state());
}

class Gate : public Element
{
    Q_OBJECT

public:
    explicit Gate(mef::Gate *gate) : Element(gate) {}

    template <typename T = mef::Operator>
    T type() const
    {
        return data<mef::Gate>()->formula().type();
    }

    int numArgs() const { return data<mef::Gate>()->formula().num_args(); }
    int voteNumber() const
    {
        return data<mef::Gate>()->formula().vote_number();
    }

    const std::vector<mef::Formula::EventArg> &args() const
    {
        return data<mef::Gate>()->formula().event_args();
    }
};

template <>
inline QString Gate::type() const
{
    switch (type()) {
    case mef::kAnd:
        return tr("and");
    case mef::kOr:
        return tr("or");
    case mef::kVote:
        return tr("at-least %1").arg(voteNumber());
    case mef::kXor:
        return tr("xor");
    case mef::kNot:
        return tr("not");
    case mef::kNull:
        return tr("null");
    case mef::kNand:
        return tr("nand");
    case mef::kNor:
        return tr("nor");
    }
    assert(false);
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
    const ProxyTable<Gate> &gates() const { return m_gates; }
    const mef::ElementTable<mef::FaultTreePtr> &faultTrees() const
    {
        return m_model->fault_trees();
    }

    /// Model manipulation commands.
    /// @{
    class SetName : public QUndoCommand
    {
    public:
        SetName(QString name, Model *model);

        void redo() override;
        void undo() override { redo(); }

    private:
        Model *m_model;
        QString m_name;
    };

    class AddHouseEvent : public QUndoCommand
    {
    public:
        AddHouseEvent(mef::HouseEventPtr houseEvent, Model *model);

        void redo() override;
        void undo() override;

    private:
        Model *m_model;
        ext::owner_ptr<HouseEvent> m_proxy;
        mef::HouseEventPtr m_houseEvent;
    };

    class AddBasicEvent : public QUndoCommand
    {
    public:
        AddBasicEvent(mef::BasicEventPtr basicEvent, Model *model);

        void redo() override;
        void undo() override;

    private:
        Model *m_model;
        ext::owner_ptr<BasicEvent> m_proxy;
        mef::BasicEventPtr m_basicEvent;
    };

    class AddGate : public QUndoCommand
    {
    public:
        /// The gate is assumed to be a fault-tree root.
        /// In other words, this is an implicit way to create a fault-tree.
        ///
        /// @param[in] gate  Fully initialized and valid gate.
        /// @param[in] faultTree  The new fault tree name.
        /// @param[in,out] model  The destination model.
        ///
        /// @todo Make fault-tree creation explicit.
        AddGate(mef::GatePtr gate, std::string faultTree, Model *model);

        void redo() override;
        void undo() override;

    private:
        Model *m_model;
        ext::owner_ptr<Gate> m_proxy;
        mef::GatePtr m_gate;
        const std::string m_faultTreeName;
    };
    /// @}

signals:
    void modelNameChanged(QString name);
    void addedHouseEvent(HouseEvent *houseEvent);
    void addedBasicEvent(BasicEvent *basicEvent);
    void addedGate(Gate *gate);
    void removedHouseEvent(HouseEvent *houseEvent);
    void removedBasicEvent(BasicEvent *basicEvent);
    void removedGate(Gate *gate);
    void addedFaultTree(mef::FaultTree *faultTree);
    void aboutToRemoveFaultTree(mef::FaultTree *faultTree);

private:
    mef::Model *m_model;
    ProxyTable<HouseEvent> m_houseEvents;
    ProxyTable<BasicEvent> m_basicEvents;
    ProxyTable<Gate> m_gates;
};

} // namespace model
} // namespace gui
} // namespace scram

#endif // MODEL_H
