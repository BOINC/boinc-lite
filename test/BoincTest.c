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
#include <string.h>
#include <stdio.h>

int boincTestInit(BoincTest * test)
{
  test->majorNb = 0;
  test->majorScore = 0;
  test->total = 0;
  return 0;
}

int boincTestInitMajor(BoincTest * test, char * name) 
{
  strncpy(test->majorName, name, 25);
  printf("============================================\n");
  printf("Running %s tests :\n", name);
  test->majorNb++;
  test->minorNb = 0;
  test->minorScore = 0;
  return 0;
}

int boincTestRunMinor(BoincTest * test, char * label, BoincTestFunction run, void * data) 
{
  test->minorNb++;
  test->total++;
  printf("\t%s : ", label);
  fflush(NULL);
  BoincError res = run(data);
  if (res) {
    printf("Error (#%d, %s) \n", boincErrorCode(res), boincErrorMessage(res));
    boincErrorDestroy(res);
    return 1;
  }
  printf("OK !!\n");
  test->minorScore++;
  return 0;
}

int boincTestEndMajor(BoincTest * test) {
  int pc = test->minorScore * 100 / test->minorNb;
  printf("--------------------------------------------\n");
  printf("Results: %3d%c\t\t[nb error(s): %02d/%02d]\n", pc, '%', test->minorNb - test->minorScore, test->minorNb);
  if (pc < 100) 
    return 1;
  test->majorScore++;
  return 0;
}

int boincTestEnd(BoincTest * test) {
  printf("============================================\n");
  printf(" Executed tests: %d\n", test->total);
  printf(" Executed groups: %d \n", test->majorNb);
  printf(" Nb fully completed: %d \n\n", test->majorScore);
  printf(" Results: %d%c\n", test->majorScore * 100 / test->majorNb, '%');
  printf("============================================\n");
  return 0;
}

