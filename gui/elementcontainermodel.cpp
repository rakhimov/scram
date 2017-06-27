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

template <typename T>
T *ElementContainerModel::getElement(int index) const
{
    GUI_ASSERT(index < m_elements.size(), nullptr);
    return static_cast<T *>(m_elements[index]);
}

int ElementContainerModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_elements.size();
}

BasicEventContainerModel::BasicEventContainerModel(const mef::Model &mefModel,
                                                   QObject *parent)
    : ElementContainerModel(mefModel.basic_events(), parent)
{
}

int BasicEventContainerModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 3;
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
        return tr("Probability");
    case 2:
        return tr("Label");
    }
    GUI_ASSERT(false && "unexpected header section", {});
}

QVariant BasicEventContainerModel::data(const QModelIndex &index,
                                        int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};
    auto *basicEvent = getElement<mef::BasicEvent>(index.row());

    switch (index.column()) {
    case 0:
        return QString::fromStdString(basicEvent->id());
    case 1:
        return basicEvent->HasExpression() ? QVariant(basicEvent->p())
                                           : QVariant(tr("NULL"));
    case 2:
        return QString::fromStdString(basicEvent->label());
    }
    GUI_ASSERT(false && "unexpected column", {});
}

} // namespace model
} // namespace gui
} // namespace scram
