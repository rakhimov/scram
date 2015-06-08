/// @file relax_ng_validator.h
/// RelaxNG Validator.
/* relaxngvalidator.h
 * this class is agented off of the schemavalidator in libxml++
 * here is their license statement:
 *
 * libxml++ and this file are copyright (C) 2000 by Ari Johnson,
 * (C) 2002-2004 by the libxml dev team and
 * are covered by the GNU Lesser General Public License, which should be
 * included with libxml++ as the file COPYING.
 */

#ifndef SCRAM_SRC_RELAX_NG_VALIDATOR_H_
#define SCRAM_SRC_RELAX_NG_VALIDATOR_H_

#include <glibmm/ustring.h>
#include <libxml++/document.h>
#include <libxml/relaxng.h>

namespace scram {

/// @class RelaxNGValidator
/// This class provides a simple interface to validate XML documents
/// against a given RelaxNG schema.
class RelaxNGValidator {
 public:
  RelaxNGValidator();

  /// Releases XML related memory by calling release_underlying().
  ~RelaxNGValidator();

  /// Parse a RelaxNG schema XML file.
  /// @param[in] contents The contents of the XML file.
  void parse_memory(const Glib::ustring& contents);

  /// Validate an XML file against the given schema.
  /// @param[in] doc The XML file document.
  bool Validate(const xmlpp::Document* doc);

 protected:
  /// Free XML-related memory.
  void release_underlying();

  /// Parse a RelaxNG schema context.
  /// @param[in] context The context.
  void parse_context(xmlRelaxNGParserCtxtPtr context);

  xmlRelaxNGPtr schema_;  ///< The schema.
  xmlRelaxNGValidCtxtPtr valid_context_;  ///< The validated context.
};

}  // namespace scram

#endif  // SCRAM_SRC_RELAX_NG_VALIDATOR_H_
