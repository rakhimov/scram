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

#include "guiassert.h"

namespace scram {
namespace gui {
namespace model {

Element::SetLabel::SetLabel(Element *element, QString label)
    : QUndoCommand(QObject::tr("Set element %1 label to '%2'")
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

HouseEvent::SetState::SetState(HouseEvent *houseEvent, bool state)
    : QUndoCommand(QObject::tr("Set house event %1 state to %2")
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

Model::Model(mef::Model *model) : Element(model), m_model(model)
{
    m_houseEvents.reserve(m_model->house_events().size());
    for (const mef::HouseEventPtr &houseEvent : m_model->house_events())
        m_houseEvents.emplace(std::make_unique<HouseEvent>(houseEvent.get()));

    m_basicEvents.reserve(m_model->basic_events().size());
    for (const mef::BasicEventPtr &basicEvent : m_model->basic_events())
        m_basicEvents.emplace(std::make_unique<BasicEvent>(basicEvent.get()));
}

Model::AddHouseEvent::AddHouseEvent(mef::HouseEventPtr houseEvent, Model *model)
    : QUndoCommand(QObject::tr("Add house-event %1")
                       .arg(QString::fromStdString(houseEvent->id()))),
      m_model(model), m_proxy(std::make_unique<HouseEvent>(houseEvent.get())),
      m_houseEvent(std::move(houseEvent))
{
}

void Model::AddHouseEvent::redo()
{
    m_model->m_model->Add(m_houseEvent);
    m_model->m_houseEvents.emplace(std::move(m_proxy));
    emit m_model->addedHouseEvent(m_proxy.get());
}

void Model::AddHouseEvent::undo()
{
    m_model->m_model->Remove(m_houseEvent.get());
    m_proxy = extract(m_houseEvent.get(), &m_model->m_houseEvents);
    emit m_model->removedHouseEvent(m_proxy.get());
}

Model::AddBasicEvent::AddBasicEvent(mef::BasicEventPtr basicEvent, Model *model)
    : QUndoCommand(QObject::tr("Add basic-event %1")
                       .arg(QString::fromStdString(basicEvent->id()))),
      m_model(model), m_proxy(std::make_unique<BasicEvent>(basicEvent.get())),
      m_basicEvent(std::move(basicEvent))
{
}

void Model::AddBasicEvent::redo()
{
    m_model->m_model->Add(m_basicEvent);
    m_model->m_basicEvents.emplace(std::move(m_proxy));
    emit m_model->addedBasicEvent(m_proxy.get());
}

void Model::AddBasicEvent::undo()
{
    m_model->m_model->Remove(m_basicEvent.get());
    m_proxy = extract(m_basicEvent.get(), &m_model->m_basicEvents);
    emit m_model->removedBasicEvent(m_proxy.get());
}

} // namespace model
} // namespace gui
} // namespace scram
