/*
 * Copyright (C) 2000 Ari Johnson
 * Copyright (C) 2002-2004 The libxml dev team
 * Copyright (C) 2014-2015 Olzhas Rakhimov
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
/// @file relax_ng_validator.cc
/// Implementation of RelaxNG Validator.
/// This class is agented off of the schemavalidator in libxml++.
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
