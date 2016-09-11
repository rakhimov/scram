/*
 * Copyright (C) 2014-2016 Olzhas Rakhimov
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

/// @file xml_parser.cc
/// Implementation of XML Parser.

#include "xml_parser.h"

#include <cassert>

#include <libxml++/exceptions/parse_error.h>
#include <libxml++/exceptions/validity_error.h>
#include <libxml++/validators/relaxngvalidator.h>

namespace scram {

XmlParser::XmlParser(const std::stringstream& xml_input_snippet)
    : parser_(new xmlpp::DomParser()) {
  try {
    parser_->parse_memory(xml_input_snippet.str());
    assert(parser_->get_document());
  } catch (std::exception& ex) {
    throw ValidationError("Error loading XML file: " + std::string(ex.what()));
  }
}

void XmlParser::Validate(const std::stringstream& xml_schema_snippet) {
  xmlpp::RelaxNGValidator validator;
  try {
    validator.parse_memory(xml_schema_snippet.str());
  } catch (const xmlpp::parse_error& err) {
    throw LogicError("Schema could not be parsed: " + std::string(err.what()));
  }

  try {
    validator.validate(parser_->get_document());
  } catch (const xmlpp::validity_error& err) {
    throw ValidationError("Document failed schema validation: " +
                          std::string(err.what()));
  }
}

}  // namespace scram
