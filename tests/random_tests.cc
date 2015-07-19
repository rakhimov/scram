#include "random.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <set>

#include <boost/assign/std/vector.hpp>
#include <gtest/gtest.h>

using namespace scram;

// Plots the sampled numbers in the range [0, 1].
void PlotDistribution(const std::multiset<double>& series) {
  assert(!series.empty());
  assert(*series.begin() >= 0);  // Min element.
  assert(*series.rbegin() <= 1);  // Max element.
  int num_bins = 50;
  double bin_width = 1.0 / num_bins;
  std::vector<int> bin_hight;
  std::multiset<double>::const_iterator it = series.begin();
  for (int bin = 0; bin < num_bins; ++bin) {
    int size = 0;
    if (it != series.end()) {
      double upper_bound = bin * bin_width;
      while (*it <= upper_bound) {
        ++size;
        ++it;
        if (it == series.end()) break;
      }
    }
    bin_hight.push_back(size);
  }
  assert(bin_hight.size() == num_bins);
  int max_size = *std::max_element(bin_hight.begin(), bin_hight.end());
  int screen_hight = 20;  // Max number of characters in hight.
  for (int i = 0; i < num_bins; ++i) {
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
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(Random::UniformRealGenerator(0, 1));
  }
  std::cout << "\n    Uniform Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Min: 0    Max: 1\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Triangular) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(Random::TriangularGenerator(0, 0.5, 1));
  }
  std::cout << "\n    Triangular Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Min: 0    Mode: 0.5    Max: 1\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, PiecewiseLinear) {
  using namespace boost::assign;
  std::vector<double> intervals;
  std::vector<double> weights;
  intervals += 0, 2, 4, 6, 8, 10;
  weights += 0, 1, 0, 1, 0, 1;
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(Random::PiecewiseLinearGenerator(intervals, weights) / 10.0);
  }
  std::cout << "\n    Piecewise Linear Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Intervals: 0  2  4  6  8  10" << std::endl;
  std::cout << "        Weights:   0  1  0  1  0  1\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Histogram) {
  using namespace boost::assign;
  std::vector<double> intervals;
  std::vector<double> weights;
  intervals += 0, 2, 4, 6, 8, 10;
  weights += 1, 2, 4, 3, 1;
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(Random::HistogramGenerator(intervals, weights) / 10.0);
  }
  std::cout << "\n    Histogram Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Intervals: 0   2   4   6   8   10" << std::endl;
  std::cout << "        Weights:     1   2   4   3   1\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Discrete) {
  using namespace boost::assign;
  std::vector<double> values;
  std::vector<double> weights;
  values += 0, 2, 4, 6, 8, 9;
  weights += 1, 2, 4, 3, 1, 4;
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(Random::DiscreteGenerator(values, weights) / 10.0);
  }
  std::cout << "\n    Discrete Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Values:  0  2  4  6  8  9" << std::endl;
  std::cout << "        Weights: 1  2  4  3  1  4\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Binomial) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(Random::BinomialGenerator(20, 0.5) / 20.0);
  }
  std::cout << "\n    Binomial Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "      Trials: 20    Prob: 0.5   Scale: 1/20\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Normal) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::NormalGenerator(0.5, 0.15);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Normal Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Mean: 0.5    Sigma: 0.15\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, LogNormal) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::LogNormalGenerator(0, 2);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Log-Normal Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Mean: 0    Sigma: 2\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Gamma) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::GammaGenerator(2, 2) / 10;
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Gamma Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        k: 2    theta: 2   Scaled-down: 1/20\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Beta) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::BetaGenerator(2, 2);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Beta Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        alpha: 2    beta: 2\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Weibull) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::WeibullGenerator(3, 1) / 2;
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Weibull Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        k: 3    lambda: 1    Scaled-down: 1/2\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Exponential) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::ExponentialGenerator(1) / 5;
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Exponential Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        lambda: 1    Scaled-down: 1/5\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, Poisson) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = Random::PoissonGenerator(5) / 10;
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Poisson Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Mean: 5    Scaled-down: 1/10\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, LogUniform) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = (Random::LogUniformGenerator(0, std::log(3.7)) - 1) /
               std::exp(1);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Log-Uniform Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Min: 0    Max: 1.308    Shifted: -1    "
      << "Scaled-down: 1/2.7\n" << std::endl;
  PlotDistribution(series);
}

TEST(RandomTest, LogTriangular) {
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = (Random::LogTriangularGenerator(0, 0.5, std::log(3.7)) - 1) /
               std::exp(1);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Log-Triangular Distribution of " << sample_size
      << " Real Numbers." << std::endl;
  std::cout << "        Lower: 0    Mode: 0.5    Upper: 1.308"
      << "    Shifted: -1    Scaled-down: 1/2.7\n" << std::endl;
  PlotDistribution(series);
}
