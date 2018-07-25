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

#include "producttablemodel.h"

#include "src/event.h"

#include "align.h"
#include "guiassert.h"
#include "translate.h"

namespace scram::gui::model {

ProductTableModel::ProductTableModel(const core::ProductContainer &products,
                                     bool withProbability, QObject *parent)
    : QAbstractTableModel(parent), m_withProbability(withProbability)
{
    m_products.reserve(products.size());
    double sum = 0;
    for (const core::Product &product : products) {
        QString members;
        for (auto it = product.begin(), it_end = product.end(); it != it_end;) {
            const core::Literal &literal = *it;
            if (literal.complement)
                members.append(QStringLiteral("\u00AC"));
            members.append(QString::fromStdString(literal.event.id()));
            if (++it != it_end)
                members.append(QStringLiteral(" \u22C5 "));
        }
        double probability = m_withProbability ? product.p() : 0;
        sum += probability;
        m_products.push_back(
            {std::move(members), product.order(), probability});
    }

    if (sum == 0)
        return;

    for (Product &product : m_products)
        product.contribution = product.probability / sum;
}

int ProductTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_products.size();
}

int ProductTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : (m_withProbability ? 4 : 2);
}

QVariant ProductTableModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const
{
    if (role == Qt::InitialSortOrderRole && section > 1)
        return Qt::DescendingOrder;
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QAbstractTableModel::headerData(section, orientation, role);
    GUI_ASSERT(m_withProbability || section < 2, {});
    switch (section) {
    case 0:
        return _("Product");
    case 1:
        return _("Order");
    case 2:
        return _("Probability");
    case 3:
        return _("Contribution");
    }
    GUI_ASSERT(false && "unexpected header section", {});
}

QVariant ProductTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};
    if (role == Qt::TextAlignmentRole && index.column() != 0)
        return ALIGN_NUMBER_IN_TABLE;
    if (role != Qt::DisplayRole)
        return {};

    GUI_ASSERT(index.row() < m_products.size(), {});
    const Product &product = m_products[index.row()];
    switch (index.column()) {
    case 0:
        return product.toString;
    case 1:
        return product.order;
    case 2:
        return product.probability;
    case 3:
        return product.contribution;
    }
    GUI_ASSERT(false && "unexpected column", {});
}

} // namespace scram::gui::model
