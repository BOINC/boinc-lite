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
#ifndef __BoincWorkUnit_H__
#define __BoincWorkUnit_H__

#include "BoincLite.h"

BoincWorkUnit * boincWorkUnitCreate(const char * workunitDir);
void boincWorkUnitDestroy(BoincWorkUnit* wu);
void boincWorkUnitGetStatus(BoincWorkUnit* workunit, BoincWorkUnitStatus* status);
int boincWorkUnitSetStatus(BoincWorkUnit* workunit, int status);
void boincWorkUnitSetError(BoincWorkUnit* workunit, BoincError error);
char* boincWorkUnitGetPath(BoincWorkUnit * wu, BoincFileInfo * fi, char * resFile, unsigned int resSize);
int boincWorkUnitFileExists(BoincWorkUnit * workunit, BoincFileInfo * file);
int boincWorkUnitFileNeedsDownload(BoincWorkUnit * workunit, BoincFileInfo * file);

#endif /* __BoincWorkUnit_H__ */



