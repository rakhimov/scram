/// @file expression.h
/// Expressions that describe basic events.
#ifndef SCRAM_SRC_EXPRESSION_H_
#define SCRAM_SRC_EXPRESSION_H_

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "element.h"

namespace scram {

/// @class Expression
/// The base class for all sorts of expressions to describe events.
class Expression {
 public:
  virtual ~Expression() {}

  /// @returns The mean value of this expression.
  virtual double Mean() = 0;

  /// @returns The sampled value of this expression.
  virtual double Sample() = 0;
};

/// @class ConstantExpression
/// Indicates a constant value.
class ConstantExpression : public Expression {
 public:
  /// Constructor for integer and float values.
  /// @param[in] val Any numerical value.
  ConstantExpression(double val) : value_(val) {}

  /// Constructor for boolean values.
  /// @param[in] val true for 1 and false for 0 value of this constant.
  ConstantExpression(bool val) : value_(val ? 1 : 0) {}

  inline double Mean() { return value_; }
  inline double Sample() { return value_; }

 private:
  /// The constant's value.
  double value_;
};

/// @enum Units
/// Provides units for parameters.
enum Units {
  kBool,
  kInt,
  kFloat,
  kHours,
  kInverseHours,
  kYears,
  kInverseYears,
  kFit,
  kDemands
};

typedef boost::shared_ptr<scram::Expression> ExpressionPtr;

/// @class Parameter
/// This class provides a representation of a variable in basic event
/// description. It is both expression and element description.
class Parameter : public Expression, public Element {
 public:

  /// Sets the expression of this basic event.
  /// @param[in] name The name of this variable (Case sensitive).
  /// @param[in] expression The expression to describe this event.
  Parameter(std::string name) : name_(name) {}

  /// Sets the expression of this parameter.
  /// @param[in] expression The expression to describe this parameter.
  inline void expression(const ExpressionPtr& expression) {
    expression_ = expression;
  }

  /// @returns The name of this variable.
  inline const std::string& name() { return name_; }

  /// Sets the unit of this parameter.
  /// @param[in] unit A valid unit.
  inline void unit(const Units& unit) { unit_ = unit; }

  /// @returns The unit of this parameter.
  inline const Units& unit() { return unit_; }

  /// Checks for circular references in parameters.
  /// @throws ValidationError if any cyclic reference is found.
  void CheckCyclicity();

  inline double Mean() { return expression_->Mean(); }
  inline double Sample() { return expression_->Sample(); }

 private:
  /// Helper funciton to check for cyclic references in parameters.
  /// @param[out] path The current path of names in cyclicity search.
  /// @throws ValidationError if any cyclic reference is found.
  void CheckCyclicity(std::vector<std::string>* path);

  /// Name of this parameter or variable.
  std::string name_;

  /// Units of this parameter.
  Units unit_;

  /// Expression for this parameter.
  ExpressionPtr expression_;
};

}  // namespace scram

#endif  // SCRAM_SRC_EXPRESSION_H_
