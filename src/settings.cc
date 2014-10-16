/// @file settings.cc
/// Implementation of Settings Builder.
#include "settings.h"

#include "error.h"

namespace scram {

Settings::Settings()
    : num_sums_(7),
      limit_order_(20),
      approx_("no"),
      cut_off_(1e-8),
      mission_time_(8760),
      trials_(1e3),
      fta_type_("default") {}

Settings& Settings::limit_order(int order) {
  if (order < 1) {
    std::string msg = "The limit on the order of minimal cut sets "
                      "cannot be less than 1.";
    throw InvalidArgument(msg);
  }
  limit_order_ = order;
  return *this;
}

Settings& Settings::num_sums(int n) {
  if (n < 1) {
    std::string msg = "The number of sums in the probability calculation "
                      "cannot be less than 1";
    throw InvalidArgument(msg);
  }
  num_sums_ = n;
  return *this;
}

Settings& Settings::cut_off(double prob) {
  if (prob < 0 || prob > 1) {
    std::string msg = "The cut-off probability cannot be negative or"
                      " more than 1.";
    throw InvalidArgument(msg);
  }
  cut_off_ = prob;
  return *this;
}

Settings& Settings::approx(std::string approx) {
  if (approx != "no" && approx != "rare" && approx != "mcub") {
    std::string msg = "The probability approximation is not recognized.";
    throw InvalidArgument(msg);
  }
  approx_ = approx;
  return *this;
}

Settings& Settings::fta_type(std::string analysis) {
  if (analysis != "default" && analysis != "mc") {
    std::string msg = "The analysis type is not recognized.";
    throw InvalidArgument(msg);
  }
  fta_type_ = analysis;
  return *this;
}

Settings& Settings::trials(int trials) {
  if (trials < 0) {
    std::string msg = "The number of trials cannot be negative.";
    throw InvalidArgument(msg);
  }
  trials_ = trials;
  return *this;
}

Settings& Settings::mission_time(double time) {
  if (time < 0) {
    std::string msg = "The mission time cannot be negative.";
    throw InvalidArgument(msg);
  }
  mission_time_ = time;
  return *this;
}

}  // namespace scram
