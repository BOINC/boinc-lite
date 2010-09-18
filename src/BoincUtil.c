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
#include "BoincUtil.h"
#include "BoincHash.h"
#include "BoincMem.h"
#include "BoincMutex.h"
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>


#define startsWith(_s,_c) (_s[0] == _c)
#define endsWith(_s,_c) ((_s[0] != 0) && (_s[strlen(_s)-1] == _c))
#define fileSeparator '/'
#define fileSeparatorStr "/"

char* initFilename(char* path, int len, const char* filename)
{
	if (strlen(filename) >= len) {
		return NULL;
	}
	strcpy(path, filename);
	return path;
}

char* appendFilename(char* path, int len, const char* filename) 
{
  if (strlen(filename) == 0) {
    return path;
  }
  if (startsWith(filename, fileSeparator)) {
    // the filename is an absolute path: overwrite the existing path
    int size = strlen(filename) + 1;
    if (size > len) 
      return NULL;
    strcpy(path, filename);

  } else if (endsWith(path, fileSeparator)) {
    int size = strlen(path) + strlen(filename) + 1;
    if (size > len)
      return NULL;
    strcat(path, filename);

  } else if (strlen(path) == 0) {
    int size = strlen(filename) + 1;
    if (size > len)
      return NULL;
    strcat(path, filename);

  } else {
    int size = strlen(path) + strlen(filename) + 2;
    if (size > len)
      return NULL;
    strcat(path, fileSeparatorStr);
    strcat(path, filename);
  }

  return path;  
}

char* boincGetAbsolutePath(const char * filebase, const char * relativeFilename, char * resFile, int resSize) 
{
  if (initFilename(resFile, resSize, filebase) == NULL) {
    return NULL;
  }
  if (appendFilename(resFile, resSize, relativeFilename) == NULL) {
    return NULL;
  }
  return resFile;  
}

#ifndef S_ISDIR
#define S_ISDIR(__m) (((__m) & S_IFMT) == S_IFDIR)
#endif

static BoincError removeItem(char* path)
{
  struct stat buff;
  BoincError err;

  if (stat(path, &buff)) {
    return NULL; // if it doesn't exist, that's good: that's exactly what we want
  }

  if (S_ISDIR(buff.st_mode)) {
    struct dirent * next;

    DIR * dh = opendir(path);
    if (dh == NULL) {
      return boincErrorCreatef(kBoincFatal, kBoincFileSystem, "Failed to open the directory %s", path); 
    }

    while ((next = readdir(dh))) {
      if (next->d_name[0] == '.' && (next->d_name[1] == '.' || next->d_name[1] == 0))
	continue;

      int len = strlen(path) + strlen(next->d_name) + 2;
      char* nextPath = boincArray(char, len);
      if (nextPath == NULL) {
        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");	      
      }
      nextPath[0] = 0;
      appendFilename(nextPath, len, path);
      appendFilename(nextPath, len, next->d_name);
      err = removeItem(nextPath);
      boincFree(nextPath);
      if (err)
	return err;
    }

    closedir(dh);
    if (rmdir(path))
      return boincErrorCreatef(kBoincFatal, kBoincFileSystem, "Cannot remove directory %s", path);

  } else if (unlink(path)) {
	  return boincErrorCreatef(kBoincFatal, kBoincFileSystem, "Cannot unlink %s", path);
  }

  return NULL;
}

BoincError boincRemoveDirectory(char* path)
{
  boincDebugf("RemoveDirecteory %s", path);
  return removeItem(path);
}

BoincError boincRenameFile(char* oldpath, char* newpath)
{
  BoincError err = NULL;

  struct stat buffer;
  if (stat(newpath, &buffer) == 0) {
	  if (unlink(newpath)) {
		  boincLogf(5, 5, "Cannot unlink %s (%s)", newpath, strerror(errno));
		  err = boincErrorCreatef(kBoincError, -1, "Cannot unlink %s", newpath);
	  }
  }
  if (rename(oldpath, newpath) ) {
    boincLogf(5, 5, "Cannot rename %s to %s (%s)", oldpath, newpath, strerror(errno));
    err = boincErrorCreatef(kBoincError, -1, "Cannot rename %s to %s", oldpath, newpath);
  }

  return err;
}

void boincPasswordHash(const char* email, const char* password, char hash[33])
{
	if ((email == NULL) || (password == NULL)) {
		memset(hash, 0, 33); // ???
		return;
	}

	int len = strlen(email) + strlen(password) + 1;
	char* buf = boincArray(char, len);
	if (buf == NULL) {
		memset(hash, 0, 33); // ???
		return;
	}
	strcpy(buf, password);
	strcat(buf, email);
	
	boincHashMd5String(buf, hash);
}


