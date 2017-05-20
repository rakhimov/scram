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

#include "model.h"

#include <string>

#include "src/xml.h"

namespace scram {
namespace gui {

Model::Model(std::vector<xmlpp::Node *> modelFiles, QObject *parent)
    : QAbstractItemModel(parent), m_modelFiles(std::move(modelFiles))
{
}

QVariant Model::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return {};
    if (index.parent().isValid() == false) {
        switch (index.row()) {
        case 0:
            return tr("Fault Trees");
        case 1:
            return tr("CCF Groups");
        case 2:
            return tr("Model Data");
        default:
            Q_ASSERT(false);
        }
    }
    Q_ASSERT(false);
    return {};
}

QVariant Model::headerData(int section, Qt::Orientation orientation,
                           int role) const
{
    Q_ASSERT(section == 0);
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        std::string name = m_modelFiles.empty()
                               ? ""
                               : scram::GetAttributeValue(
                                     XmlElement(m_modelFiles.front()), "name");
        return tr("Model: %1").arg(QString::fromStdString(name));
    }
    return {};
}

QModelIndex Model::index(int row, int column, const QModelIndex &parent) const
{
    Q_ASSERT(!parent.isValid());
    return createIndex(row, column);
}

QModelIndex Model::parent(const QModelIndex &index) const
{
    (void)index;
    return QModelIndex();
}

int Model::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 3; /// @todo Magic Number: The number of top-level containers.
    return 0;
}

int Model::columnCount(const QModelIndex &) const { return 1; }

} // namespace gui
} // namespace scram
