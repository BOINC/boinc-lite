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
  BoincProxy, a light-weight implementation of the BOINC protocol.
*/ 

#ifndef __BoincProxy_H__
#define __BoincProxy_H__

#include "BoincLite.h"

/* Forward declarations. */

typedef struct _BoincProxy BoincProxy;


/** File info flags */
#define kBoincFileExecutable 1
#define kBoincFileGenerated  2
#define kBoincFileUpload     4
#define kBoincFileMainProgram 8



typedef struct _BoincProxyWorkUnit {
  BoincProxy* proxy;
  BoincWorkUnit* wu;
} BoincProxyWorkUnit;

/** \brief Create a new proxy to the BOINC server.
 *
 *  Use this function to create a new proxy to a BOINC server. The
 *  project URL will be taken from the BoincConfiguration passed as
 *  argument.
 *  
 *  \return A pointer to the proxy object or NULL if an error occured.
 */
BoincProxy* boincProxyCreate(BoincConfiguration* configuration);

/** \brief Initialize a proxy
 *
 * Do a http request to the server to have information about the project
 */
BoincError boincProxyInit(BoincProxy * proxy) ;

/** \brief Destroy a proxy
 */
void boincProxyDestroy(BoincProxy* proxy);

/** \brief Destroy a Work Unit
 */
void boincProxyDestroyWorkUnit(BoincWorkUnit * wu);

/** \brief send an authentication request to the BOINC server
 *
 *  This function sends the user ID and the password hash to the BOINC
 *  server. Both the user ID and password should have been set in the
 *  BoincConfiguration object before boincProxyAuthenticate is called.
 *  If the request is successful, the authenticator returned by the
 *  server will be equally be stored in the BoincConfiguration object.
 *  If an error occrued, the function returns a non-zero value. A more
 *  detailed error code and message can be obtained using
 *  boincProxyGetErrorCode and boincProxyGetErrorMessage.
 *
 *  \brief returns zero if no error occured, non-zero otherwise.
 */
BoincError boincProxyAuthenticate(BoincProxy* proxy);

BoincError boincProxyChangeAuthentication(BoincProxy* proxy,
                                          const char* email, 
                                          const char* password);

// Scheduler requests: 

/** \brief Request new work
 *
 *  \return A pointer to new work unit
 */
BoincError boincProxyRequestWork(BoincProxyWorkUnit * pwu);

// Data server requests: 

/** \brief  Upload a result file
 *
 *  \returns zero on success, non-zero otherwise.
 */
BoincError boincProxyUploadFile(BoincProxyWorkUnit * pwu, BoincFileInfo* file, int filenum, int filetotal);


/** \brief inform the server a work unit has been executed
 *
 */
BoincError boinProxyWorkUnitExecuted(BoincProxyWorkUnit * pwu, unsigned int error);

/** \brief Download a fileinfo
 *
 */
BoincError boincProxyDownloadFile(BoincProxyWorkUnit* pwu, BoincFileInfo* file, int filenum, int filetotal);


/** \brief change the status of a work unit
 *
 */
BoincError boincProxyChangeWorkUnitStatus(BoincProxyWorkUnit * pwu, int status);

/** \brief change progression of the current work unit status
 *
 */
BoincError boincProxyChangeWorkUnitProgress(BoincProxyWorkUnit * pwu, float progress);


void boincProxySetStatusChangedCallback(BoincProxy * proxy, 
					BoincStatusChangedCallback callback, 
					void *userData);


BoincError boincProxyLoadWorkUnit(BoincWorkUnit* wu);

#endif // __BoincProxy_H__
