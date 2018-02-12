/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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
/// Wrapper Model classes for the MEF data.

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>

#include <QObject>
#include <QString>
#include <QUndoCommand>
#include <QVariant>

#include "src/event.h"
#include "src/ext/multi_index.h"
#include "src/model.h"

#include "command.h"

namespace scram::gui::model {

/// Fault tree container element management assuming normalized model.
/// @{
inline void remove(mef::Event *, mef::FaultTree *)
{
}
inline void remove(mef::Gate *gate, mef::FaultTree *faultTree)
{
    faultTree->Remove(gate);
}
inline void add(mef::Event *, mef::FaultTree *)
{
}
inline void add(mef::Gate *gate, mef::FaultTree *faultTree)
{
    faultTree->Add(gate);
}
/// @}

/// The base proxy Element model for mef::Element.
class Element : public QObject
{
    Q_OBJECT

    template <class, class>
    friend class Proxy; // Gets access to the data.

public:
    /// @returns A unique ID string for element within the element type-group.
    ///
    /// @pre The element is public.
    QString id() const { return QString::fromStdString(m_data->name()); }

    /// @returns The additional description for the element.
    QString label() const { return QString::fromStdString(m_data->label()); }

    /// Sets the label of an Element.
    class SetLabel : public Involution
    {
    public:
        /// Stores an element and its new label.
        SetLabel(Element *element, QString label);

        void redo() override; ///< Applies changes.

    private:
        QString m_label;    ///< The label to be applied.
        Element *m_element; ///< The target element.
    };

    /// Sets the name of an Element.
    ///
    /// @tparam T  The proxy type.
    ///
    /// @pre The name format is valid for the MEF Elements.
    /// @pre The name does not belong to another element of the same type.
    ///
    /// @todo Generalize for non-Event types.
    template <class T>
    class SetId : public Involution
    {
    public:
        /// Stores an element, its new name and parent containers.
        SetId(T *event, QString name, mef::Model *model,
              mef::FaultTree *faultTree = nullptr)
            : Involution(QObject::tr("Rename event '%1' to '%2'")
                             .arg(event->id(), name)),
              m_name(std::move(name)), m_event(event), m_model(model),
              m_faultTree(faultTree)
        {
        }

        /// Applies the new name to the element.
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
        QString m_name;              ///< The name string to be applied.
        T *m_event;                  ///< The proxy element.
        mef::Model *m_model;         ///< The top model container.
        mef::FaultTree *m_faultTree; ///< The parent fault tree container.
    };

signals:
    /// @param[in] label  The new label of the element.
    void labelChanged(const QString &label);

    /// @param[in] id  The new ID of the element.
    void idChanged(const QString &id);

protected:
    /// Stores the reference to the original MEF Element to manage.
    explicit Element(mef::Element *element) : m_data(element) {}

private:
    mef::Element *const m_data; ///< The MEF element in the MEF model.
};

/// Provides the type and data of the origin for Proxy Elements.
///
/// @tparam E  The Element class.
/// @tparam T  The MEF class.
template <class E, class T>
class Proxy
{
public:
    using Origin = T; ///< The MEF type.

    /// @returns The original data managed by the proxy.
    /// @{
    const T *data() const
    {
        return static_cast<const T *>(static_cast<const E *>(this)->m_data);
    }
    T *data()
    {
        return const_cast<T *>(static_cast<const Proxy *>(this)->data());
    }
    /// @}
};

/// The proxy to manage mef::BasicEvent.
class BasicEvent : public Element, public Proxy<BasicEvent, mef::BasicEvent>
{
    Q_OBJECT

public:
    /// Basic event flavors.
    enum Flavor { Basic = 0, Undeveloped };

    /// Converts a basic event flavor to a UI string.
    static QString flavorToString(Flavor flavor)
    {
        switch (flavor) {
        case Basic:
            return tr("Basic");
        case Undeveloped:
            return tr("Undeveloped");
        }
        assert(false);
    }

    /// Initializes proxy with the MEF basic event and its implicit flavor.
    explicit BasicEvent(mef::BasicEvent *basicEvent);

    /// @returns The flavor of the basic event.
    Flavor flavor() const { return m_flavor; }

    /// @returns The current expression of this basic event.
    ///          nullptr if no expression has been set.
    mef::Expression *expression() const
    {
        return data()->HasExpression() ? &data()->expression() : nullptr;
    }

    /// @returns The probability value of the event.
    ///
    /// @pre The basic event has expression or the type has null state.
    template <typename T = double>
    T probability() const
    {
        if constexpr (std::is_same_v<T, QVariant>) {
            return data()->HasExpression() ? QVariant(data()->p()) : QVariant();

        } else {
            return data()->p();
        }
    }

