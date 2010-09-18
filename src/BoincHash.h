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

#ifndef __BoincHash_H__
#define __BoincHash_H__

/** \brief md5 a string
 * \param[in]  src string to hash
 * \param[out] hash  the md5 result (must have space for 32 bytes)
 */
BoincError boincHashMd5String(const char * src, char * hash) ;

/** \brief md5 a string
 * \param[in]  fileName path to the file to md5
 * \param[out] hash  the md5 result (must have space for 32 bytes)
 */
BoincError boincHashMd5File(const char * fileName, char * hash) ;

#endif // __BoincHash_H__
