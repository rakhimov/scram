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

#include "modeltree.h"

#include "guiassert.h"

namespace scram {
namespace gui {

ModelTree::ModelTree(model::Model *model, QObject *parent)
    : QAbstractItemModel(parent), m_model(model)
{
    for (const mef::FaultTreePtr &faultTree : m_model->faultTrees())
        m_faultTrees.insert(faultTree.get());

    connect(m_model, &model::Model::addedFaultTree, this,
            [this](mef::FaultTree *faultTree) {
                auto it = m_faultTrees.lower_bound(faultTree);
                int index = m_faultTrees.index_of(it);
                beginInsertRows(
                    this->index(static_cast<int>(Row::FaultTrees), 0, {}),
                    index, index);
                m_faultTrees.insert(it, faultTree);
                endInsertRows();
            });
    connect(m_model, &model::Model::aboutToRemoveFaultTree, this,
            [this](mef::FaultTree *faultTree) {
                auto it = m_faultTrees.find(faultTree);
                GUI_ASSERT(it != m_faultTrees.end(), );
                int index = m_faultTrees.index_of(it);
                beginRemoveRows(
                    this->index(static_cast<int>(Row::FaultTrees), 0, {}),
                    index, index);
                m_faultTrees.erase(it);
                endRemoveRows();
            });
}

int ModelTree::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() == false)
        return 4;
    if (parent.parent().isValid())
        return 0;
    if (static_cast<Row>(parent.row()) == Row::FaultTrees)
        return m_faultTrees.size();
    return 0;
}

int ModelTree::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid() == false)
        return 1;
    if (parent.parent().isValid())
        return 0;
    if (static_cast<Row>(parent.row()) == Row::FaultTrees)
        return 1;
    return 0;
}

QModelIndex ModelTree::index(int row, int column,
                             const QModelIndex &parent) const
{
    if (parent.isValid() == false)
        return createIndex(row, column, nullptr);
    GUI_ASSERT(parent.parent().isValid() == false, {});
    GUI_ASSERT(static_cast<Row>(parent.row()) == Row::FaultTrees, {});
    return createIndex(row, column, *m_faultTrees.nth(row));
}

QModelIndex ModelTree::parent(const QModelIndex &index) const
{
    GUI_ASSERT(index.isValid(), {});
    if (index.internalPointer() == nullptr)
        return {};
    return createIndex(static_cast<int>(Row::FaultTrees), 0, nullptr);
}

QVariant ModelTree::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};
    if (index.parent().isValid())
        return QString::fromStdString(
            static_cast<mef::FaultTree *>(index.internalPointer())->name());
    switch (static_cast<Row>(index.row())) {
    case Row::FaultTrees:
        return tr("Fault Trees");
    case Row::Gates:
        return tr("Gates");
    case Row::BasicEvents:
        return tr("Basic Events");
    case Row::HouseEvents:
        return tr("House Events");
    }
    GUI_ASSERT(false, {});
}

} // namespace gui
} // namespace scram