    /// Sets the basic event expression.
    ///
    /// @pre The expression is valid for mef::BasicEvent.
    ///
    /// @note Currently, the expression change
    ///       is detected with address comparison,
    ///       which may fail if the current expression has been changed.
    class SetExpression : public Involution
    {
    public:
        /// @param[in] basicEvent  The basic event to receive an expression.
        /// @param[in] expression  The valid expression for the basic event.
        ///                        nullptr to unset the expression.
        SetExpression(BasicEvent *basicEvent, mef::Expression *expression);

        void redo() override; ///< Applies the expression changes.

    private:
        mef::Expression *m_expression; ///< The valid expression to apply.
        BasicEvent *m_basicEvent;      ///< The receiver basic event.
    };

    /// Sets the flavor of the basic event.
    class SetFlavor : public Involution
    {
    public:
        /// Stores the basic event and its new flavor.
        SetFlavor(BasicEvent *basicEvent, Flavor flavor);

        void redo() override; ///< Applies the flavor changes.

    private:
        Flavor m_flavor;          ///< The basic event flavor.
        BasicEvent *m_basicEvent; ///< The target basic event.
    };

signals:
    /// @param[in] expression  The new expression of the basic event.
    void expressionChanged(mef::Expression *expression);

    /// @param[in] flavor  The new flavor of the basic event.
    void flavorChanged(Flavor flavor);

private:
    Flavor m_flavor; ///< The current flavor of the basic event.
};

/// Converts Boolean value to a UI string.
inline QString boolToString(bool value)
{
    return value ? QObject::tr("True") : QObject::tr("False");
}

/// The proxy to manage mef::HouseEvent.
class HouseEvent : public Element, public Proxy<HouseEvent, mef::HouseEvent>
{
    Q_OBJECT

public:
    /// @param[in,out] houseEvent  The MEF house event.
    explicit HouseEvent(mef::HouseEvent *houseEvent) : Element(houseEvent) {}

    /// @returns The state data of the house event.
    template <typename T = bool>
    T state() const
    {
        if constexpr (std::is_same_v<T, QString>) {
            return boolToString(state());

        } else {
            return data()->state();
        }
    }

    /// Flips the house event state.
    class SetState : public Involution
    {
    public:
        /// Stores the house event and its new state.
        SetState(HouseEvent *houseEvent, bool state);

        void redo() override; ///< Applies the new state to the house event.

    private:
        bool m_state;             ///< The new state.
        HouseEvent *m_houseEvent; ///< The target house event.
    };

signals:
    /// @param[in] value  The value of the house event's new state.
    void stateChanged(bool value);
};

/// The proxy to manage mef::Gate.
///
/// @pre The gate formula is flat.
class Gate : public Element, public Proxy<Gate, mef::Gate>
{
    Q_OBJECT

public:
    /// @param[in,out] gate  The MEF gate with a flat formula.
    explicit Gate(mef::Gate *gate) : Element(gate) {}

    /// @returns The current connective of the gate.
    template <typename T = mef::Connective>
    T type() const
    {
        if constexpr (std::is_same_v<T, QString>) {
            switch (type()) {
            case mef::kAnd:
                return tr("and");
            case mef::kOr:
                return tr("or");
            case mef::kAtleast:
                //: Also named as 'vote', 'voting or', 'combination', 'combo'.
                return tr("at-least %1").arg(*minNumber());
            case mef::kXor:
                return tr("xor");
            case mef::kNot:
                return tr("not");
            case mef::kNull:
                //: This is 'pass-through' or 'no-action' gate type.
                return tr("null");
            case mef::kNand:
                //: not and.
                return tr("nand");
            case mef::kNor:
                //: not or.
                return tr("nor");
            default:
                assert(false && "Unsupported connectives.");
            }

        } else {
            return data()->formula().connective();
        }
    }

    /// @returns The number of gate arguments.
    int numArgs() const { return args().size(); }

    /// @returns The min number of the gate formula.
    std::optional<int> minNumber() const
    {
        return data()->formula().min_number();
    }

    /// @returns Event arguments of the gate.
    const std::vector<mef::Formula::Arg> &args() const
    {
        return data()->formula().args();
    }

    /// Formula modification commands.
    ///
    /// @pre The formula is valid for mef::Gate.
    class SetFormula : public Involution
    {
    public:
        /// Stores the gate and its new formula.
        SetFormula(Gate *gate, std::unique_ptr<mef::Formula> formula);

