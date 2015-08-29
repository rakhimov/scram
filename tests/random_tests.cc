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

#include "random.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <set>
#include <vector>

#include <gtest/gtest.h>

namespace scram {
namespace test {

/// Plots the sampled numbers in the range [0, 1].
///
/// @param[in,out] series The sampled results.
void PlotDistribution(const std::vector<double>& series) {
  assert(!series.empty());
  int num_bins = 50;
  double bin_width = 1.0 / num_bins;
  std::vector<int> bin_hight(num_bins + 1, 0);
  for (double sample : series) {
    int bin = sample / bin_width;
    ++bin_hight[bin];
  }
  int max_size = *std::max_element(bin_hight.begin(), bin_hight.end());
  int screen_hight = 20;  // Max number of characters in hight.
  for (int i = 0; i < bin_hight.size(); ++i) {
    int num_x = screen_hight * bin_hight[i] / static_cast<double>(max_size) +
                0.5;
    bin_hight[i] = num_x;  // Height in characters.
  }
  std::string hight_char = "x";
  for (int i = screen_hight; i > 0; --i) {
    std::cout << "    ";
    for (int j = 0; j < bin_hight.size(); ++j) {
      (i <= bin_hight[j]) ? std::cout << hight_char : std::cout << " ";
    }
    std::cout << std::endl;
  }
  std::ios::fmtflags fmt(std::cout.flags());
  std::cout << "    0" << std::right << std::setw(num_bins + 1)
      << "1\n" << std::endl;
  std::cout.flags(fmt);
}

TEST(RandomTest, Seed) {
  ASSERT_NO_THROW(Random::seed(std::time(0)));
}

TEST(RandomTest, UniformReal) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    series.push_back(Random::UniformRealGenerator(0, 1));
  }
  std::cout << "\n    Uniform Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Min: 0    Max: 1\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Triangular) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    series.push_back(Random::TriangularGenerator(0, 0.5, 1));
  }
  std::cout << "\n    Triangular Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Min: 0    Mode: 0.5    Max: 1\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, PiecewiseLinear) {
  std::vector<double> intervals = {0, 2, 4, 6, 8, 10};
  std::vector<double> weights = {0, 1, 0, 1, 0, 1};
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    series.push_back(
        Random::PiecewiseLinearGenerator(intervals, weights) / 10.0);
  }
  std::cout << "\n    Piecewise Linear Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Intervals: 0  2  4  6  8  10" << std::endl;
  std::cout << "        Weights:   0  1  0  1  0  1\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Histogram) {
  std::vector<double> intervals = {0, 2, 4, 6, 8, 10};
  std::vector<double> weights = {1, 2, 4, 3, 1};
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    series.push_back(Random::HistogramGenerator(intervals, weights) / 10.0);
  }
  std::cout << "\n    Histogram Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Intervals: 0   2   4   6   8   10" << std::endl;
  std::cout << "        Weights:     1   2   4   3   1\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Discrete) {
  std::vector<double> values = {0, 2, 4, 6, 8, 9};
  std::vector<double> weights = {1, 2, 4, 3, 1, 4};
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    series.push_back(Random::DiscreteGenerator(values, weights) / 10.0);
  }
  std::cout << "\n    Discrete Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Values:  0  2  4  6  8  9" << std::endl;
  std::cout << "        Weights: 1  2  4  3  1  4\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Binomial) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    series.push_back(Random::BinomialGenerator(20, 0.5) / 20.0);
  }
  std::cout << "\n    Binomial Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "      Trials: 20    Prob: 0.5   Scale: 1/20\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Normal) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::NormalGenerator(0.5, 0.15);
    } while (sample < 0 || sample > 1);
    series.push_back(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Normal Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Mean: 0.5    Sigma: 0.15\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, LogNormal) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::LogNormalGenerator(0, 2);
    } while (sample < 0 || sample > 1);
    series.push_back(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Log-Normal Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Mean: 0    Sigma: 2\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Gamma) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::GammaGenerator(2, 2) / 10;
    } while (sample < 0 || sample > 1);
    series.push_back(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Gamma Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        k: 2    theta: 2   Scaled-down: 1/20\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Beta) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::BetaGenerator(2, 2);
    } while (sample < 0 || sample > 1);
    series.push_back(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Beta Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        alpha: 2    beta: 2\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Weibull) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::WeibullGenerator(3, 1) / 2;
    } while (sample < 0 || sample > 1);
    series.push_back(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Weibull Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        k: 3    lambda: 1    Scaled-down: 1/2\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Exponential) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::ExponentialGenerator(1) / 5;
    } while (sample < 0 || sample > 1);
    series.push_back(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Exponential Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        lambda: 1    Scaled-down: 1/5\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Poisson) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::PoissonGenerator(5) / 10.0;
    } while (sample < 0 || sample > 1);
    series.push_back(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Poisson Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Mean: 5    Scaled-down: 1/10\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, LogUniform) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = (Random::LogUniformGenerator(0, std::log(3.7)) - 1) /
               std::exp(1);
    } while (sample < 0 || sample > 1);
    series.push_back(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Log-Uniform Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Min: 0    Max: 1.308    Shifted: -1    "
      << "Scaled-down: 1/2.7\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, LogTriangular) {
  std::vector<double> series;
  int sample_size = 1e5;
  series.reserve(sample_size);
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = (Random::LogTriangularGenerator(0, 0.5, std::log(3.7)) - 1) /
               std::exp(1);
    } while (sample < 0 || sample > 1);
    series.push_back(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Log-Triangular Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Lower: 0    Mode: 0.5    Upper: 1.308"
      << "    Shifted: -1    Scaled-down: 1/2.7\n" << std::endl;
  PlotDistribution(series);
}

}  // namespace test
}  // namespace scram
