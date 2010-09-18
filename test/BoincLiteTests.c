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
#include "BoincTest.h"
#include "BoincHttpTest.h"
#include "BoincProxyTest.h"
#include "BoincConfigurationTest.h"
#include "BoincHashTest.h"
#include "BoincSchedulerTest.h"
#include "BoincMem.h"
#include "BoincUtil.h"
#include <stdlib.h>

int main(int argc, char * argv[] ) 
{
  boincMemInit();

  BoincLogger log = { boincDefaultLogFunction, (void*) 9 };
  boincLogInit(&log);

  int test = 0;
  if (argc > 1) {
    test = atoi(argv[1]);
  }

  BoincTest myTests;
  boincTestInit(&myTests);


  // (void) boincNew(int); // to test memory leak detector

  switch (test) {
  default:
  case 0:
	  testBoincHash(&myTests);
	  testBoincHttp(&myTests);
	  testBoincConfiguration(&myTests);
	  testBoincProxy(&myTests);
	  testBoincScheduler(&myTests);
	  break;
  case 1:
	  log.userData = (void*) 0;
	  testBoincHash(&myTests);
	  break;
  case 2:
	  log.userData = (void*) 0;
	  testBoincHttp(&myTests);
	  break;
  case 3:
	  log.userData = (void*) 0;
	  testBoincConfiguration(&myTests);
	  break;
  case 4:
	  log.userData = (void*) 0;
	  testBoincProxy(&myTests);
	  break;
  case 999: ///LAST
  case 5:
	  log.userData = (void*) 0;
	  testBoincScheduler(&myTests);
	  break;
  }

  boincTestEnd(&myTests);

  boincMemLeaks();
  boincMemCleanup();

  return 0;
}
