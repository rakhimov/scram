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

/// @file model.cpp

#include "model.h"

#include <boost/range/algorithm.hpp>

#include "src/fault_tree.h"

#include "guiassert.h"

namespace scram {
namespace gui {
namespace model {

Element::SetLabel::SetLabel(Element *element, QString label)
    : QUndoCommand(QObject::tr("Set element '%1' label to '%2'")
                       .arg(element->id(), label)),
      m_label(std::move(label)), m_element(element)
{
}

void Element::SetLabel::redo()
{
    QString cur_label = m_element->label();
    if (m_label == cur_label)
        return;
    m_element->m_data->label(m_label.toStdString());
    emit m_element->labelChanged(m_label);
    m_label = std::move(cur_label);
}

BasicEvent::BasicEvent(mef::BasicEvent *basicEvent)
    : Element(basicEvent), m_flavor(Flavor::Basic)
{
    if (basicEvent->HasAttribute("flavor")) {
        const mef::Attribute &flavor = basicEvent->GetAttribute("flavor");
        if (flavor.value == "undeveloped") {
            m_flavor = Flavor::Undeveloped;
        } else if (flavor.value == "conditional") {
            m_flavor = Flavor::Conditional;
        }
    }
}

BasicEvent::SetExpression::SetExpression(BasicEvent *basicEvent,
                                         mef::Expression *expression)
    : QUndoCommand(QObject::tr("Modify basic event '%1' expression")
                       .arg(basicEvent->id())),
      m_expression(expression), m_basicEvent(basicEvent)
{
}

void BasicEvent::SetExpression::redo()
{
    auto *mefEvent = m_basicEvent->data();
    mef::Expression *cur_expression
        = mefEvent->HasExpression() ? &mefEvent->expression() : nullptr;
    if (m_expression == cur_expression)
        return;
    mefEvent->expression(m_expression);
    emit m_basicEvent->expressionChanged(m_expression);
    m_expression = cur_expression;
}

BasicEvent::SetFlavor::SetFlavor(BasicEvent *basicEvent, Flavor flavor)
    : QUndoCommand(tr("Set basic event '%1' flavor to '%2'")
                       .arg(basicEvent->id(), flavorToString(flavor))),
      m_flavor(flavor), m_basicEvent(basicEvent)
{
}

void BasicEvent::SetFlavor::redo()
{
    Flavor cur_flavor = m_basicEvent->flavor();
    if (m_flavor == cur_flavor)
        return;

    mef::Element *mefEvent = m_basicEvent->data();
    switch (m_flavor) {
    case Basic:
        mefEvent->RemoveAttribute("flavor");
        break;
    case Undeveloped:
        mefEvent->SetAttribute({"flavor", "undeveloped", ""});
        break;
    case Conditional:
        mefEvent->SetAttribute({"flavor", "conditional", ""});
        break;
    }
    m_basicEvent->m_flavor = m_flavor;
    emit m_basicEvent->flavorChanged(m_flavor);
    m_flavor = cur_flavor;
}

HouseEvent::SetState::SetState(HouseEvent *houseEvent, bool state)
    : QUndoCommand(QObject::tr("Set house event '%1' state to '%2'")
                       .arg(houseEvent->id(), boolToString(state))),
      m_state(state), m_houseEvent(houseEvent)
{
}

void HouseEvent::SetState::redo()
{
    if (m_state == m_houseEvent->state())
        return;
    bool prev_state = m_houseEvent->state();
    m_houseEvent->data()->state(m_state);
    emit m_houseEvent->stateChanged(m_state);
    m_state = prev_state;
}

Gate::SetFormula::SetFormula(Gate *gate, mef::FormulaPtr formula)
    : QUndoCommand(QObject::tr("Update gate '%1' formula").arg(gate->id())),
      m_formula(std::move(formula)), m_gate(gate)
{
}

void Gate::SetFormula::redo()
{
    m_formula = m_gate->data()->formula(std::move(m_formula));
    emit m_gate->formulaChanged();
}

namespace {

template <class T, class S>
void populate(const mef::IdTable<S> &source, ProxyTable<T> *proxyTable)
{
    proxyTable->reserve(source.size());
    for (const S &element : source)
        proxyTable->emplace(std::make_unique<T>(element.get()));
}

} // namespace

Model::Model(mef::Model *model) : Element(model), m_model(model)
{
    normalize(model);
    populate<HouseEvent>(m_model->house_events(), &m_houseEvents);
    populate<BasicEvent>(m_model->basic_events(), &m_basicEvents);
    populate<Gate>(m_model->gates(), &m_gates);
}

void Model::normalize(mef::Model *model)
{
    for (const mef::FaultTreePtr &faultTree : model->fault_trees()) {
        const_cast<mef::ElementTable<mef::BasicEvent *> &>(
            faultTree->basic_events())
            .clear();
        const_cast<mef::ElementTable<mef::HouseEvent *> &>(
            faultTree->house_events())
            .clear();
    }
}

std::vector<Gate *> Model::parents(mef::Formula::EventArg event) const
{
    std::vector<Gate *> result;
    for (const std::unique_ptr<Gate> &gate : m_gates) {
        if (boost::find(gate->args(), event) != gate->args().end())
            result.push_back(gate.get());
    }
    return result;
}

Model::SetName::SetName(QString name, Model *model)
    : QUndoCommand(QObject::tr("Rename model to '%1'").arg(name)),
      m_model(model), m_name(std::move(name))
{
}

void Model::SetName::redo()
{
    QString currentName = m_model->m_model->HasDefaultName()
                              ? QStringLiteral("")
                              : m_model->id();
    if (currentName == m_name)
        return;
    m_model->m_model->SetOptionalName(m_name.toStdString());
    emit m_model->modelNameChanged(m_name);
    m_name = std::move(currentName);
}

Model::AddEvent<Gate>::AddEvent(mef::GatePtr gate, std::string faultTree,
                                Model *model)
    : QUndoCommand(
          QObject::tr("Add gate '%1'").arg(QString::fromStdString(gate->id()))),
      m_model(model), m_proxy(std::make_unique<Gate>(gate.get())),
      m_address(gate.get()), m_gate(std::move(gate)),
      m_faultTree(std::make_unique<mef::FaultTree>(faultTree)),
      m_faultTreeAddress(m_faultTree.get())
{
    m_faultTree->Add(m_address);
    m_faultTree->CollectTopEvents();
}

void Model::AddEvent<Gate>::redo()
{
    m_model->m_model->Add(std::move(m_faultTree));
    emit m_model->added(m_faultTreeAddress);

    m_model->m_model->Add(std::move(m_gate));
    auto it = m_model->m_gates.emplace(std::move(m_proxy)).first;
    emit m_model->added(it->get());
}

void Model::AddEvent<Gate>::undo()
{
    m_faultTree = m_model->m_model->Remove(m_faultTreeAddress);
    emit m_model->removed(m_faultTreeAddress);

    m_gate = m_model->m_model->Remove(m_address);
    m_proxy = ext::extract(m_address, &m_model->m_gates);
    emit m_model->removed(m_proxy.get());
}

} // namespace model
} // namespace gui
} // namespace scram
