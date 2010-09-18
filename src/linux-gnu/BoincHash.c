#include "BoincHash.h"
#include <string.h>
#include <stdio.h>

#include "md5.c"

char * boincHashConvert2Hex(const unsigned char * buff, unsigned int len, char * hex_res)
{
  int i;
  for (i = 0; i < len; i++)
    sprintf(hex_res + i * 2, "%02x", buff[i]);
  return hex_res;
}

BoincError boincHashMd5String(const char * src, char * hash)
{
  MD5_CTX state; 

  MD5Init(&state); 
  MD5Update(&state, src, strlen(src));
  MD5Final(&state);

  boincHashConvert2Hex(state.digest, 16, hash);
  return NULL;
}

BoincError boincHashMd5File(const char * fileName, char * hash)
{
  MD5_CTX state; 
  char buff[128];

  MD5Init(&state); 

  FILE * fh = fopen(fileName, "r");
  if (fh == NULL)
    return boincErrorCreatef(kBoincError, 1, "could not read file %s", fileName);
  int read = 1;
  while (!feof(fh) && !ferror(fh)) {
    read = fread(buff, sizeof(char), 128, fh);
    MD5Update(&state, buff, read);
  }
  fclose(fh);

  MD5Final(&state);
  boincHashConvert2Hex(state.digest, 16, hash);

  return NULL;
}

