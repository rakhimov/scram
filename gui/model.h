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
#include <type_traits>
#include <vector>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <QObject>
#include <QString>
#include <QUndoCommand>
#include <QVariant>

#include "src/event.h"
#include "src/ext/multi_index.h"
#include "src/model.h"

namespace scram {
namespace gui {
namespace model {

/// Fault tree container element management assuming normalized model.
/// @todo Move into an appropriate proxy type.
/// @{
inline void remove(mef::Event *, mef::FaultTree *)  {}
inline void remove(mef::Gate *gate, mef::FaultTree *faultTree)
{
    faultTree->Remove(gate);
}
inline void add(mef::Event *, mef::FaultTree *)  {}
inline void add(mef::Gate *gate, mef::FaultTree *faultTree)
{
    faultTree->Add(gate);
}
/// @}

class Element : public QObject
{
    Q_OBJECT

    template <class, class>
    friend class Proxy;  // Gets access to the data.

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

    template <class T>
    class SetId : public QUndoCommand
    {
    public:
        SetId(T *event, QString name, mef::Model *model,
              mef::FaultTree *faultTree = nullptr)
            : QUndoCommand(QObject::tr("Rename event '%1' to '%2'")
                               .arg(event->id(), name)),
              m_name(std::move(name)), m_event(event), m_model(model),
              m_faultTree(faultTree)
        {
        }

        void undo() override { redo(); }
        void redo() override
        {
            QString cur_name = m_event->id();
            if (m_name == cur_name)
                return;
            if (m_faultTree)
                remove(m_event->data(), m_faultTree);
            auto ptr = m_model->Remove(m_event->data());
            m_event->data()->id(m_name.toStdString());
            if (m_faultTree)
                add(m_event->data(), m_faultTree);
            m_model->Add(std::move(ptr));
            emit m_event->idChanged(m_name);
            m_name = std::move(cur_name);
        }

    private:
        QString m_name;
        T *m_event;
        mef::Model *m_model;
        mef::FaultTree *m_faultTree;
    };

signals:
    void labelChanged(const QString &label);
    void idChanged(const QString &id);

protected:
    explicit Element(mef::Element *element) : m_data(element) {}

private:
    mef::Element *const m_data;
};

/// Provides the type and data of the origin for Proxy Elements.
///
/// @tparam E  The Element class.
/// @tparam T  The MEF class.
template <class E, class T>
class Proxy
{
public:
    using Origin = T;  ///< The MEF type.

    const T *data() const
    {
        return static_cast<const T *>(static_cast<const E *>(this)->m_data);
    }
    T *data()
    {
        return const_cast<T *>(static_cast<const Proxy *>(this)->data());
    }
};

class BasicEvent : public Element, public Proxy<BasicEvent, mef::BasicEvent>
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
        return data()->HasExpression() ? &data()->expression() : nullptr;
    }

    /// @returns The probability value of the event.
    ///
    /// @pre The basic event has expression.
    template <typename T = double>
    T probability() const { return data()->p(); }

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
    return data()->HasExpression() ? QVariant(probability()) : QVariant();
}

class HouseEvent : public Element, public Proxy<HouseEvent, mef::HouseEvent>
{
    Q_OBJECT

public:
    explicit HouseEvent(mef::HouseEvent *houseEvent) : Element(houseEvent) {}

    template <typename T = bool>
    T state() const
    {
        return data()->state();
    }

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

class Gate : public Element, public Proxy<Gate, mef::Gate>
{
    Q_OBJECT

public:
    explicit Gate(mef::Gate *gate) : Element(gate) {}

    template <typename T = mef::Operator>
    T type() const
    {
        return data()->formula().type();
    }

    int numArgs() const { return data()->formula().num_args(); }
    int voteNumber() const
    {
        return data()->formula().vote_number();
    }

    const std::vector<mef::Formula::EventArg> &args() const
    {
        return data()->formula().event_args();
    }

