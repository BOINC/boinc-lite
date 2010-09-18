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
