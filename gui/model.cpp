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

Model::Model(mef::Model *model) : Element(model), m_model(model)
{
    m_houseEvents.reserve(m_model->house_events().size());
    for (const mef::HouseEventPtr &houseEvent : m_model->house_events())
        m_houseEvents.emplace(std::make_unique<HouseEvent>(houseEvent.get()));

    m_basicEvents.reserve(m_model->basic_events().size());
    for (const mef::BasicEventPtr &basicEvent : m_model->basic_events())
        m_basicEvents.emplace(std::make_unique<BasicEvent>(basicEvent.get()));
}

void Model::addHouseEvent(const mef::HouseEventPtr &houseEvent)
{
    m_model->Add(houseEvent);
    auto *proxy = new HouseEvent(houseEvent.get());
    m_houseEvents.emplace(proxy);
    emit addedHouseEvent(proxy);
}

void Model::addBasicEvent(const mef::BasicEventPtr &basicEvent)
{
    m_model->Add(basicEvent);
    auto *proxy = new BasicEvent(basicEvent.get());
    m_basicEvents.emplace(proxy);
    emit addedBasicEvent(proxy);
}

std::unique_ptr<HouseEvent> Model::removeHouseEvent(mef::HouseEvent *houseEvent)
{
    m_model->Remove(houseEvent);
    auto result = extract(houseEvent, &m_houseEvents);
    emit removedHouseEvent(result.get());
    return result;
}

std::unique_ptr<BasicEvent> Model::removeBasicEvent(mef::BasicEvent *basicEvent)
{
    m_model->Remove(basicEvent);
    auto result = extract(basicEvent, &m_basicEvents);
    emit removedBasicEvent(result.get());
    return result;
}

ChangeHouseEventStateCommand::ChangeHouseEventStateCommand(
    HouseEvent *houseEvent, Model *model)
    : QUndoCommand(
          QObject::tr("Change house event %1 state").arg(houseEvent->id())),
      m_model(model), m_houseEvent(static_cast<const mef::HouseEvent *>(
                          static_cast<const HouseEvent *>(houseEvent)->data()))
{
}

void ChangeHouseEventStateCommand::flip()
{
    auto it = m_model->houseEvents().find(m_houseEvent);
    GUI_ASSERT(it != m_model->houseEvents().end(), );
    (*it)->setState(!m_houseEvent->state());
}

AddHouseEventCommand::AddHouseEventCommand(mef::HouseEventPtr houseEvent,
                                           Model *model)
    : QUndoCommand(QObject::tr("Add house-event %1")
                       .arg(QString::fromStdString(houseEvent->id()))),
      m_model(model), m_houseEvent(std::move(houseEvent))
{
}

void AddHouseEventCommand::redo()
{
    m_model->addHouseEvent(m_houseEvent);
}

void AddHouseEventCommand::undo()
{
    m_model->removeHouseEvent(m_houseEvent.get());
}

AddBasicEventCommand::AddBasicEventCommand(mef::BasicEventPtr basicEvent,
                                           Model *model)
    : QUndoCommand(QObject::tr("Add basic-event %1")
                       .arg(QString::fromStdString(basicEvent->id()))),
      m_model(model), m_basicEvent(std::move(basicEvent))
{
}

void AddBasicEventCommand::redo()
{
    m_model->addBasicEvent(m_basicEvent);
}

void AddBasicEventCommand::undo()
{
    m_model->removeBasicEvent(m_basicEvent.get());
}

} // namespace model
} // namespace gui
} // namespace scram