typedef struct _QueueEntry QueueEntry;

struct _QueueEntry
{
	unsigned int time;
	void* data;
	QueueEntry* next;
};

struct _BoincQueue
{
  QueueEntry * first;
  BoincMutex mutex;
};


BoincQueue* boincQueueCreate()
{
  BoincQueue* queue = boincNew(BoincQueue);
  if (queue == NULL) {
    return NULL;
  }
  queue->mutex = boincMutexCreate("queue");
  if (queue->mutex == NULL) {
    boincFree(queue);
    return NULL;
  }
  return queue;
}

void boincQueueDestroy(BoincQueue* queue)
{
  QueueEntry * qe = NULL;
  for(qe = queue->first ; queue->first ; qe = queue->first ) {
    queue->first = qe->next;
    boincFree(qe);
  }
  boincMutexLock(queue->mutex);
  boincMutexUnlock(queue->mutex);
  boincMutexDestroy(queue->mutex);
}

unsigned int boincQueueNow()
{
  return (unsigned int) time(NULL);
}

BoincError boincQueuePut(BoincQueue* queue, unsigned int eventTime, void* data)
{
  boincDebug("boincQueuePut");

  QueueEntry * qe = boincNew(QueueEntry);
  if (qe == NULL) {
    return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
  }
  qe->data = data;
  qe->time = eventTime;

  boincMutexLock(queue->mutex);

  QueueEntry ** pqe = &(queue->first);
  while (*pqe) {
    if ((*pqe)->time > eventTime) {
      break;
    }
    pqe = &((*pqe)->next);
  }
  qe->next = *pqe;
  *pqe = qe;

  boincMutexUnlock(queue->mutex);

  return NULL;
}


static inline int boincQueueHasEventLocked(BoincQueue* queue)
{
  return (queue->first && queue->first->time <= boincQueueNow());
}

int boincQueueHasEvent(BoincQueue* queue)
{
  boincMutexLock(queue->mutex);
  int r = boincQueueHasEventLocked(queue);
  boincMutexUnlock(queue->mutex);

  return r;
}

void* boincQueueGet(BoincQueue* queue)
{
  QueueEntry * qe = NULL;
  void * data = NULL;

  boincMutexLock(queue->mutex);

  if (boincQueueHasEventLocked(queue)) {
    qe = queue->first;
    queue->first = qe->next;
  }

  boincMutexUnlock(queue->mutex);

  if (qe) {
    data = qe->data;
    boincFree(qe);
  }

  return data;
}

#define DEBUG_FOPEN 0

#if DEBUG_FOPEN

static FILE* _openFP[20];
static char* _openFiles[20];

FILE* boincFopen(const char* filename, const char* mode)
{
	for (int i = 0; i < 20; i++) {
		if ((_openFiles[i] != NULL) && (strcmp(_openFiles[i], filename) == 0)) {
			boincLogf(kBoincError, kBoincFileSystem, "File opened twice: %s\n", filename);
		}
	}

	int index = -1;
	for (int i = 0; i < 20; i++) {
		if (_openFiles[i] == NULL) {
			index = i;
			_openFiles[i] = strdup(filename);
			break;
		}
	}
	if (index == -1) {
		boincLogf(kBoincError, kBoincFileSystem, "Too many files open\n");	
		boincLogf(kBoincError, kBoincFileSystem, "Open files:\n");	
		for (int i = 0; i < 20; i++) {
			if (_openFiles[i] != NULL) {
				boincLogf(kBoincError, kBoincFileSystem, "%s\n", _openFiles[i]);	
			}
		}
	}

	FILE* fp = fopen(filename, mode);

	if (fp == NULL) {
		boincLogf(kBoincError, kBoincFileSystem, "Failed to open %s\n", filename);	
		return NULL;
	}
		
	_openFP[index] = fp;

	return fp;
}

int boincFclose(FILE* fp)
{
	int index = -1;

	for (int i = 0; i < 20; i++) {
		if (_openFP[i] == fp) {
			index = i;
			break;
		}
	}

	if (index != -1) {
		free(_openFiles[index]);
		_openFiles[index] = NULL;
	}
	
	return fclose(fp);
}


#else

FILE* boincFopen(const char* filename, const char* mode)
{
	return fopen(filename, mode);
}

int boincFclose(FILE* fp)
{
	return fclose(fp);
}

#endif
