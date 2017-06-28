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

Model::Model(mef::Model *model, QObject *parent)
    : QObject(parent), m_model(model)
{
}

void Model::addHouseEvent(const mef::HouseEventPtr &houseEvent)
{
    m_model->Add(houseEvent);
    emit addedHouseEvent(houseEvent.get());
}

void Model::addBasicEvent(const mef::BasicEventPtr &basicEvent)
{
    m_model->Add(basicEvent);
    emit addedBasicEvent(basicEvent.get());
}

void Model::removeHouseEvent(mef::HouseEvent *houseEvent)
{
    m_model->Remove(houseEvent);
    emit removedHouseEvent(houseEvent);
}

void Model::removeBasicEvent(mef::BasicEvent *basicEvent)
{
    m_model->Remove(basicEvent);
    emit removedBasicEvent(basicEvent);
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

HouseEvent::HouseEvent(mef::HouseEvent *houseEvent) : m_houseEvent(houseEvent)
{
}

} // namespace model
} // namespace gui
} // namespace scram
