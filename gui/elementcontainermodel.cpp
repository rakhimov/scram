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

#include "elementcontainermodel.h"

#include "src/event.h"
#include "src/ext/variant.h"
#include "src/model.h"

#include "align.h"
#include "guiassert.h"
#include "overload.h"
#include "translate.h"

namespace scram::gui::model {

template <class T>
ElementContainerModel::ElementContainerModel(const T &container, Model *model,
                                             QObject *parent)
    : QAbstractItemModel(parent)
{
    m_elements.reserve(container.size());
    m_elementToIndex.reserve(container.size());
    for (const auto &elementPtr : container) {
        m_elementToIndex.emplace(elementPtr.get(), m_elements.size());
        m_elements.push_back(elementPtr.get());
    }
    using E = typename T::value_type::element_type;
    connect(model, OVERLOAD(Model, added, E *), this,
            &ElementContainerModel::addElement);
    connect(model, OVERLOAD(Model, removed, E *), this,
            &ElementContainerModel::removeElement);
}

void ElementContainerModel::connectElement(Element *element)
{
    connect(element, &Element::labelChanged, this, [this, element] {
        QModelIndex index =
            createIndex(getElementIndex(element), columnCount() - 1, element);
        emit dataChanged(index, index);
    });
    connect(element, &Element::idChanged, this, [this, element] {
        QModelIndex index = createIndex(getElementIndex(element), 0, element);
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
        emit dataChanged(createIndex(index, 0, lastElement),
                         createIndex(index, columnCount() - 1, lastElement));
    }
    disconnect(element, 0, this, 0);
}

int ElementContainerModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_elements.size();
}

BasicEventContainerModel::BasicEventContainerModel(Model *model,
                                                   QObject *parent)
    : ElementContainerModel(model->basicEvents(), model, parent)
{
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
    if (role == Qt::InitialSortOrderRole && section == 2)
        return Qt::DescendingOrder;
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return ElementContainerModel::headerData(section, orientation, role);

    switch (section) {
    case 0:
        return _("ID");
    case 1:
        //: The flavor type of a basic event.
        return _("Flavor");
    case 2:
        //: In PRA context, probability may be unavailability or unreliability.
        return _("Probability");
    case 3:
        return _("Label");
    }
    GUI_ASSERT(false && "unexpected header section", {});
}

QVariant BasicEventContainerModel::data(const QModelIndex &index,
                                        int role) const
{
    if (!index.isValid())
        return {};
    if (role == Qt::TextAlignmentRole && index.column() == 2)
        return ALIGN_NUMBER_IN_TABLE;
    if (role == Qt::UserRole)
        return QVariant::fromValue(index.internalPointer());
    if (role != Qt::DisplayRole)
        return {};

    auto *basicEvent = static_cast<BasicEvent *>(index.internalPointer());

    switch (index.column()) {
    case 0:
        return basicEvent->id();
    case 1:
        return BasicEvent::flavorToString(basicEvent->flavor());
    case 2:
        return basicEvent->probability<QVariant>();
    case 3:
        return basicEvent->label();
    }
    GUI_ASSERT(false && "unexpected column", {});
}

void BasicEventContainerModel::connectElement(Element *element)
{
    ElementContainerModel::connectElement(element);
    connect(static_cast<BasicEvent *>(element), &BasicEvent::flavorChanged,
            this, [this, element] {
                QModelIndex index =
                    createIndex(getElementIndex(element), 1, element);
                emit dataChanged(index, index);
            });
    connect(static_cast<BasicEvent *>(element), &BasicEvent::expressionChanged,
            this, [this, element] {
                QModelIndex index =
                    createIndex(getElementIndex(element), 2, element);
                emit dataChanged(index, index);
            });
}

HouseEventContainerModel::HouseEventContainerModel(Model *model,
                                                   QObject *parent)
    : ElementContainerModel(model->houseEvents(), model, parent)
{
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
        return _("ID");
    case 1:
        //: House event Boolean state.
        return _("State");
    case 2:
        return _("Label");
    }
    GUI_ASSERT(false && "unexpected header section", {});
}

