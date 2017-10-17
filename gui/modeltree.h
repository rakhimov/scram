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

/// @file modeltree.h
/// The main tree representation of the Model.

#ifndef MODELTREE_H
#define MODELTREE_H

#include <QAbstractItemModel>

#include <boost/container/flat_set.hpp>

#include "src/element.h"
#include "src/fault_tree.h"

#include "model.h"

namespace scram {
namespace gui {

class ModelTree : public QAbstractItemModel
{
    Q_OBJECT

public:
    /// The top row containers of the tree.
    enum class Row {
        FaultTrees,
        Gates,
        BasicEvents,
        HouseEvents
    };

    explicit ModelTree(model::Model *model, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    struct NameComparator {
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

    model::Model *m_model;
    boost::container::flat_set<mef::FaultTree *, NameComparator> m_faultTrees;
};

} // namespace gui
} // namespace scram

#endif // MODELTREE_H
