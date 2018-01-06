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
/// Table model for reporting importance factors.

#pragma once

#include <vector>

#include <QAbstractTableModel>

#include "src/importance_analysis.h"

namespace scram::gui::model {

/// Table model wrapping the importance analysis result data.
///
/// @note The table does not track changes in the analysis constructs,
///       so the data can get out of date if the analysis input has changed.
class ImportanceTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /// @param[in] data  The results of the importance analysis.
    /// @param[in,out] parent  The optional owner of the object.
    ///
    /// @pre The data is alive at least as long as this model.
    ImportanceTableModel(const std::vector<core::ImportanceRecord> *data,
                         QObject *parent = nullptr);

    /// Required standard member functions of QAbstractItemModel interface.
    /// @{
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    /// @}

private:
    const std::vector<core::ImportanceRecord> &m_data; ///< The analysis result.
};

} // namespace scram::gui::model
