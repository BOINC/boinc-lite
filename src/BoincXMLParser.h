/*
  BoincLite, a light-weight library for BOINC clients.

  Copyright (C) 2008 Sony Computer Science Laboratory Paris 
  Authors: Peter Hanappe, Anthony Beurive, Tangui Morlier

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; version 2.1 of the
  License.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301 USA

*/ 
#include "BoincError.h"

#ifndef __BoincXMLParser_H__
#define __BoincXMLParser_H__

/** \brief called when a tag is detected
 * \param[in] elementName tag name
 * \param[in] attr array of strings (even => attribute name, odd => attribute value)
 * \param[in] userData
 * \returns an identifyer passed as an argument of EndCallback (if positive)
 */
typedef int (*boincXMLElementBeginCallback) (const char* elementName, const char** attr, void* userData);

/** \brief called when a tag is closed (if the opening tag callback returned a positive value)
 * \param[in] index identifyer returned by BeginCallback
 * \param[in] elementValue string contained between opening and closing tag
 * \param[in] userData
 */
typedef BoincError (*boincXMLElementEndCallback) (int index, const char* elementValue, void* userData);

/** \brief parse xml file using user defined open and close callbacks
 *
 */
BoincError boincXMLParse(const char * xmlFile, boincXMLElementBeginCallback begin, boincXMLElementEndCallback end, void* userData);

/** \brief returns the value of a attribute name
 *
 *  cf. second argument of boincXMLElementBeginCallback
 *
 */
const char * boincXMLGetAttributeValue(const char** attributes, const char* attributeName) ;

/** \brief returns 0 if attribute value (defined by its name) is the same than the one expected
 *
 * \param[in] attributes ( cf. second argument of boincXMLElementBeginCallback)
 * \param[in] attributeName the name of the attribute we want to compare
 * \param[in] attributeValue the expected value 
 * \returns 0 if attribute value is th expected one ; -1 if the attribute does not exist ; 1 if the value is not the expected one
 * 
 */
int boincXMLAttributeCompare(const char** attributes, const char* attributeName, const char* attributeValue);


#endif // __BoincXMLParser_H__