    /// Formula modification commands.
    class SetFormula : public QUndoCommand
    {
    public:
        SetFormula(Gate *gate, mef::FormulaPtr formula);

        void redo() override;
        void undo() override { redo(); }

    private:
        mef::FormulaPtr m_formula;
        Gate *m_gate;
    };

signals:
    void formulaChanged();
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
template <class T, class M = typename T::Origin, class P = Proxy<T, M>>
using ProxyTable = boost::multi_index_container<
    std::unique_ptr<T>, boost::multi_index::indexed_by<
           boost::multi_index::hashed_unique<boost::multi_index::const_mem_fun<
               P, const M *, &P::data>>>>;

/// The wrapper around the MEF Model.
class Model : public Element, public Proxy<Model, mef::Model>
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
    /// @returns The parent gates of an event.
    std::vector<Gate *> parents(mef::Formula::EventArg event) const;

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

    /// @todo Provide a proxy class for the fault tree.
    class AddFaultTree : public QUndoCommand
    {
    public:
        AddFaultTree(mef::FaultTreePtr faultTree, Model *model);

        void redo() override;
        void undo() override;

    protected:
        AddFaultTree(mef::FaultTree *address, Model *model, QString description)
            : QUndoCommand(std::move(description)), m_model(model),
              m_address(address)
        {
        }

    private:
        Model *m_model;
        mef::FaultTree *const m_address;
        mef::FaultTreePtr m_faultTree;
    };

    class RemoveFaultTree : public AddFaultTree
    {
    public:
        RemoveFaultTree(mef::FaultTree *faultTree, Model *model);

        void redo() override { AddFaultTree::undo(); }
        void undo() override { AddFaultTree::redo(); }
    };

    /// @tparam T  The Model event type.
    template <class T>
    class AddEvent : public QUndoCommand
    {
    public:
        AddEvent(std::unique_ptr<typename T::Origin> event, Model *model,
                 mef::FaultTree *faultTree = nullptr)
            : QUndoCommand(QObject::tr("Add event '%1'")
                               .arg(QString::fromStdString(event->id()))),
              m_model(model), m_proxy(std::make_unique<T>(event.get())),
              m_address(event.get()), m_event(std::move(event)),
              m_faultTree(faultTree)
        {
        }

    void redo() override
    {
        m_model->m_model->Add(std::move(m_event));
        auto it = m_model->table<T>().emplace(std::move(m_proxy)).first;
        emit m_model->added(it->get());

        if (m_faultTree)
            add(m_address, m_faultTree);
    }

    void undo() override
    {
        m_event = m_model->m_model->Remove(m_address);
        m_proxy = ext::extract(m_address, &m_model->table<T>());
        emit m_model->removed(m_proxy.get());

        if (m_faultTree)
            remove(m_address, m_faultTree);
    }

    protected:
        AddEvent(T *event, Model *model, mef::FaultTree *faultTree,
                 QString description)
            : QUndoCommand(std::move(description)), m_model(model),
              m_address(event->data()), m_faultTree(faultTree)
        {
        }

    private:
        Model *m_model;
        std::unique_ptr<T> m_proxy;
        typename T::Origin *const m_address;
        std::unique_ptr<typename T::Origin> m_event;
        mef::FaultTree *m_faultTree;  ///< Optional container.
    };

    /// Removes an event from the model.
    ///
    /// @tparam T  The proxy event type.
    ///
    /// @pre The event has no dependent/parent gates.
    template <class T>
    class RemoveEvent : public AddEvent<T>
    {
        static_assert(std::is_base_of<Element, T>::value, "");

    public:
        RemoveEvent(T *event, Model *model, mef::FaultTree *faultTree = nullptr)
            : AddEvent<T>(event, model, faultTree,
                          QObject::tr("Remove event '%1'").arg(event->id()))
        {
        }

        void redo() override { AddEvent<T>::undo(); }
        void undo() override { AddEvent<T>::redo(); }
    };

