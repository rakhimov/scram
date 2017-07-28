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

#include "src/ext/multi_index.h"
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
    m_element->data()->label(m_label.toStdString());
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
    auto *mefEvent = m_basicEvent->data<mef::BasicEvent>();
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
    m_houseEvent->data<mef::HouseEvent>()->state(m_state);
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
    m_formula = m_gate->data<mef::Gate>()->formula(std::move(m_formula));
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
    populate<HouseEvent>(m_model->house_events(), &m_houseEvents);
    populate<BasicEvent>(m_model->basic_events(), &m_basicEvents);
    populate<Gate>(m_model->gates(), &m_gates);
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

Model::AddHouseEvent::AddHouseEvent(mef::HouseEventPtr houseEvent, Model *model)
    : QUndoCommand(QObject::tr("Add house-event '%1'")
                       .arg(QString::fromStdString(houseEvent->id()))),
      m_model(model), m_proxy(std::make_unique<HouseEvent>(houseEvent.get())),
      m_address(houseEvent.get()), m_houseEvent(std::move(houseEvent))
{
}

void Model::AddHouseEvent::redo()
{
    m_model->m_model->Add(std::move(m_houseEvent));
    auto it = m_model->m_houseEvents.emplace(std::move(m_proxy)).first;
    emit m_model->addedHouseEvent(it->get());
}

void Model::AddHouseEvent::undo()
{
    m_houseEvent = m_model->m_model->Remove(m_address);
    m_proxy = ext::extract(m_address, &m_model->m_houseEvents);
    emit m_model->removedHouseEvent(m_proxy.get());
}

Model::AddBasicEvent::AddBasicEvent(mef::BasicEventPtr basicEvent, Model *model)
    : QUndoCommand(QObject::tr("Add basic-event '%1'")
                       .arg(QString::fromStdString(basicEvent->id()))),
      m_model(model), m_proxy(std::make_unique<BasicEvent>(basicEvent.get())),
      m_address(basicEvent.get()), m_basicEvent(std::move(basicEvent))
{
}

void Model::AddBasicEvent::redo()
{
    m_model->m_model->Add(std::move(m_basicEvent));
    auto it = m_model->m_basicEvents.emplace(std::move(m_proxy)).first;
    emit m_model->addedBasicEvent(it->get());
}

void Model::AddBasicEvent::undo()
{
    m_basicEvent = m_model->m_model->Remove(m_address);
    m_proxy = ext::extract(m_address, &m_model->m_basicEvents);
    emit m_model->removedBasicEvent(m_proxy.get());
}

Model::AddGate::AddGate(mef::GatePtr gate, std::string faultTree, Model *model)
    : QUndoCommand(
          QObject::tr("Add gate '%1'").arg(QString::fromStdString(gate->id()))),
      m_model(model), m_proxy(std::make_unique<Gate>(gate.get())),
      m_address(gate.get()), m_gate(std::move(gate)),
      m_faultTreeName(std::move(faultTree))
{
}

void Model::AddGate::redo()
{
    auto faultTree = std::make_unique<mef::FaultTree>(m_faultTreeName);
    faultTree->Add(m_address);
    faultTree->CollectTopEvents();
    auto *signalPtr = faultTree.get();
    m_model->m_model->Add(std::move(faultTree));
    emit m_model->addedFaultTree(signalPtr);

    m_model->m_model->Add(std::move(m_gate));
    auto it = m_model->m_gates.emplace(std::move(m_proxy)).first;
    emit m_model->addedGate(it->get());
}

void Model::AddGate::undo()
{
    auto *faultTree
        = m_model->m_model->fault_trees().find(m_faultTreeName)->get();
    emit m_model->aboutToRemoveFaultTree(faultTree);
    m_model->m_model->Remove(faultTree);

    m_gate = m_model->m_model->Remove(m_address);
    m_proxy = ext::extract(m_address, &m_model->m_gates);
    emit m_model->removedGate(m_proxy.get());
}

} // namespace model
} // namespace gui
} // namespace scram