QVariant HouseEventContainerModel::data(const QModelIndex &index,
                                        int role) const
{
    if (!index.isValid())
        return {};
    if (role == Qt::UserRole)
        return QVariant::fromValue(index.internalPointer());
    if (role != Qt::DisplayRole)
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
                QModelIndex index =
                    createIndex(getElementIndex(element), 1, element);
                emit dataChanged(index, index);
            });
}

GateContainerModel::GateContainerModel(Model *model, QObject *parent)
    : ElementContainerModel(model->gates(), model, parent)
{
    for (Element *element : elements())
        connectElement(element);
}

void GateContainerModel::connectElement(Element *element)
{
    ElementContainerModel::connectElement(element);
    connect(static_cast<Gate *>(element), &Gate::formulaChanged, this,
            [this, element] {
                int row = getElementIndex(element);
                emit dataChanged(createIndex(row, 1, element),
                                 createIndex(row, 2, element));
                /// @todo Track gate formula changes more precisely.
                beginResetModel();
                endResetModel();
            });
}

int GateContainerModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 4;
    if (parent.parent().isValid())
        return 0;
    if (parent.column() == 0)
        return 1;
    return 0;
}

int GateContainerModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return ElementContainerModel::rowCount(parent);
    if (parent.parent().isValid())
        return 0;
    if (parent.column() == 0)
        return static_cast<Gate *>(parent.internalPointer())->numArgs();
    return 0;
}

QModelIndex GateContainerModel::index(int row, int column,
                                      const QModelIndex &parent) const
{
    if (!parent.isValid())
        return ElementContainerModel::index(row, column, parent);
    GUI_ASSERT(parent.parent().isValid() == false, {});

    auto value = reinterpret_cast<std::uintptr_t>(parent.internalPointer());
    GUI_ASSERT(value && !(value & m_parentMask), {});

    return createIndex(row, column,
                       reinterpret_cast<void *>(value | m_parentMask));
}

QModelIndex GateContainerModel::parent(const QModelIndex &index) const
{
    GUI_ASSERT(index.isValid(), {});
    auto value = reinterpret_cast<std::uintptr_t>(index.internalPointer());
    GUI_ASSERT(value, {});
    if (value & m_parentMask) {
        auto *parent = reinterpret_cast<Gate *>(value & ~m_parentMask);
        return createIndex(getElementIndex(parent), 0, parent);
    }
    return {};
}

QVariant GateContainerModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return ElementContainerModel::headerData(section, orientation, role);

    switch (section) {
    case 0:
        return _("ID");
    case 1:
        //: Boolean operator of the Boolean formula.
        return _("Connective");
    case 2:
        //: The number of arguments in the Boolean formula.
        return _("Args");
    case 3:
        return _("Label");
    }
    GUI_ASSERT(false && "unexpected header section", {});
}

QVariant GateContainerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    auto value = reinterpret_cast<std::uintptr_t>(index.internalPointer());
    if (role == Qt::UserRole) {
        return QVariant::fromValue(
            value & m_parentMask ? nullptr : index.internalPointer());
    }
    if (role != Qt::DisplayRole)
        return {};

    if (value & m_parentMask) {
        auto *parent = reinterpret_cast<Gate *>(value & ~m_parentMask);
        return QString::fromStdString(
            ext::as<const mef::Event *>(parent->args().at(index.row()).event)
                ->id());
    }

    auto *gate = static_cast<Gate *>(index.internalPointer());
    switch (index.column()) {
    case 0:
        return gate->id();
    case 1:
        return gate->type<QString>();
    case 2:
        return gate->numArgs();
    case 3:
        return gate->label();
    }
    GUI_ASSERT(false && "unexpected column", {});
}

bool GateSortFilterProxyModel::filterAcceptsRow(int row,
                                                const QModelIndex &parent) const
{
    if (parent.isValid())
        return true;
    return QSortFilterProxyModel::filterAcceptsRow(row, parent);
}

bool GateSortFilterProxyModel::lessThan(const QModelIndex &lhs,
                                        const QModelIndex &rhs) const
{
    if (lhs.parent().isValid())
        return false; // Don't sort arguments.
    return QSortFilterProxyModel::lessThan(lhs, rhs);
}

} // namespace scram::gui::model
