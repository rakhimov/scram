/// @file reporter.cc
/// Implements Reporter class.
#include "reporter.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>

namespace pt = boost::posix_time;

namespace scram {

void Reporter::ReportOrphans(
    const std::set<boost::shared_ptr<PrimaryEvent> >& orphan_primary_events,
    std::ostream& out) {
  if (orphan_primary_events.empty()) return;

  out << "WARNING! Found unused primary events:\n";
  std::set<boost::shared_ptr<PrimaryEvent> >::const_iterator it;
  for (it = orphan_primary_events.begin(); it != orphan_primary_events.end();
       ++it) {
    out << "    " << (*it)->orig_id() << "\n";
  }
  out.flush();
}

void Reporter::ReportFta(
    const boost::shared_ptr<const FaultTreeAnalysis>& fta,
    std::ostream& out) {
  std::ios::fmtflags fmt(out.flags());  // Save the state to recover later.

  // Convert MCS into representative strings.
  std::map< std::set<std::string>, std::vector<std::string> > lines;
  Reporter::McsToPrint(fta->min_cut_sets_, fta->basic_events_, &lines);

  // Print warnings of calculations.
  if (fta->warnings_ != "") {
    out << "\n" << fta->warnings_ << "\n";
  }

  // Print minimal cut sets by their order.
  out << "\n" << "Minimal Cut Sets" << "\n";
  out << "================\n\n";
  out << std::left;
  out << std::setw(40) << std::left << "Top Event: " << fta->top_event_name_
      << "\n";
  out << std::setw(40) << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << std::setw(40) << "Gate Expansion Time: " << std::setprecision(5)
      << fta->exp_time_ << "s\n";
  out << std::setw(40) << "MCS Generation Time: " << std::setprecision(5)
      << fta->mcs_time_ - fta->exp_time_ << "s\n";
  out << std::setw(40) << "Number of Basic Events: "
      << fta->basic_events_.size() << "\n";
  out << std::setw(40) << "Number of Gates: " << fta->num_gates_ << "\n";
  out << std::setw(40) << "Limit on order of cut sets: "
      << fta->limit_order_ << "\n";
  out << std::setw(40) << "Minimal Cut Set Maximum Order: "
      << fta->max_order_ << "\n";
  out << std::setw(40) << "Total number of MCS found: "
      << fta->min_cut_sets_.size() << "\n";
  out.flush();

  int order = 1;  // Order of minimal cut sets.
  std::vector<int> order_numbers;  // Number of sets per order.
  while (order < fta->max_order_ + 1) {
    std::set< std::set<std::string> >::const_iterator it_min;
    std::set< std::set<std::string> > order_sets;
    for (it_min = fta->min_cut_sets_.begin();
         it_min != fta->min_cut_sets_.end(); ++it_min) {
      if (it_min->size() == order) {
        order_sets.insert(*it_min);
      }
    }
    order_numbers.push_back(order_sets.size());
    if (!order_sets.empty()) {
      out << "\nOrder " << order << ":\n";
      int i = 1;
      for (it_min = order_sets.begin(); it_min != order_sets.end(); ++it_min) {
        std::stringstream number;
        number << i << ") ";
        out << std::left;
        std::vector<std::string>::iterator it;
        int j = 0;
        for (it = lines.find(*it_min)->second.begin();
             it != lines.find(*it_min)->second.end(); ++it) {
          if (j == 0) {
            out << number.str() <<  *it << "\n";
          } else {
            out << "  " << std::setw(number.str().length()) << " "
                << *it << "\n";
          }
          ++j;
        }
        out.flush();
        i++;
      }
    }
    order++;
  }

  out << "\nQualitative Importance Analysis:" << "\n";
  out << "--------------------------------\n";
  out << std::left;
  out << std::setw(20) << "Order" << "Number\n";
  out << std::setw(20) << "-----" << "------\n";
  for (int i = 1; i < fta->max_order_ + 1; ++i) {
    out << "  " << std::setw(18) << i << order_numbers[i-1] << "\n";
  }
  out << "  " << std::setw(18) << "ALL" << fta->min_cut_sets_.size() << "\n";
  out.flush();
  out.flags(fmt);  // Restore the initial state.
}

void Reporter::ReportProbability(
    const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
    std::ostream& out) {
  std::ios::fmtflags fmt(out.flags());  // Save the state to recover later.
  // Print warnings of calculations.
  if (prob_analysis->warnings_ != "") {
    out << "\n" << prob_analysis->warnings_ << "\n";
  }

  out << "\n" << "Probability Analysis" << "\n";
  out << "====================\n\n";
  out << std::left;
  out << std::setw(40) << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << std::setw(40) << "Probability Calculations Time: "
      << std::setprecision(5) << prob_analysis->p_time_ << "s\n";
  out << std::setw(40) << "Importance Calculations Time: "
      << std::setprecision(5) << prob_analysis->imp_time_ << "s\n\n";
  out << std::setw(40) << "Approximation:" << prob_analysis->approx_ << "\n";
  out << std::setw(40) << "Limit on series: " << prob_analysis->nsums_ << "\n";
  out << std::setw(40) << "Cut-off probability for cut sets: "
      << prob_analysis->cut_off_ << "\n";
  out << std::setw(40) << "Total MCS provided: "
      << prob_analysis->min_cut_sets_.size() << "\n";
  out << std::setw(40) << "Number of Cut Sets Used: "
      << prob_analysis->num_prob_mcs_ << "\n";
  out << std::setw(40) << "Total Probability: "
      << prob_analysis->p_total_ << "\n";
  out.flush();

  // Print total probability.
  out << "\n================================\n";
  out <<  "Total Probability: " << std::setprecision(7)
      << prob_analysis->p_total_;
  out << "\n================================\n\n";

  if (prob_analysis->p_total_ > 1)
    out << "WARNING: Total Probability is invalid.\n\n";

  out.flush();

  Reporter::ReportMcsProb(prob_analysis, out);

  out.flush();

  Reporter::ReportImportance(prob_analysis, out);

  out.flush();
  out.flags(fmt);  // Restore the initial state.
}

void Reporter::ReportUncertainty(
    const boost::shared_ptr<const UncertaintyAnalysis>& uncert_analysis,
    std::ostream& out) {
  std::ios::fmtflags fmt(out.flags());  // Save the state to recover later.
  if (uncert_analysis->warnings_ != "") {
    out << "\n" << uncert_analysis->warnings_ << "\n";
  }
  out << "\n" << "Uncertainty Analysis" << "\n";
  out << "====================\n\n";
  out << std::left;
  out << std::setw(40) << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << std::setw(40) << "Uncertainty Calculation Time: "
      << uncert_analysis->p_time_ << "\n";
  out << std::setw(40) << "Number of trials: "
      << uncert_analysis->num_trials_ << "\n";
  out << std::setw(40) << "Mean: " << uncert_analysis->mean() << "\n";
  out << std::setw(40) << "Standard deviation: "
      << uncert_analysis->sigma() << "\n";
  out << std::setw(40) << "Confidence range(95%): "
      << uncert_analysis->confidence_interval().first
      << " -:- " << uncert_analysis->confidence_interval().second << "\n";
  out << "\nDistribution:\n";
  out << std::setw(40) << "Bin Bounds (b(n), b(n+1)]" << "Value\n";
  std::vector<std::pair<double, double> >::const_iterator it;
  for (it = uncert_analysis->distribution().begin();
       it != uncert_analysis->distribution().end(); ++it) {
    out << std::setw(40) << it->first << it->second << "\n";
  }
  out.flush();
  out.flags(fmt);  // Restore the initial state.
}

void Reporter::ReportMcsProb(
    const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
    std::ostream& out) {
  std::ios::fmtflags fmt(out.flags());  // Save the state to recover later.

  // Convert MCS into representative strings.
  std::map< std::set<std::string>, std::vector<std::string> > lines;
  Reporter::McsToPrint(prob_analysis->min_cut_sets_,
                       prob_analysis->basic_events_,
                       &lines);

  out << "\nMinimal Cut Set Probabilities Sorted by Order:\n";
  out << "----------------------------------------------\n";
  out.flush();

  int order = 1;  // Order of minimal cut sets.
  int max_order = 1;

  std::set< std::set<std::string> >::const_iterator it_min;
  // Find max order
  for (it_min = prob_analysis->min_cut_sets_.begin();
       it_min != prob_analysis->min_cut_sets_.end(); ++it_min) {
    if (it_min->size() > max_order) max_order = it_min->size();
  }

  std::multimap < double, std::set<std::string> >::const_reverse_iterator
      it_or;
  while (order < max_order + 1) {
    std::multimap< double, std::set<std::string> > order_sets;
    for (it_min = prob_analysis->min_cut_sets_.begin();
         it_min != prob_analysis->min_cut_sets_.end(); ++it_min) {
      if (it_min->size() == order) {
        order_sets.insert(
            std::make_pair(
                prob_analysis->prob_of_min_sets_.find(*it_min)->second,
                            *it_min));
      }
    }
    if (!order_sets.empty()) {
      out << "\nOrder " << order << ":\n";
      int i = 1;
      for (it_or = order_sets.rbegin(); it_or != order_sets.rend(); ++it_or) {
        std::stringstream number;
        number << i << ") ";
        out << std::left;
        std::vector<std::string>::iterator it;
        int j = 0;
        for (it = lines.find(it_or->second)->second.begin();
             it != lines.find(it_or->second)->second.end(); ++it) {
          if (j == 0) {
            out << number.str() << std::setw(70 - number.str().length())
                << *it << std::setprecision(7) << it_or->first << "\n";
          } else {
            out << "  " << std::setw(number.str().length()) << " "
                << *it << "\n";
          }
          ++j;
        }
        out.flush();
        i++;
      }
    }
    order++;
  }

  out << "\nMinimal Cut Set Probabilities Sorted by Probability:\n";
  out << "----------------------------------------------------\n";
  out.flush();
  int i = 1;
  for (it_or = prob_analysis->ordered_min_sets_.rbegin();
       it_or != prob_analysis->ordered_min_sets_.rend(); ++it_or) {
    std::stringstream number;
    number << i << ") ";
    out << std::left;
    std::vector<std::string>::iterator it;
    int j = 0;
    for (it = lines.find(it_or->second)->second.begin();
         it != lines.find(it_or->second)->second.end(); ++it) {
      if (j == 0) {
        out << number.str() << std::setw(70 - number.str().length())
            << *it << std::setprecision(7) << it_or->first << "\n";
      } else {
        out << "  " << std::setw(number.str().length()) << " "
            << *it << "\n";
      }
      ++j;
    }
    i++;
    out.flush();
  }
  out.flags(fmt);  // Restore the initial state.
}

void Reporter::McsToPrint(
    const std::set< std::set<std::string> >& min_cut_sets,
    const boost::unordered_map<std::string, BasicEventPtr>& basic_events,
    std::map< std::set<std::string>, std::vector<std::string> >* lines) {

  std::set< std::set<std::string> >::const_iterator it_min;
  for (it_min = min_cut_sets.begin(); it_min != min_cut_sets.end();
       ++it_min) {
    std::string line = "{ ";
    std::vector<std::string> vec_line;
    int j = 1;
    int size = it_min->size();

    std::set<std::string>::const_iterator it_set;
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      std::vector<std::string> names;
      std::string full_name = *it_set;
      boost::split(names, full_name, boost::is_any_of(" "),
                   boost::token_compress_on);
      assert(names.size() < 3);
      assert(names.size() > 0);
      std::string name = "";
      if (names.size() == 1) {
        name = basic_events.find(names[0])->second->orig_id();
      } else if (names.size() == 2) {
        name = "NOT " + basic_events.find(names[1])->second->orig_id();
      }

      if (line.length() + name.length() + 2 > 60) {
        vec_line.push_back(line);
        line = name;
      } else {
        line += name;
      }

      if (j < size) {
        line += ", ";
      } else {
        line += " ";
      }
      ++j;
    }
    line += "}";
    vec_line.push_back(line);
    lines->insert(std::make_pair(*it_min, vec_line));
  }
}

