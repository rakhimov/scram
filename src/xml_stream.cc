/*
 * Copyright (C) 2016-2017 Olzhas Rakhimov
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

/// @file xml_stream.cc
/// Implementation of facilities to stream for XML data.

#include "xml_stream.h"

#include <cassert>

namespace scram {
namespace xml {

StreamElement::StreamElement(const char* name, detail::Indenter* indenter,
                             std::ostream& out)
    : StreamElement(name, 0, nullptr, indenter, out) {}

StreamElement::StreamElement(const char* name, int indent,
                             StreamElement* parent, detail::Indenter* indenter,
                             std::ostream& out)
    : kName_(name),
      kIndent_(indent),
      accept_attributes_(true),
      accept_elements_(true),
      accept_text_(true),
      active_(true),
      parent_(parent),
      indenter_(*indenter),
      out_(out) {
  if (*kName_ == '\0')
    throw StreamError("The element name can't be empty.");

  if (parent_) {
    if (!parent_->active_)
      throw StreamError("The parent is inactive.");
    parent_->active_ = false;
  }
  assert(kIndent_ >= 0 && "Negative XML indentation.");
  out_ << indenter_(kIndent_) << "<" << kName_;
}

StreamElement::~StreamElement() noexcept {
  assert(active_ && "The child element may still be alive.");
  assert(!(parent_ && parent_->active_) && "The parent must be inactive.");
  if (parent_)
    parent_->active_ = true;
  if (accept_attributes_) {
    out_ << "/>\n";
  } else if (accept_elements_) {
    out_ << indenter_(kIndent_);
closing_tag:
    out_ << "</" << kName_ << ">\n";
  } else {
    assert(accept_text_ && "The element is in unspecified state.");
    goto closing_tag;
  }
}

StreamElement StreamElement::AddChild(const char* name) {
  if (!active_)
    throw StreamError("The element is inactive.");
  if (!accept_elements_)
    throw StreamError("Too late to add elements.");
  if (*name == '\0')
    throw StreamError("Element name can't be empty.");

  if (accept_text_)
    accept_text_ = false;
  if (accept_attributes_) {
    accept_attributes_ = false;
    out_ << ">\n";
  }
  return StreamElement(name, kIndent_ + kIndentIncrement, this, &indenter_,
                       out_);
}

}  // namespace xml
}  // namespace scram
