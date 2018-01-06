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
/// The main tree representation of the Model.

#pragma once

#include <QAbstractItemModel>

#include <boost/container/flat_set.hpp>

#include "src/element.h"
#include "src/fault_tree.h"

#include "model.h"

namespace scram::gui {

/// The tree representation for the Model constructs.
class ModelTree : public QAbstractItemModel
{
    Q_OBJECT

public:
    /// The top row containers of the tree.
    enum class Row { FaultTrees, Gates, BasicEvents, HouseEvents };

    /// Constructs with the proxy Model as the data.
    explicit ModelTree(model::Model *model, QObject *parent = nullptr);

    /// Required standard member functions of QAbstractItemModel interface.
    /// @{
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    /// @}

    /// Provides the data for the tree items.
    ///
    /// @param[in] index  The index in this model.
    /// @param[in] role  The role of the data.
    ///
    /// @returns The index data for the given role.
    ///
    /// @note The Qt::UserRole provides the pointer
    ///       to the original model element if applicable.
    QVariant data(const QModelIndex &index, int role) const override;

private:
    /// The comparator to sort elements by their names.
    struct NameComparator
    {
        /// Compares element names lexicographically.
        bool operator()(const mef::Element *lhs, const mef::Element *rhs) const
        {
            return lhs->name() < rhs->name();
        }
    };

    /// Sets up count tracking connections for model element table changes.
    ///
    /// @tparam T  The element type in Model::added and Model::removed signals.
    /// @tparam Row  The corresponding top row for the count data.
    template <class T, Row R>
    void setupElementCountConnections();

    model::Model *m_model; ///< The proxy Model managing the data.

    /// The set of fault trees.
    boost::container::flat_set<mef::FaultTree *, NameComparator> m_faultTrees;
};

} // namespace scram::gui
