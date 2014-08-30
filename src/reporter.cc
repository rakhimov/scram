/// @file reporter.cc
/// Implements Reporter class.
#include "reporter.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>

namespace pt = boost::posix_time;

namespace scram {
Reporter::Reporter() {}

void Reporter::ReportFta(FaultTreeAnalysis* fta,
                         std::map<std::string, std::string>& orig_ids,
                         std::string output) {
  // Check if output to file is requested.
  std::streambuf* buf;
  std::ofstream of;
  if (output != "cli") {
    of.open(output.c_str());
    buf = of.rdbuf();

  } else {
    buf = std::cout.rdbuf();
  }
  std::ostream out(buf);

  // An iterator for a set with ids of events.
  std::set<std::string>::iterator it_set;

  // Iterator for minimal cut sets.
  std::set< std::set<std::string> >::iterator it_min;

  // Iterator for a map with minimal cut sets and their probabilities.
  std::map< std::set<std::string>, double >::iterator it_pr;

  // Convert MCS into representative strings.
  std::map< std::set<std::string>, std::string> represent;
  std::map< std::set<std::string>, std::vector<std::string> > lines;
  for (it_min = fta->min_cut_sets_.begin(); it_min != fta->min_cut_sets_.end();
       ++it_min) {
    std::stringstream rep;
    rep << "{ ";
    std::string line = "{ ";
    std::vector<std::string> vec_line;
    int j = 1;
    int size = it_min->size();
    for (it_set = it_min->begin(); it_set != it_min->end(); ++it_set) {
      std::vector<std::string> names;
      std::string full_name = *it_set;
      boost::split(names, full_name, boost::is_any_of(" "),
                   boost::token_compress_on);
      assert(names.size() < 3);
      assert(names.size() > 0);
      std::string name = "";
      if (names.size() == 1) {
        name = orig_ids.find(names[0])->second;
      } else if (names.size() == 2) {
        name = "NOT " + orig_ids.find(names[1])->second;
      }
      rep << name;

      if (line.length() + name.length() + 2 > 60) {
        vec_line.push_back(line);
        line = name;
      } else {
        line += name;
      }

      if (j < size) {
        rep << ", ";
        line += ", ";
      } else {
        rep << " ";
        line += " ";
      }
      ++j;
    }
    rep << "}";
    line += "}";
    vec_line.push_back(line);
    represent.insert(std::make_pair(*it_min, rep.str()));
    lines.insert(std::make_pair(*it_min, vec_line));
  }

  // Print warnings of calculations.
  if (fta->warnings_ != "") {
    out << "\n" << fta->warnings_ << "\n";
  }

  // Print minimal cut sets by their order.
  out << "\n" << "Minimal Cut Sets" << "\n";
  out << "================\n\n";
  // out << std::setw(40) << std::left << "Fault Tree: " << fta->input_file_ << "\n";
  out << std::setw(40) << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << std::setw(40) << "Analysis algorithm: " << fta->analysis_ << "\n";
  out << std::setw(40) << "Limit on order of cut sets: " << fta->limit_order_ << "\n";
  out << std::setw(40) << "Number of Primary Events: " << fta->primary_events_.size() << "\n";
  out << std::setw(40) << "Minimal Cut Set Maximum Order: " << fta->max_order_ << "\n";
  out << std::setw(40) << "Gate Expansion Time: " << std::setprecision(5)
      << fta->exp_time_ << "s\n";
  out << std::setw(40) << "MCS Generation Time: " << std::setprecision(5)
      << fta->mcs_time_ - fta->exp_time_ << "s\n";
  out.flush();

  int order = 1;  // Order of minimal cut sets.
  std::vector<int> order_numbers;  // Number of sets per order.
  while (order < fta->max_order_ + 1) {
    std::set< std::set<std::string> > order_sets;
    for (it_min = fta->min_cut_sets_.begin(); it_min != fta->min_cut_sets_.end();
        ++it_min) {
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
        for (it = lines.find(*it_min)->second.begin(); it != lines.find(*it_min)->second.end(); ++it) {
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

  // Print probabilities of minimal cut sets only if requested.
  if (!fta->prob_requested_) return;

  out << "\n" << "Probability Analysis" << "\n";
  out << "====================\n\n";
  // out << std::setw(40) << std::left << "Fault Tree: " << fta->input_file_ << "\n";
  out << std::setw(40) << "Time: " << pt::second_clock::local_time() << "\n\n";
  out << std::setw(40) << "Analysis type:" << fta->analysis_ << "\n";
  out << std::setw(40) << "Limit on series: " << fta->nsums_ << "\n";
  out << std::setw(40) << "Number of Primary Events: "
      << fta->primary_events_.size() << "\n";
  out << std::setw(40) << "Number of Minimal Cut Sets: "
      << fta->min_cut_sets_.size() << "\n";
  out << std::setw(40) << "Probability Operations Time: " << std::setprecision(5)
      << fta->p_time_ - fta->mcs_time_ << "s\n\n";
  out.flush();

  if (fta->analysis_ == "default") {
    out << "Minimal Cut Set Probabilities Sorted by Order:\n";
    out << "----------------------------------------------\n";
    out.flush();
    order = 1;  // Order of minimal cut sets.
    std::multimap < double, std::set<std::string> >::reverse_iterator it_or;
    while (order < fta->max_order_ + 1) {
      std::multimap< double, std::set<std::string> > order_sets;
      for (it_min = fta->min_cut_sets_.begin(); it_min != fta->min_cut_sets_.end();
           ++it_min) {
        if (it_min->size() == order) {
          order_sets.insert(std::make_pair(fta->prob_of_min_sets_.find(*it_min)->second,
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
    for (it_or = fta->ordered_min_sets_.rbegin(); it_or != fta->ordered_min_sets_.rend();
         ++it_or) {
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

    // Print total probability.
    out << "\n" << "================================\n";
    out <<  "Total Probability: " << std::setprecision(7) << fta->p_total_ << "\n";
    out << "================================\n\n";

    if (fta->p_total_ > 1) out << "WARNING: Total Probability is invalid.\n\n";

    out.flush();

    // Primary event analysis.
    out << "Primary Event Analysis:\n";
    out << "-----------------------\n";
    out << std::left;
    out << std::setw(20) << "Event" << std::setw(20) << "Failure Contrib."
        << "Importance\n\n";
    std::multimap < double, std::string >::reverse_iterator it_contr;
    for (it_contr = fta->ordered_primaries_.rbegin();
         it_contr != fta->ordered_primaries_.rend(); ++it_contr) {
      out << std::left;
      std::vector<std::string> names;
      boost::split(names, it_contr->second, boost::is_any_of(" "),
                   boost::token_compress_on);
      assert(names.size() < 3);
      assert(names.size() > 0);
      if (names.size() == 1) {
        out << std::setw(20) << orig_ids.find(names[0])->second << std::setw(20)
            << it_contr->first << 100 * it_contr->first / fta->p_total_ << "%\n";

      } else if (names.size() == 2) {
        out << "NOT " << std::setw(16) << orig_ids.find(names[1])->second << std::setw(20)
            << it_contr->first << 100 * it_contr->first / fta->p_total_ << "%\n";
      }
      out.flush();
    }

  } else if (fta->analysis_ == "mc") {
    // Report for Monte Carlo Uncertainty Analysis.
    // Show the terms of the equation.
    // Positive terms.
    out << "\nPositive Terms in the Probability Equation:\n";
    out << "--------------------------------------------\n";
    std::vector< std::set<int> >::iterator it_vec;
    std::set<int>::iterator it_set;
    for (it_vec = fta->pos_terms_.begin(); it_vec != fta->pos_terms_.end(); ++it_vec) {
      out << "{ ";
      int j = 1;
      int size = it_vec->size();
      for (it_set = it_vec->begin(); it_set != it_vec->end(); ++it_set) {
        if (*it_set > 0) {
          out << orig_ids.find(fta->int_to_prime_[*it_set]->id())->second;
        } else {
          out << "NOT " << orig_ids.find(fta->int_to_prime_[std::abs(*it_set)]->id())->second;
        }
        if (j < size) {
          out << ", ";
        } else {
          out << " ";
        }
        ++j;
      }
      out << "}\n";
      out.flush();
    }
    // Negative terms.
    out << "\nNegative Terms in the Probability Equation:\n";
    out << "-------------------------------------------\n";
    for (it_vec = fta->neg_terms_.begin(); it_vec != fta->neg_terms_.end(); ++it_vec) {
      out << "{ ";
      int j = 1;
      int size = it_vec->size();
      for (it_set = it_vec->begin(); it_set != it_vec->end(); ++it_set) {
        if (*it_set > 0) {
          out << orig_ids.find(fta->int_to_prime_[*it_set]->id())->second;
        } else {
          out << "NOT " << orig_ids.find(fta->int_to_prime_[std::abs(*it_set)]->id())->second;
        }
        if (j < size) {
          out << ", ";
        } else {
          out << " ";
        }
        ++j;
      }
      out << "}\n";
      out.flush();
    }
  }
}

}  // namespace scram
