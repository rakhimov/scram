/*
 * Copyright (C) 2016 Olzhas Rakhimov
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

#include <string>

namespace scram {

XmlStreamElement::XmlStreamElement(const char* name, std::ostream& out)
    : XmlStreamElement(name, 0, nullptr, out) {}

XmlStreamElement::XmlStreamElement(const char* name, int indent,
                                   XmlStreamElement* parent, std::ostream& out)
    : kName_(name),
      kIndent_(indent),
      accept_attributes_(true),
      accept_elements_(true),
      accept_text_(true),
      active_(true),
      parent_(parent),
      out_(out) {
  if (*kName_ == '\0')
    throw XmlStreamError("The element name can't be empty.");
  if (kIndent_ < 0)
    throw XmlStreamError("Negative indentation.");

  if (parent_) {
    if (!parent_->active_)
      throw XmlStreamError("The parent is inactive.");
    parent_->active_ = false;
  }
  out_ << std::string(kIndent_, ' ') << "<" << kName_;
}

XmlStreamElement::~XmlStreamElement() noexcept {
  assert(active_ && "The child element may still be alive.");
  assert(!(parent_ && parent_->active_) && "The parent must be inactive.");
  if (parent_)
    parent_->active_ = true;
  if (accept_attributes_) {
    out_ << "/>\n";
  } else if (accept_elements_) {
    out_ << std::string(kIndent_, ' ');
closing_tag:
    out_ << "</" << kName_ << ">\n";
  } else {
    assert(accept_text_ && "The element is in unspecified state.");
    goto closing_tag;
  }
}

XmlStreamElement XmlStreamElement::AddChild(const char* name) {
  if (!active_)
    throw XmlStreamError("The element is inactive.");
  if (!accept_elements_)
    throw XmlStreamError("Too late to add elements.");
  if (*name == '\0')
    throw XmlStreamError("Element name can't be empty.");

  if (accept_text_)
    accept_text_ = false;
  if (accept_attributes_) {
    accept_attributes_ = false;
    out_ << ">\n";
  }
  return XmlStreamElement(name, kIndent_ + 2, this, out_);
}

}  // namespace scram
