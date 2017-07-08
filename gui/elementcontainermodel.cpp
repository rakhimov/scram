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

/// @file elementcontainermodel.cpp

#include "elementcontainermodel.h"

#include "src/event.h"
#include "src/model.h"

#include "guiassert.h"

namespace scram {
namespace gui {
namespace model {

template <typename T>
ElementContainerModel::ElementContainerModel(const T &container,
                                             QObject *parent)
    : QAbstractTableModel(parent)
{
    m_elements.reserve(container.size());
    m_elementToIndex.reserve(container.size());
    for (const auto &elementPtr : container) {
        m_elementToIndex.emplace(elementPtr.get(), m_elements.size());
        m_elements.push_back(elementPtr.get());
    }
}

void ElementContainerModel::connectElement(Element *element)
{
    connect(element, &Element::labelChanged, this, [this, element] {
        QModelIndex index
            = createIndex(getElementIndex(element), columnCount() - 1, element);
        emit dataChanged(index, index);
    });
}

QModelIndex ElementContainerModel::index(int row, int column,
                                         const QModelIndex &parent) const
{
    GUI_ASSERT(parent.isValid() == false, {});
    return createIndex(row, column, getElement(row));
}

Element *ElementContainerModel::getElement(int index) const
{
    GUI_ASSERT(index < m_elements.size(), nullptr);
    return m_elements[index];
}

int ElementContainerModel::getElementIndex(Element *element) const
{
    auto it = m_elementToIndex.find(element);
    GUI_ASSERT(it != m_elementToIndex.end(), -1);
    return it->second;
}

void ElementContainerModel::addElement(Element *element)
{
    int index = m_elements.size();
    beginInsertRows({}, index, index);
    m_elementToIndex.emplace(element, index);
    m_elements.push_back(element);
    endInsertRows();
    connectElement(element);
}

void ElementContainerModel::removeElement(Element *element)
{
    GUI_ASSERT(m_elementToIndex.count(element), );
    int index = m_elementToIndex.find(element)->second;
    int lastIndex = m_elements.size() - 1;
    auto *lastElement = m_elements.back();
    // The following is basically a swap with the last item.
    beginRemoveRows({}, lastIndex, lastIndex);
    m_elementToIndex.erase(element);
    m_elements.pop_back();
    endRemoveRows();
    if (index != lastIndex) {
        m_elements[index] = lastElement;
        m_elementToIndex[lastElement] = index;
        emit dataChanged(createIndex(index, 0),
                         createIndex(index, columnCount()));
    }
    disconnect(element, 0, this, 0);
}

int ElementContainerModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_elements.size();
}

BasicEventContainerModel::BasicEventContainerModel(Model *model,
                                                   QObject *parent)
    : ElementContainerModel(model->basicEvents(), parent)
{
    connect(model, &Model::addedBasicEvent, this,
            &BasicEventContainerModel::addElement);
    connect(model, &Model::removedBasicEvent, this,
            &BasicEventContainerModel::removeElement);
    for (Element *element : elements())
        connectElement(element);
}

int BasicEventContainerModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
}

QVariant BasicEventContainerModel::headerData(int section,
                                              Qt::Orientation orientation,
                                              int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return ElementContainerModel::headerData(section, orientation, role);

    switch (section) {
    case 0:
        return tr("Id");
    case 1:
        return tr("Flavor");
    case 2:
        return tr("Probability");
    case 3:
        return tr("Label");
    }
    GUI_ASSERT(false && "unexpected header section", {});
}

QVariant BasicEventContainerModel::data(const QModelIndex &index,
                                        int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};
    auto *basicEvent = static_cast<BasicEvent *>(index.internalPointer());

    switch (index.column()) {
    case 0:
        return basicEvent->id();
    case 1:
        switch (basicEvent->flavor()) {
        case BasicEvent::Basic:
            return {};
        case BasicEvent::Undeveloped:
            return tr("Undeveloped");
        case BasicEvent::Conditional:
            return tr("Conditional");
        }
    case 2:
        return basicEvent->probability<QVariant>();
    case 3:
        return basicEvent->label();
    }
    GUI_ASSERT(false && "unexpected column", {});
}

HouseEventContainerModel::HouseEventContainerModel(Model *model,
                                                   QObject *parent)
    : ElementContainerModel(model->houseEvents(), parent)
{
    connect(model, &Model::addedHouseEvent, this,
            &HouseEventContainerModel::addElement);
    connect(model, &Model::removedHouseEvent, this,
            &HouseEventContainerModel::removeElement);
    for (Element *element : elements())
        connectElement(element);
}

int HouseEventContainerModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 3;
}

QVariant HouseEventContainerModel::headerData(int section,
                                              Qt::Orientation orientation,
                                              int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return ElementContainerModel::headerData(section, orientation, role);

    switch (section) {
    case 0:
        return tr("Id");
    case 1:
        return tr("State");
    case 2:
        return tr("Label");
    }
    GUI_ASSERT(false && "unexpected header section", {});
}

QVariant HouseEventContainerModel::data(const QModelIndex &index,
                                        int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};
    auto *houseEvent = static_cast<HouseEvent *>(index.internalPointer());

    switch (index.column()) {
    case 0:
        return houseEvent->id();
    case 1:
        return houseEvent->state<QString>();
    case 2:
        return houseEvent->label();
    }
    GUI_ASSERT(false && "unexpected column", {});
}

void HouseEventContainerModel::connectElement(Element *element)
{
    ElementContainerModel::connectElement(element);
    connect(static_cast<HouseEvent *>(element), &HouseEvent::stateChanged, this,
            [this, element] {
                QModelIndex index
                    = createIndex(getElementIndex(element), 1, element);
                emit dataChanged(index, index);
            });
}

} // namespace model
} // namespace gui
} // namespace scram
