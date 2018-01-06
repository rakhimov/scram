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
/// Table model for reporting products.

#pragma once

#include <vector>

#include <QAbstractTableModel>

#include "src/fault_tree_analysis.h"

namespace scram::gui::model {

/// The table model for immutable analysis products.
class ProductTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /// @param[in] products  The analysis results.
    /// @param[in] withProbability  The analysis includes the probability data.
    /// @param[in,out] parent  The optional owner of this object.
    ///
    /// @pre The products container does not change
    ///      during the lifetime of this table model.
    ProductTableModel(const core::ProductContainer &products,
                      bool withProbability, QObject *parent = nullptr);

    /// Required standard member functions of QAbstractItemModel interface.
    /// @{
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    /// @}

private:
    /// Proxy wrapper/cache for a product in the container.
    ///
    /// @note The products are converted into strings
    ///       to simplify search & filter w/ Regex.
    struct Product
    {
        QString toString;    ///< Specially formatted/encoded list of literals.
        int order;           ///< The order of the product.
        double probability;  ///< The optional probability.
        double contribution; ///< The optional contribution.
    };
    std::vector<Product> m_products; ///< The proxy list of the products.
    bool m_withProbability; ///< The flag for probability data inclusion.
};

} // namespace scram::gui::model
