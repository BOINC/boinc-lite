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
#ifndef __BoincMutex_H__
#define __BoincMutex_H__

#if !defined(BOINCLITE_SINGLE_THREADED)

typedef struct _BoincMutexImpl* BoincMutex; 

/* Initialise a mutex.
 * Returns 0 if no error occured, non-zero otherwise.
 */
BoincMutex boincMutexCreate(char* name);

/* End of the mutex.
 * Returns 0 if no error occured, non-zero otherwise.
 */
void boincMutexDestroy(BoincMutex mutex);

/* Lock the mutex.
 * Returns 0 if no error occured, non-zero otherwise.
 */
int boincMutexLock(BoincMutex mutex);

/* Unlock the mutex.
 * Returns 0 if no error occured, non-zero otherwise.
 */
int boincMutexUnlock(BoincMutex mutex);

#else

typedef int BoincMutex; 
#define boincMutexCreate(_n) (0)
#define boincMutexDestroy(_m)
#define boincMutexLock(_m) (0)
#define boincMutexUnlock(_m) (0)

#endif

typedef void (*boincTermHandler)(int);

void boincCatchSigTerm(boincTermHandler handler);

#endif /* __BoincMutex_H__ */
