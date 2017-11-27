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

/// @file producttablemodel.h
/// Table model for reporting products.

#ifndef PRODUCTTABLEMODEL_H
#define PRODUCTTABLEMODEL_H

#include <vector>

#include <QAbstractTableModel>

#include "src/fault_tree_analysis.h"

namespace scram {
namespace gui {
namespace model {

class ProductTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /// @param[in] products  The analysis results.
    /// @param[in] withProbability  The analysis includes the probability data.
    ProductTableModel(const core::ProductContainer &products,
                      bool withProbability, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    struct Product
    {
        QString toString;
        int order;
        double probability;
        double contribution;
    };
    std::vector<Product> m_products;
    bool m_withProbability;
};

} // namespace model
} // namespace gui
} // namespace scram

#endif // PRODUCTTABLEMODEL_H
