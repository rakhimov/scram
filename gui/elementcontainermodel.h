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

/// @file elementcontainermodel.h
/// The table model for elements.

#ifndef ELEMENTCONTAINERMODEL_H
#define ELEMENTCONTAINERMODEL_H

#include <unordered_map>
#include <vector>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include "src/element.h"

#include "model.h"

namespace scram {
namespace gui {
namespace model {

/// The model to list elements in a table.
class ElementContainerModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    int rowCount(const QModelIndex &parent) const final;

protected:
    /// @tparam T  The container of smart pointers to elements.
    template <typename T>
    explicit ElementContainerModel(const T &container,
                                   QObject *parent = nullptr);

    /// @tparam T  The derived element type.
    ///
    /// @returns The element with the given index.
    ///
    /// @pre The index is valid.
    /// @pre The element type matches type stored in the container.
    template <typename T> T *getElement(int index) const;

    void addElement(mef::Element *element);
    void removeElement(mef::Element *element);

private:
    std::vector<mef::Element *> m_elements;
    std::unordered_map<mef::Element *, int> m_elementToIndex;
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

class BasicEventContainerModel : public ElementContainerModel
{
    Q_OBJECT

public:
    explicit BasicEventContainerModel(Model *model, QObject *parent = nullptr);

    int columnCount(const QModelIndex &parent) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

    QVariant data(const QModelIndex &index, int role) const override;
};

class HouseEventContainerModel : public ElementContainerModel
{
    Q_OBJECT

public:
    explicit HouseEventContainerModel(Model *model, QObject *parent = nullptr);

    int columnCount(const QModelIndex &parent) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

    QVariant data(const QModelIndex &index, int role) const override;
};

} // namespace model
} // namespace gui
} // namespace scram

#endif // ELEMENTCONTAINERMODEL_H
