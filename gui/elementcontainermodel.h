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
/// The table model for elements.

#pragma once

#include <cstdint>

#include <unordered_map>
#include <vector>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "src/element.h"
#include "src/event.h"

#include "model.h"

namespace scram::gui::model {

/// The base class for models to list elements in a table.
///
/// The model contains the original element pointer for Qt::UserRole.
/// This only applies to top-level indices.
class ElementContainerModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    /// @param[in] parent  The top index (i.e., not valid).
    ///
    /// @returns The number of elements in the list as the row count.
    int rowCount(const QModelIndex &parent) const override;

protected:
    /// @tparam T  The container type of smart pointers to elements.
    ///
    /// @param[in] container  The data container.
    /// @param[in] model  The model managing the proxy Elements.
    /// @param[in,out] parent  The optional owner of this object.
    template <class T>
    explicit ElementContainerModel(const T &container, Model *model,
                                   QObject *parent = nullptr);

    /// Puts the element pointer into the index's internal pointer.
    QModelIndex index(int row, int column,
                      const QModelIndex &parent) const override;

    /// Assumes the table-layout and returns null index.
    QModelIndex parent(const QModelIndex &) const override { return {}; }

    /// @param[in] index  The top row index in this container model.
    ///
    /// @returns The element with the given index (row).
    ///
    /// @pre The index is valid.
    Element *getElement(int index) const;

    /// @param[in] element  The element in this container model.
    ///
    /// @returns The current index (row) of the element.
    ///
    /// @pre The element is in the table.
    int getElementIndex(Element *element) const;

    /// @returns The current elements in the container.
    const std::vector<Element *> &elements() const { return m_elements; }

protected:
    /// Connects of the element change signals to the table modification.
    /// The base implementation only handles signals coming from base element.
    /// The derived classes need to override this function
    /// and append more connections.
    ///
    /// @param[in] element  The element in this container model.
    virtual void connectElement(Element *element);

private:
    /// Adds an elements to the end of this container model.
    void addElement(Element *element);
    /// Removes an element from the container model.
    void removeElement(Element *element);

    std::vector<Element *> m_elements; ///< All the elements in the model.
    std::unordered_map<Element *, int> m_elementToIndex; ///< An element to row.
};

/// The proxy model allows sorting and filtering.
class SortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    /// Keep the row indices sequential.
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override
    {
        return sourceModel()->headerData(section, orientation, role);
    }
};

/// Container model for basic events.
class BasicEventContainerModel : public ElementContainerModel
{
    Q_OBJECT

public:
    using ItemModel = BasicEvent;     ///< The proxy Element type.
    using DataType = mef::BasicEvent; ///< The data Element type.

    /// Constructs from the table of proxy Basic Events in the Model.
    explicit BasicEventContainerModel(Model *model, QObject *parent = nullptr);

    /// Required standard member functions of QAbstractItemModel interface.
    /// @{
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    /// @}

private:
    void connectElement(Element *element) final;
};

/// Container model for house events.
class HouseEventContainerModel : public ElementContainerModel
{
    Q_OBJECT

public:
    using ItemModel = HouseEvent;     ///< The proxy Element type.
    using DataType = mef::HouseEvent; ///< The data Element type.

    /// Constructs from the table of proxy House Events in the Model.
    explicit HouseEventContainerModel(Model *model, QObject *parent = nullptr);

    /// Required standard member functions of QAbstractItemModel interface.
    /// @{
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    /// @}

private:
    void connectElement(Element *element) final;
};

/// Tree-view inside table.
class GateContainerModel : public ElementContainerModel
{
    Q_OBJECT

public:
    using ItemModel = Gate;     ///< The proxy Element type.
    using DataType = mef::Gate; ///< The data Element type.

    /// Constructs from the table of proxy Gates in the Model.
    explicit GateContainerModel(Model *model, QObject *parent = nullptr);

    /// The index for children embeds the parent information into the data.
    /// @{
    QModelIndex index(int row, int column,
                      const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    /// @}

private:
    static const std::uintptr_t m_parentMask = 1; ///< Tagged parent pointer.

    void connectElement(Element *element) final;
};

/// The proxy model specialized for the gate tree-table.
class GateSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

protected:
    /// Accepts only top elements for sorting and filtering.
    /// @{
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &lhs,
                  const QModelIndex &rhs) const override;
    /// @}
};

} // namespace scram::gui::model
