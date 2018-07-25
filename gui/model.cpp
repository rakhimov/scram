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

#include "model.h"

#include <boost/range/algorithm.hpp>

#include "src/fault_tree.h"

#include "guiassert.h"

namespace scram::gui::model {

Element::SetLabel::SetLabel(Element *element, QString label)
    : Involution(_("Set element '%1' label to '%2'").arg(element->id(), label)),
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
    if (const mef::Attribute *flavor = basicEvent->GetAttribute("flavor")) {
        if (flavor->value() == "undeveloped")
            m_flavor = Flavor::Undeveloped;
    }
}

BasicEvent::SetExpression::SetExpression(BasicEvent *basicEvent,
                                         mef::Expression *expression)
    : Involution(_("Modify basic event '%1' expression").arg(basicEvent->id())),
      m_expression(expression), m_basicEvent(basicEvent)
{
}

void BasicEvent::SetExpression::redo()
{
    auto *mefEvent = m_basicEvent->data();
    mef::Expression *cur_expression =
        mefEvent->HasExpression() ? &mefEvent->expression() : nullptr;
    if (m_expression == cur_expression)
        return;
    mefEvent->expression(m_expression);
    emit m_basicEvent->expressionChanged(m_expression);
    m_expression = cur_expression;
}

BasicEvent::SetFlavor::SetFlavor(BasicEvent *basicEvent, Flavor flavor)
    : Involution(_("Set basic event '%1' flavor to '%2'")
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
    }
    m_basicEvent->m_flavor = m_flavor;
    emit m_basicEvent->flavorChanged(m_flavor);
    m_flavor = cur_flavor;
}

HouseEvent::SetState::SetState(HouseEvent *houseEvent, bool state)
    : Involution(_("Set house event '%1' state to '%2'")
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

Gate::SetFormula::SetFormula(Gate *gate, std::unique_ptr<mef::Formula> formula)
    : Involution(_("Update gate '%1' formula").arg(gate->id())),
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
void populate(const mef::TableRange<S> &source, ProxyTable<T> *proxyTable)
{
    proxyTable->reserve(source.size());
    for (auto &element : source)
        proxyTable->emplace(std::make_unique<T>(&element));
}

} // namespace

Model::Model(mef::Model *model) : Element(model), m_model(model)
{
    populate<HouseEvent>(m_model->table<mef::HouseEvent>(), &m_houseEvents);
    populate<BasicEvent>(m_model->table<mef::BasicEvent>(), &m_basicEvents);
    populate<Gate>(m_model->table<mef::Gate>(), &m_gates);
}

std::vector<Gate *> Model::parents(mef::Formula::ArgEvent event) const
{
    std::vector<Gate *> result;
    for (const std::unique_ptr<Gate> &gate : m_gates) {
        auto it = boost::find_if(gate->args(),
                                 [&event](const mef::Formula::Arg &arg) {
                                     return arg.event == event;
                                 });

        if (it != gate->args().end())
            result.push_back(gate.get());
    }
    return result;
}

Model::SetName::SetName(QString name, Model *model)
    : Involution(_("Rename model to '%1'").arg(name)), m_model(model),
      m_name(std::move(name))
{
}

void Model::SetName::redo()
{
    QString currentName =
        m_model->m_model->HasDefaultName() ? QStringLiteral("") : m_model->id();
    if (currentName == m_name)
        return;
    m_model->m_model->SetOptionalName(m_name.toStdString());
    emit m_model->modelNameChanged(m_name);
    m_name = std::move(currentName);
}

Model::AddFaultTree::AddFaultTree(std::unique_ptr<mef::FaultTree> faultTree,
                                  Model *model)
    : QUndoCommand(_("Add fault tree '%1'")
                       .arg(QString::fromStdString(faultTree->name()))),
      m_model(model), m_address(faultTree.get()),
      m_faultTree(std::move(faultTree))
{
}

void Model::AddFaultTree::redo()
{
    m_model->m_model->Add(std::move(m_faultTree));
    emit m_model->added(m_address);
}

void Model::AddFaultTree::undo()
{
    m_faultTree = m_model->m_model->Remove(m_address);
    emit m_model->removed(m_address);
}

Model::RemoveFaultTree::RemoveFaultTree(mef::FaultTree *faultTree, Model *model)
    : Inverse<AddFaultTree>(faultTree, model,
                            _("Remove fault tree '%1'")
                                .arg(QString::fromStdString(faultTree->name())))
{
}

} // namespace scram::gui::model
