/// @file relax_ng_validator.cc
/// Implementation of RelaxNG Validator.
/* relaxngvalidator.cpp
 * this class is agented off of the schemavalidator in libxml++
 * here is their license statement:
 *
 * libxml++ and this file are copyright (C) 2000 by Ari Johnson,
 * (C) 2002-2004 by the libxml dev team and
 * are covered by the GNU Lesser General Public License, which should be
 * included with libxml++ as the file COPYING.
 */

#include "relax_ng_validator.h"

#include <libxml/xmlerror.h>

#include "error.h"

namespace scram {

RelaxNGValidator::RelaxNGValidator() : schema_(NULL), valid_context_(NULL) {}

RelaxNGValidator::~RelaxNGValidator() {
  RelaxNGValidator::ReleaseUnderlying();
}

void RelaxNGValidator::ParseMemory(const Glib::ustring& contents) {
  xmlRelaxNGParserCtxtPtr context =
      xmlRelaxNGNewMemParserCtxt(contents.c_str(), contents.bytes());
  RelaxNGValidator::ParseContext(context);
}

void RelaxNGValidator::Validate(const xmlpp::Document* doc) {
  if (!doc) {
    throw InvalidArgument("Document pointer cannot be NULL");
  }

  if (!schema_) {
    throw LogicError("Must have a schema to validate document");
  }

  // A context is required at this stage only.
  if (!valid_context_) {
    valid_context_ = xmlRelaxNGNewValidCtxt(schema_);
  }

  if (!valid_context_) {
    throw Error("Couldn't create validating context");
  }

  int res = 0;  // Validation result.
  try {
    res = xmlRelaxNGValidateDoc(valid_context_,
                                const_cast<xmlDocPtr>(doc->cobj()));
  } catch (xmlError& e) {
    throw ValidationError(e.message);
  }

  if (res != 0) {
    throw ValidationError("Document failed schema validation");
  }
}

void RelaxNGValidator::ParseContext(xmlRelaxNGParserCtxtPtr context) {
  RelaxNGValidator::ReleaseUnderlying();  // Free any existing info.
  schema_ = xmlRelaxNGParse(context);
  if (!schema_) {
    throw LogicError("Schema could not be parsed");
  }
}

void RelaxNGValidator::ReleaseUnderlying() {
  if (valid_context_) {
    xmlRelaxNGFreeValidCtxt(valid_context_);
    valid_context_ = NULL;
  }

  if (schema_) {
    xmlRelaxNGFree(schema_);
    schema_ = NULL;
  }
}

}  // namespace scram