        void redo() override; ///< Applies the gate formula changes.

    private:
        std::unique_ptr<mef::Formula> m_formula; ///< The new formula.
        Gate *m_gate; ///< The receiver gate for the formula.
    };

signals:
    /// Indicates gate formula changes or resets.
    void formulaChanged();
};

/// Table of proxy elements uniquely wrapping the core model element.
///
/// @tparam T  The proxy type.
template <class T, class M = typename T::Origin, class P = Proxy<T, M>>
using ProxyTable = boost::multi_index_container<
    std::unique_ptr<T>,
    boost::multi_index::indexed_by<boost::multi_index::hashed_unique<
        boost::multi_index::const_mem_fun<P, const M *, &P::data>>>>;

/// The wrapper around the MEF Model.
class Model : public Element, public Proxy<Model, mef::Model>
{
    Q_OBJECT

public:
    /// @param[in] model  The analysis model with all constructs.
    explicit Model(mef::Model *model);

    /// The proxy element tables of the model.
    /// @{
    const ProxyTable<HouseEvent> &houseEvents() const { return m_houseEvents; }
    const ProxyTable<BasicEvent> &basicEvents() const { return m_basicEvents; }
    const ProxyTable<Gate> &gates() const { return m_gates; }
    auto faultTrees() const { return m_model->fault_trees(); }
    auto faultTrees() { return m_model->table<mef::FaultTree>(); }
    /// @}

    /// Generic access to event tables.
    template <class T>
    ProxyTable<T> &table()
    {
        if constexpr (std::is_same_v<T, Gate>) {
            return m_gates;

        } else if constexpr (std::is_same_v<T, BasicEvent>) {
            return m_basicEvents;

        } else {
            static_assert(std::is_same_v<T, HouseEvent>, "Unknown type.");
            return m_houseEvents;
        }
    }

    /// @param[in] event  The event defined/registered in the model.
    ///
    /// @returns The parent gates of an event.
    std::vector<Gate *> parents(mef::Formula::ArgEvent event) const;

    /// Sets the optional name of the model.
    ///
    /// @pre The name format is valid for mef::Model.
    ///
    /// @note Empty name string resets the model name to a default one.
    class SetName : public Involution
    {
    public:
        /// Stores the model and its new name.
        SetName(QString name, Model *model);

        void redo() override; ///< Applies the new name to the model.

    private:
        Model *m_model; ///< The current model.
        QString m_name; ///< The new name string for the model.
    };

    /// Adds a fault tree into a model.
    ///
    /// @pre The fault tree is not a duplicate of any existing fault tree.
    ///
    /// @todo Provide a proxy class for the fault tree.
    class AddFaultTree : public QUndoCommand
    {
    public:
        /// Stores the new fault tree and the target model.
        AddFaultTree(std::unique_ptr<mef::FaultTree> faultTree, Model *model);

        void redo() override; ///< Adds the fault tree.
        void undo() override; ///< Removes the fault tree.

    protected:
        /// Sets up the removal state.
        AddFaultTree(mef::FaultTree *address, Model *model, QString description)
            : QUndoCommand(std::move(description)), m_model(model),
              m_address(address)
        {
        }

    private:
        Model *m_model; ///< The model for the fault tree addition.
        mef::FaultTree *const m_address; ///< The data MEF fault tree.
        std::unique_ptr<mef::FaultTree> m_faultTree; ///< The lifetime.
    };

    /// Removes a fault tree from the model.
    class RemoveFaultTree : public Inverse<AddFaultTree>
    {
    public:
        /// Stores the model and existing fault tree for removal.
        RemoveFaultTree(mef::FaultTree *faultTree, Model *model);
    };

    /// Adds an event to the model.
    ///
    /// @tparam T  The proxy event type.
    ///
    /// @pre The event is not a duplicate of any existing event.
    ///
    /// @todo Generalize for all element types.
    template <class T>
    class AddEvent : public QUndoCommand
    {
    public:
        /// Stores the newly defined event and its destination container.
        AddEvent(std::unique_ptr<typename T::Origin> event, Model *model,
                 mef::FaultTree *faultTree = nullptr)
            : QUndoCommand(QObject::tr("Add event '%1'")
                               .arg(QString::fromStdString(event->id()))),
              m_model(model), m_proxy(std::make_unique<T>(event.get())),
              m_address(event.get()), m_event(std::move(event)),
              m_faultTree(faultTree)
        {
        }

        /// Adds the event to the containers.
        void redo() override
        {
            m_model->m_model->Add(std::move(m_event));
            auto it = m_model->table<T>().emplace(std::move(m_proxy)).first;
            emit m_model->added(it->get());

            if (m_faultTree)
                add(m_address, m_faultTree);
        }

