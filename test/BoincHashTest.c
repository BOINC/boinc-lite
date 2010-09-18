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
#include "BoincHashTest.h"
#include <string.h>
#include <stdio.h>

BoincError testBoincHashMd5String(void* data)
{
  char string[] = "bonjours très chers amis adorés ùùùùùù àààà";
  char wantedmd5[] = "28f4ec9e275883579cf3f51866d71adc";
  char md5res[33];
  BoincError err = boincHashMd5String(string, md5res);
  if (err)
    return err;
  boincDebugf("md5res %s (%s)", md5res, wantedmd5);
  if (strcmp(md5res, wantedmd5))
    return boincErrorCreate(kBoincError, 1, "wrong md5");
  return NULL;
}

BoincError testBoincHashMd5File(void* data)
{
  char file[] = "/tmp/md5test.txt";
  char string[] = "bonjours très chers amis adorés ùùùùùù àààà";
  char wantedmd5[] = "28f4ec9e275883579cf3f51866d71adc";
  char md5res[33];

  FILE * fh = fopen(file, "w+");
  fprintf(fh, "%s", string);
  fclose(fh);

  BoincError err = boincHashMd5File(file, md5res);
  boincDebugf("md5res %s (%s)", md5res, wantedmd5);
  if (err)
    return err;
  if (strcmp(md5res, wantedmd5))
    return boincErrorCreate(kBoincError, 1, "wrong md5");
  return NULL;
  
}

int testBoincHash(BoincTest * test)
{
  boincTestInitMajor(test, "BoincHash");
  boincTestRunMinor(test, "BoincHash md5 string", testBoincHashMd5String, NULL);
  boincTestRunMinor(test, "BoincHash md5 file", testBoincHashMd5File, NULL);
  boincTestEndMajor(test);
  return 0;
}
