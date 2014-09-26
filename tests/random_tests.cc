#include <gtest/gtest.h>

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <set>

#include <boost/assign/std/vector.hpp>

#include "random.h"

using namespace scram;

// Plots the sampled numbers in the range [0, 1].
void PlotDistribution(const std::multiset<double>& series) {
  assert(!series.empty());
  assert(*series.begin() >= 0);  // Min element.
  assert(*series.rbegin() <= 1);  // Max element.
  int num_bins = 50;
  double bin_width = 1.0 / num_bins;
  std::vector<int> bin_hight;
  int size = 0;
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
    int num_x = screen_hight * bin_hight[i] / static_cast<double>(max_size) + 0.5;
    bin_hight[i] = num_x;  // Hight in characters.
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

TEST(RandomTest, UniformReal) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(rng->UniformRealGenerator(0, 1));
  }
  std::cout << "\n    Uniform Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Triangular) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(rng->TriangularGenerator(0, 0.5, 1));
  }
  std::cout << "\n    Triangular Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, PiecewiseLinear) {
  using namespace boost::assign;
  Random* rng = new Random(std::time(0));
  std::vector<double> intervals;
  std::vector<double> weights;
  intervals += 0, 2, 4, 6, 8, 10;
  weights += 0, 1, 0, 1, 0, 1;
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(rng->PiecewiseLinearGenerator(intervals, weights) / 10.0);
  }
  std::cout << "\n    Piecewise Linear Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Histogram) {
  using namespace boost::assign;
  Random* rng = new Random(std::time(0));
  std::vector<double> intervals;
  std::vector<double> weights;
  intervals += 0, 2, 4, 6, 8, 10;
  weights += 1, 2, 4, 3, 1;
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(rng->HistogramGenerator(intervals, weights) / 10.0);
  }
  std::cout << "\n    Histogram Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Discrete) {
  using namespace boost::assign;
  Random* rng = new Random(std::time(0));
  std::vector<double> values;
  std::vector<double> weights;
  values += 0, 2, 4, 6, 8, 9;
  weights += 1, 2, 4, 3, 1, 4;
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(rng->DiscreteGenerator(values, weights) / 10.0);
  }
  std::cout << "\n    Discrete Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Binomial) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    series.insert(rng->BinomialGenerator(20, 0.5) / 20.0);
  }
  std::cout << "\n    Binomial Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Normal) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = rng->NormalGenerator(0.5, 0.15);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Normal Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, LogNormal) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = rng->LogNormalGenerator(0.3, 0.2);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Log-Normal Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Gamma) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = rng->GammaGenerator(2, 2) / 10;
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Gamma Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Beta) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = rng->BetaGenerator(2, 2);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Beta Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Weibull) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = rng->WeibullGenerator(3, 1) / 2;
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Weibull Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Exponential) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = rng->ExponentialGenerator(1) / 5;
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Exponential Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, Poisson) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = rng->PoissonGenerator(5) / 10;
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Poisson Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, LogUniform) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = (rng->LogUniformGenerator(0, std::log(3.7)) - 1) / std::exp(1);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Log-Uniform Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}

TEST(RandomTest, LogTriangular) {
  Random* rng = new Random(std::time(0));
  std::multiset<double> series;
  int sample_size = 1e5;
  for (int i = 0; i < sample_size; ++i) {
    double sample = 0;
    do {
      sample = (rng->LogTriangularGenerator(0, 0.5, std::log(3.7)) - 1) /
               std::exp(1);
    } while (sample < 0 || sample >= 1);
    series.insert(sample);
  }
  assert(series.size() == sample_size);
  std::cout << "\n    Log-Triangular Distribution of " << sample_size
      << " Real Numbers.\n" << std::endl;
  PlotDistribution(series);
  delete rng;
}
