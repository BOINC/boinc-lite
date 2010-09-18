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
#ifndef __BoincUtil_H__
#define __BoincUtil_H__

#include "BoincError.h"

/*   Common file system functions    */


/** \brief initialise the path with the filename passed as argument.
 * \param[out] path absolute path in which the filename will written
 * \param[in] len max length of path
 * \param[in] filename
 * \returns modified path
 */
char* initFilename(char* path, int len, const char* filename);

/** \brief append filename to path (with directory separator management)
 * \param[out] path absolute path in which the filename will be append
 * \param[in] len max length of path
 * \param[in] filename
 * \returns modified path
 */
char* appendFilename(char* path, int len, const char* filename);


char* boincGetAbsolutePath(const char * filebase, const char * relativeFilename, char * resFile, int resSize);


BoincError boincRemoveDirectory(char* path);

BoincError boincRenameFile(char* oldpath, char* newpath);


/*   Event queue    */


typedef struct _BoincQueue BoincQueue;

BoincQueue* boincQueueCreate();

void boincQueueDestroy(BoincQueue* queue);

unsigned int boincQueueNow();

BoincError boincQueuePut(BoincQueue* queue, unsigned int eventTime, void* data);

int boincQueueHasEvent(BoincQueue* queue);

void* boincQueueGet(BoincQueue* queue);


#include <stdio.h>
FILE* boincFopen(const char* file, const char* mode);
int boincFclose(FILE* fp);


#endif // __BoincUtil_H__
