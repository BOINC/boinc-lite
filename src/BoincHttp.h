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
/*

  BoincHttp, a small abstraction library for handling HTTP queries.

  Copyright (C) 2008 Sony Computer Science Laboratory Paris 
  Authors: Peter Hanappe, Anthony Beurive, Tangui Morlier


  
  BoincHttp is a thin abstraction layer used by BoincLite for all HTTP
  communication. It is implemented on top of existing HTTP libraries,
  such as libcurl, and facilitates the porting of BoincLite to other
  platforms.
   
  It supposes that the native library handles the following features:
  - Keep-Alive and connection reuse
  - Automatic redirection
 
  Character encoding:

  All strings (char*) are expected to be encoded in UTF-8.
 
  SSL: 

  The underlying library should be able to handle HTTPS connections.
  TODO: define the level of support for certificate verification +
  possibly extra functions to specify the client certificates.
 
*/

#include <stdio.h>
#include "BoincError.h"
#ifndef __BoincHttp_H__
#define __BoincHttp_H__



/**  
 * Use curl HTTP POST callback :
 *  " The data area pointed at by the pointer uploadData may be filled 
 *   with at most size multiplied with nmemb number of bytes. Your function
 *   must return the actual number of bytes that you stored in that memory
 *   area. Returning 0 will signal end-of-file to the library and cause it
 *   to stop the current transfer. "
 *
 */
typedef size_t (*BoincHttpReadFun)( void *uploadData, size_t size, size_t nmemb, void *userData );

/**
 * Init HTTP engine
 * 
 * MUST BE CALLED before HttpGet or HttpPost
 */
BoincError boincHttpInit();

/**
 * Destroy the HTTP engine
 *
 * MUST BE CALLED after the last HttpGet or HttpPost use
 */
BoincError boincHttpCleanup();

/**
 * Retrieve the content of an url via an HTTP GET
 *
 * url: the url to download
 * localFile: the file the downloaded content will be stored in 
 */
BoincError boincHttpGet(const char* url, const char* localFile);

/**
 * POST a content to an url
 *
 * url : the url to download
 * localFile: the content will be stored in this file
 * contentLength: size in OCTETs (byte) of the uploaded content
 * readFunc: callback allowing to retrieve the uploaded content
 * userData: callback user arg
 */
BoincError boincHttpPost(const char* url, const char* localFile, unsigned int contentLength, BoincHttpReadFun readFunc, void* userData);

/**
 * Set a progress callback
 * must be set before every http calls : the callback attachement is reset after each call
 */
typedef void (*BoincHttpProgressCallback) (void * userdata, float progress);
BoincError boincHttpSetProgressCallback(BoincHttpProgressCallback progressFunc, void * progressData, int fileNum, int fileTotal);

#endif // __BoincHttp_H__