        /// Removes the event from the containers.
        void undo() override
        {
            m_event = m_model->m_model->Remove(m_address);
            m_proxy = ext::extract(m_address, &m_model->table<T>());
            emit m_model->removed(m_proxy.get());

            if (m_faultTree)
                remove(m_address, m_faultTree);
        }

    protected:
        /// Sets up the removal state.
        AddEvent(T *event, Model *model, mef::FaultTree *faultTree,
                 QString description)
            : QUndoCommand(std::move(description)), m_model(model),
              m_address(event->data()), m_faultTree(faultTree)
        {
        }

    private:
        Model *m_model;             ///< The top container for the event.
        std::unique_ptr<T> m_proxy; ///< The proxy managing the event data.
        typename T::Origin *const m_address;         ///< The MEF data.
        std::unique_ptr<typename T::Origin> m_event; ///< The MEF event.
        mef::FaultTree *m_faultTree;                 ///< Optional container.
    };

    /// Removes an existing event from the model.
    ///
    /// @tparam T  The proxy event type.
    ///
    /// @pre The event has no dependent/parent gates.
    template <class T>
    class RemoveEvent : public Inverse<AddEvent<T>>
    {
        static_assert(std::is_base_of_v<Element, T>);

    public:
        /// Stores model containers and the existing event for removal.
        RemoveEvent(T *event, Model *model, mef::FaultTree *faultTree = nullptr)
            : Inverse<AddEvent<T>>(
                  event, model, faultTree,
                  QObject::tr("Remove event '%1'").arg(event->id()))
        {
        }
    };

    /// Changes the event type.
    ///
    /// @tparam E  The type of the existing Model Event.
    /// @tparam T  The type of the new (target) Event.
    template <class E, class T>
    class ChangeEventType : public QUndoCommand
    {
        static_assert(!std::is_same_v<E, T>);
        static_assert(std::is_base_of_v<Element, E>);
        static_assert(std::is_base_of_v<Element, T>);

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

        /// Switches event type to the target one.
        void redo() override { m_switchFrom = m_switchTo(*this); }

        /// Switches back the event type to the original one.
        void undo() override { m_switchTo = m_switchFrom(*this); }

    private:
        /// Switches event type from the current one to the next one.
        template <class Current, class Next>
        struct Switch
        {
            /// @param[in] self  The provider of access to the model data.
            ///
            /// @returns The reverse operation to switch types to the origin.
            Switch<Next, Current> operator()(const ChangeEventType &self)
            {
                std::unique_ptr<typename Current::Origin> curEvent =
                    self.m_model->m_model->Remove(m_address->data());
                std::unique_ptr<Current> curProxy =
                    ext::extract(m_address->data(),
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
                for (Gate *gate : self.m_gates)
                    gate->data()->formula().Swap(m_address->data(),
                                                 nextAddress->data());

                for (Gate *gate : self.m_gates)
                    emit gate->formulaChanged();

                return {nextAddress, std::move(curProxy), std::move(curEvent)};
            }

            Current *m_address;            ///< The proxy in the model.
            std::unique_ptr<Next> m_proxy; ///< The substitute target proxy.
            std::unique_ptr<typename Next::Origin> m_event; ///< The target.
        };

        Switch<E, T> m_switchTo;   ///< The forward switch.
        Switch<T, E> m_switchFrom; ///< The backward switch.

        Model *m_model;              ///< The proxy to manage the model.
        mef::FaultTree *m_faultTree; ///< The optional fault tree container.
        std::vector<Gate *> m_gates; ///< The parent gates of the event.
    };

signals:
    /// @param[in] name  The new name of the model.
    void modelNameChanged(QString name);

    /// Signals the addition of new elements into the model.
    /// @{
    void added(mef::FaultTree *faultTree);
    void added(HouseEvent *houseEvent);
    void added(BasicEvent *basicEvent);
    void added(Gate *gate);
    /// @}

    /// Signals the removal of existing elements of the model.
    /// @{
    void removed(mef::FaultTree *faultTree);
    void removed(HouseEvent *houseEvent);
    void removed(BasicEvent *basicEvent);
    void removed(Gate *gate);
    /// @}

private:
    mef::Model *m_model; ///< The MEF model with data.

    /// Proxy element tables.
    /// @{
    ProxyTable<HouseEvent> m_houseEvents;
    ProxyTable<BasicEvent> m_basicEvents;
    ProxyTable<Gate> m_gates;
    /// @}
};

} // namespace scram::gui::model
