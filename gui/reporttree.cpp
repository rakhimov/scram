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

#include <cstdint>

#include "reporttree.h"

#include "guiassert.h"
#include "translate.h"

namespace scram::gui {

ReportTree::ReportTree(const std::vector<core::RiskAnalysis::Result> *results,
                       QObject *parent)
    : QAbstractItemModel(parent), m_results(*results)
{
}

int ReportTree::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_results.size();
    if (parent.parent().isValid())
        return 0;

    GUI_ASSERT(parent.row() < m_results.size(), 0);
    const core::RiskAnalysis::Result &result = m_results[parent.row()];
    if (result.importance_analysis)
        return 3;
    if (result.probability_analysis)
        return 2;
    GUI_ASSERT(result.fault_tree_analysis, 0);
    return 1;
}

int ReportTree::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 1;
    if (parent.parent().isValid())
        return 0;
    return 1;
}

QModelIndex ReportTree::index(int row, int column,
                              const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, nullptr);
    GUI_ASSERT(parent.parent().isValid() == false, {});
    // Carry the (parent-row-index + 1) in internal pointer.
    return createIndex(row, column, reinterpret_cast<void *>(parent.row() + 1));
}

QModelIndex ReportTree::parent(const QModelIndex &index) const
{
    GUI_ASSERT(index.isValid(), {});
    if (index.internalPointer() == nullptr)
        return {};
    // Retrieve the parent row index from the internal pointer.
    return createIndex(reinterpret_cast<std::uintptr_t>(index.internalPointer())
                           - 1,
                       0, nullptr);
}

QVariant ReportTree::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};
    struct
    {
        QString operator()(const mef::Gate *gate)
        {
            return QString::fromStdString(gate->id());
        }

        QString operator()(const std::pair<const mef::InitiatingEvent &,
                                           const mef::Sequence &> &)
        {
            GUI_ASSERT(false && "unexpected analysis target", {});
            return {};
        }
    } nameExtractor;

    if (!index.parent().isValid()) {
        GUI_ASSERT(index.row() < m_results.size(), {});
        return std::visit(nameExtractor, m_results[index.row()].id.target);
    }
    GUI_ASSERT(index.parent().row() < m_results.size(), {});
    const core::RiskAnalysis::Result &result = m_results[index.parent().row()];
    switch (static_cast<Row>(index.row())) {
    case Row::Products:
        return _("Products (%L1)")
            .arg(result.fault_tree_analysis->products().size());
    case Row::Probability:
        GUI_ASSERT(result.probability_analysis, {});
        return _("Probability (%1)")
            .arg(result.probability_analysis->p_total());
    case Row::Importance:
        GUI_ASSERT(result.importance_analysis, {});
        return _("Importance Factors (%L1)")
            .arg(result.importance_analysis->importance().size());
    }
    GUI_ASSERT(false && "Unexpected analysis report data", {});
}

} // namespace scram::gui
