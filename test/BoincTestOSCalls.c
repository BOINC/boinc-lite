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
#include "BoincTestOSCalls.h"
#include <string.h>
#include <stdlib.h>

int grep(char * file, char * motif) {
  char cmd [150] = "grep -q ";
  strncat(cmd, motif, 50);
  strcat(cmd, " ");
  strncat(cmd, file, 50);
  return (system(cmd));
}

int zgrep(char * file, char * motif) {
  char cmd [150] = "zgrep -q ";
  strncat(cmd, motif, 50);
  strcat(cmd, " ");
  strncat(cmd, file, 50);
  return (system(cmd));
}

int rmrf(const char * dir) {
  char cmd [150] = "rm -rf ";
  strncat(cmd, dir, 140);
  return (system(cmd));
}
