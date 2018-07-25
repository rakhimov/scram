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

#include "importancetablemodel.h"

#include "src/event.h"

#include "align.h"
#include "guiassert.h"
#include "translate.h"

namespace scram::gui::model {

ImportanceTableModel::ImportanceTableModel(
    const std::vector<core::ImportanceRecord> *data, QObject *parent)
    : QAbstractTableModel(parent), m_data(*data)
{
}

int ImportanceTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int ImportanceTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 8;
}

QVariant ImportanceTableModel::headerData(int section,
                                          Qt::Orientation orientation,
                                          int role) const
{
    if (role == Qt::InitialSortOrderRole && section != 0)
        return Qt::DescendingOrder;
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case 0:
        return _("ID");
    case 1:
        return _("Occurrence");
    case 2:
        return _("Probability");
    case 3:
        return _("MIF");
    case 4:
        return _("CIF");
    case 5:
        return _("DIF");
    case 6:
        return _("RAW");
    case 7:
        return _("RRW");
    }
    GUI_ASSERT(false && "unexpected header section", {});
}

QVariant ImportanceTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    if (role == Qt::TextAlignmentRole && index.column() != 0)
        return ALIGN_NUMBER_IN_TABLE;
    if (role != Qt::DisplayRole)
        return {};

    GUI_ASSERT(index.row() < m_data.size(), {});

    const core::ImportanceRecord &record = m_data[index.row()];

    switch (index.column()) {
    case 0:
        return QString::fromStdString(record.event.id());
    case 1:
        return record.factors.occurrence;
    case 2:
        return record.event.p();
    case 3:
        return record.factors.mif;
    case 4:
        return record.factors.cif;
    case 5:
        return record.factors.dif;
    case 6:
        return record.factors.raw;
    case 7:
        return record.factors.rrw;
    }
    GUI_ASSERT(false && "unexpected column", {});
}

} // namespace scram::gui::model
