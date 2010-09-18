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
#ifndef __BoincTest_H__
#define __BoincTest_H__


#ifdef __cplusplus
#extern "C" {
#endif
#include "BoincError.h"
#include "BoincTestOSCalls.h"

typedef BoincError (*BoincTestFunction)(void * data);

typedef struct _BoincTest {
  char majorName[25];
  int majorNb;
  int majorScore;
  int minorNb;
  int minorScore;
  int total;
} BoincTest;

int boincTestInit(BoincTest * test);

int boincTestInitMajor(BoincTest * test, char * name);

int boincTestRunMinor(BoincTest * test, char * label, BoincTestFunction run, void * data);

int boincTestEndMajor(BoincTest * test) ;

int boincTestEnd(BoincTest * test) ;

#ifdef __cplusplus
}
#endif

#endif // __BoincTest_H__
