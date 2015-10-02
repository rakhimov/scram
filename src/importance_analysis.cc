/*
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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

/// @file importance_analysis.cc
/// Implementations of functions to provide
/// quantitative importance informations.

#include "importance_analysis.h"

#include "logger.h"

namespace scram {

ImportanceAnalysis::ImportanceAnalysis(
    const ProbabilityAnalysis* prob_analysis)
    : Analysis::Analysis(prob_analysis->settings()) {}

void ImportanceAnalysis::Analyze() noexcept {
  CLOCK(imp_time);
  LOG(DEBUG3) << "Calculating importance factors...";
  std::vector<std::pair<int, BasicEventPtr>> target_events =
      this->GatherImportantEvents();
  double p_total = this->p_total();  /// @todo Delegate to Probability analysis.
  for (const auto& event : target_events) {
    double p_var = event.second->p();
    ImportanceFactors imp;
    imp.mif = this->CalculateMif(event.first);
    imp.cif = p_var * imp.mif / p_total;
    imp.raw = 1 + (1 - p_var) * imp.mif / p_total;
    imp.dif = p_var * imp.raw;
    imp.rrw = p_total / (p_total - p_var * imp.mif);
    importance_.emplace(event.second->id(), imp);
    important_events_.emplace_back(event.second, imp);
  }
  LOG(DEBUG3) << "Calculated importance factors in " << DUR(imp_time);
  analysis_time_ = DUR(imp_time);
}

}  // namespace scram
