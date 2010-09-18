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

#ifndef __BoincError_H__
#define __BoincError_H__

#include "BoincLite.h" 

BoincError boincErrorCreateWithDelay(int code, const char* message, int delay);

int boincErrorDelay(BoincError error);

/** 
 * boincErrorIsMajor returns true if the error code is < 0
 **/
int boincErrorIsMajor(BoincError error);


/** \brief tell if an error occured and destroy the structure
 */
int boincErrorOccured(BoincError error);

void boincLogSchedulerStates(BoincScheduler * sched);

//#if __DEBUG__
#define boincDebugSchedulerStates(_scheduler) boincLogSchedulerStates(_scheduler)
/*#else
#define boincDebugSchedulerStates(_scheduler)
#endif*/

#endif // __BoincError_H__
