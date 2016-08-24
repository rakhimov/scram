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

/*
 * libxml++ and this file are copyright (C) 2000 by Ari Johnson,
 * (C) 2002-2004 by the libxml dev team and
 * are covered by the GNU Lesser General Public License, which should be
 * included with libxml++ as the file COPYING.
 */

/// @file relax_ng_validator.h
/// RelaxNG Validator.
/// This class is agented off of the schemavalidator in libxml++.

#ifndef SCRAM_SRC_RELAX_NG_VALIDATOR_H_
#define SCRAM_SRC_RELAX_NG_VALIDATOR_H_

#include <boost/noncopyable.hpp>
#include <glibmm/ustring.h>
#include <libxml++/document.h>
#include <libxml/relaxng.h>

namespace scram {

/// This class provides a simple interface
/// to validate XML documents
/// against a given RelaxNG schema.
class RelaxNGValidator : private boost::noncopyable {
 public:
  /// Releases XML related memory.
  ~RelaxNGValidator() noexcept { RelaxNGValidator::ReleaseUnderlying(); }

  /// Parses a RelaxNG schema XML file.
  ///
  /// @param[in] contents  The contents of the XML file.
  ///
  /// @throws LogicError  The contents of the schema could not be parsed.
  void ParseMemory(const Glib::ustring& contents);

  /// Validates an XML file against the given schema.
  ///
  /// @param[in] doc  The XML file document.
  ///
  /// @throws ValidationError  The XML snippet failed schema validation.
  /// @throws InvalidArgument  The pointer to the doc cannot be NULL.
  /// @throws LogicError  There is no schema to validate against.
  /// @throws Error  Could not create validating context.
  void Validate(const xmlpp::Document* doc);

 private:
  /// Frees XML-related memory.
  void ReleaseUnderlying() noexcept;

  xmlRelaxNGPtr schema_ = nullptr;  ///< The schema.
  xmlRelaxNGValidCtxtPtr valid_context_ = nullptr;  ///< The validated context.
};

}  // namespace scram

#endif  // SCRAM_SRC_RELAX_NG_VALIDATOR_H_
