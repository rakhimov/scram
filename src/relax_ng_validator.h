/* relaxngvalidator.h
 * this class is agented off of the schemavalidator in libxml++
 * here is their license statement:
 *
 * libxml++ and this file are copyright (C) 2000 by Ari Johnson,
 * (C) 2002-2004 by the libxml dev team and
 * are covered by the GNU Lesser General Public License, which should be
 * included with libxml++ as the file COPYING.
 */

#ifndef SCRAM_RELAX_NG_VALIDATOR_H_
#define SCRAM_RELAX_NG_VALIDATOR_H_

#include <libxml/relaxng.h>
#include <libxml++/document.h>
#include <glibmm/ustring.h>

namespace scram {

/// RelaxNGValidator
///
/// This class provides a simple interface to validate xml documents
/// agaisnt a given RelaxNG schema.
class RelaxNGValidator {
 public:
  /// Constructor.
  RelaxNGValidator();

  /// Destructor.
  ~RelaxNGValidator();

  /// Parse a relaxng schema xml file.
  /// @param[in] contents The contents of the xml file.
  void parse_memory(const Glib::ustring& contents);

  /// Validate an xml file agaisnt the given schema.
  /// @param[in] doc The xml file document.
  bool Validate(const xmlpp::Document* doc);

 protected:
  /// Free xml-related memory.
  void release_underlying();

  /// Parse a relaxng schema context.
  /// @param[in] context The context.
  void parse_context(xmlRelaxNGParserCtxtPtr context);

  /// The schema.
  xmlRelaxNGPtr schema_;

  /// The validated context.
  xmlRelaxNGValidCtxtPtr valid_context_;
};

}  // namespace scram

#endif  // SCRAM_RELAX_NG_VALIDATOR_H_