void Reporter::ReportImportance(
    const boost::shared_ptr<const ProbabilityAnalysis>& prob_analysis,
    std::ostream& out) {
  std::ios::fmtflags fmt(out.flags());  // Save the state to recover later.
  // Basic event analysis.
  out << "\nBasic Event Analysis:\n";
  out << "-----------------------\n";
  out << std::left;
  out << std::setw(20) << "Event"
      << std::setw(12) << "DIF"
      << std::setw(12) << "MIF"
      << std::setw(12) << "CIF"
      << std::setw(12) << "RRW" << "RAW"
      << "\n\n";
  std::multimap < double, std::string >::const_reverse_iterator it_contr;
  for (it_contr = prob_analysis->ordered_primaries_.rbegin();
       it_contr != prob_analysis->ordered_primaries_.rend(); ++it_contr) {
    out << std::left;
    out << std::setw(20)
        << prob_analysis->basic_events_.find(it_contr->second)->second
              ->orig_id();
    for (int i = 0; i < 5; ++i) {
        if (i < 4) out << std::setw(12);
        out << std::setprecision(4)
            << prob_analysis->importance_.find(it_contr->second)->second[i];
    }
    out << "\n";
  }
  out.flags(fmt);  // Restore the initial state.
}

}  // namespace scram