    /// Changes the event type.
    ///
    /// @tparam E  The type of the existing Model Event.
    /// @tparam T  The type of the new (target) Event.
    template <class E, class T>
    class ChangeEventType : public QUndoCommand
    {
        static_assert(!std::is_same<E, T>::value, "");
        static_assert(std::is_base_of<Element, E>::value, "");
        static_assert(std::is_base_of<Element, T>::value, "");

    public:
        /// Assumes that events have the same ID.
        ChangeEventType(E *currentEvent,
                        std::unique_ptr<typename T::Origin> newEvent,
                        Model *model, mef::FaultTree *faultTree = nullptr)
            : QUndoCommand(QObject::tr("Change the type of event '%1'")
                               .arg(currentEvent->id())),
              m_switchTo{currentEvent, std::make_unique<T>(newEvent.get()),
                         std::move(newEvent)},
              m_model(model), m_faultTree(faultTree),
              m_gates(model->parents(currentEvent->data()))
        {
        }

        void undo() override { m_switchTo = m_switchFrom(*this); }
        void redo() override { m_switchFrom = m_switchTo(*this); }

    private:
        template <class Current, class Next>
        struct Switch
        {
            Switch<Next, Current> operator()(const ChangeEventType &self)
            {
                std::unique_ptr<typename Current::Origin> curEvent
                    = self.m_model->m_model->Remove(m_address->data());
                std::unique_ptr<Current> curProxy
                    = ext::extract(m_address->data(),
                                   &self.m_model->template table<Current>());
                emit self.m_model->removed(m_address);
                Next *nextAddress = m_proxy.get();
                self.m_model->m_model->Add(std::move(m_event));
                self.m_model->template table<Next>().emplace(
                    std::move(m_proxy));
                emit self.m_model->added(nextAddress);
                if (self.m_faultTree) {
                    remove(m_address->data(), self.m_faultTree);
                    add(nextAddress->data(), self.m_faultTree);
                }
                for (Gate *gate : self.m_gates) {
                    gate->data()->formula().RemoveArgument(m_address->data());
                    gate->data()->formula().AddArgument(nextAddress->data());
                    emit gate->formulaChanged();
                }
                return {nextAddress, std::move(curProxy), std::move(curEvent)};
            }

            Current *m_address;
            std::unique_ptr<Next> m_proxy;
            std::unique_ptr<typename Next::Origin> m_event;
        };
        Switch<E, T> m_switchTo;
        Switch<T, E> m_switchFrom;

        Model *m_model;
        mef::FaultTree *m_faultTree;
        std::vector<Gate *> m_gates;
    };
    /// @}

signals:
    void modelNameChanged(QString name);
    void added(mef::FaultTree *faultTree);
    void added(HouseEvent *houseEvent);
    void added(BasicEvent *basicEvent);
    void added(Gate *gate);
    void removed(mef::FaultTree *faultTree);
    void removed(HouseEvent *houseEvent);
    void removed(BasicEvent *basicEvent);
    void removed(Gate *gate);

private:
    /// Normalizes the model to the GUI expectations.
    ///
    /// @post No house events or basic events in fault tree containers.
    ///
    /// @todo Remove normalization upon full container support for elements.
    void normalize(mef::Model *model);

    template <class T>
    ProxyTable<T> &table();

    mef::Model *m_model;
    ProxyTable<HouseEvent> m_houseEvents;
    ProxyTable<BasicEvent> m_basicEvents;
    ProxyTable<Gate> m_gates;
};

template <>
inline ProxyTable<Gate> &Model::table<Gate>() { return m_gates; }
template <>
inline ProxyTable<BasicEvent> &Model::table<BasicEvent>()
{
    return m_basicEvents;
}
template <>
inline ProxyTable<HouseEvent> &Model::table<HouseEvent>()
{
    return m_houseEvents;
}

} // namespace model
} // namespace gui
} // namespace scram

#endif // MODEL_H
