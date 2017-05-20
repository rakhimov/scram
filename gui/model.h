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

#ifndef MODEL_H
#define MODEL_H

#include <vector>

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include <libxml++/libxml++.h>

namespace scram {
namespace gui {

/// Hierarchical tree-like representation of the model.
class Model : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit Model(std::vector<xmlpp::Node *> modelFiles,
                   QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

private:
    std::vector<xmlpp::Node *> m_modelFiles;  ///< The root elements of files.
};

} // namespace gui
} // namespace scram

#endif // MODEL_H
