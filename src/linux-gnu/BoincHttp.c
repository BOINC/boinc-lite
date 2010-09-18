/*

  BoincHttp, a small abstraction library for handling HTTP queries.

  Copyright (C) 2008 Sony Computer Science Laboratory Paris 
  Authors: Tangui Morlier
 
*/
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "BoincHttp.h"
#include "config.h"
#include "BoincError.h"
#include "BoincMem.h"


typedef struct _BoincHttp {
  CURL *curl;
  BoincHttpProgressCallback progressfunc;
  void * progressdata;
  int progressFileNum;	
  int progressFileTotal;	
  char ua[100];
} BoincHttp;


BoincHttp * singleHttp = NULL;

BoincError boincHttpInit() 
{
  if (!singleHttp) {
    singleHttp = boincNew(BoincHttp);
    singleHttp->curl = curl_easy_init();
    if (!singleHttp->curl)
      return boincErrorCreate(kBoincFatal, 0, "Cannot initialise HTTP layer");
    strcpy(singleHttp->ua, "User-Agent: \"");
    strcat(singleHttp->ua, PACKAGE_STRING);
    strcat(singleHttp->ua, "\"");
  }
  return NULL;
}

BoincError boincHttpCleanup() 
{
  if (!singleHttp)
    return NULL;
  if (singleHttp->curl)
    curl_easy_cleanup(singleHttp->curl);
  singleHttp->curl = NULL;
  boincFree(singleHttp);
  singleHttp = NULL;
  return NULL;
}

BoincError boincHttpSetProgressCallback(BoincHttpProgressCallback progressFunc, void * progressData, int fileNum, int fileTotal)
{
  BoincError err = boincHttpInit();
  if (err)
    return err;
  singleHttp->progressfunc = progressFunc;
  singleHttp->progressdata = progressData;
  singleHttp->progressFileNum = fileNum;	
  singleHttp->progressFileTotal = (fileTotal <= 0)? 1 : fileTotal;
  return NULL;
}

int boincHttpProgressInternalFunc(BoincHttp * bHttp,
		     double t, /* dltotal */
		     double d, /* dlnow */
		     double ultotal,
		     double ulnow)
{
  float progress = 100.0f * ((float) singleHttp->progressFileNum + (float) d / (float) t) / (float) singleHttp->progressFileTotal; 
  if (bHttp->progressfunc)
	  bHttp->progressfunc(bHttp->progressdata, progress);
  return 0;
}

BoincError boincHttpCommon(BoincHttp * bHttp, const char* localFile) 
{
  int res;

  //Redirection
  curl_easy_setopt(bHttp->curl, CURLOPT_FOLLOWLOCATION, 1);
  //Error management
  curl_easy_setopt(bHttp->curl, CURLOPT_FAILONERROR, 1);
  //Result file
  char * tmpfile = boincArray(char, strlen(localFile) + 5);
  strcpy(tmpfile, localFile);
  strcat(tmpfile, ".tmp");
  FILE * file = fopen(tmpfile, "w+");
  if (file == NULL) {
    return boincErrorCreatef(kBoincError, 0, "Failed to open file %s", localFile);
  }
  curl_easy_setopt(bHttp->curl, CURLOPT_WRITEDATA, file);
  //User agent
  struct curl_slist *slist=NULL;
  slist = curl_slist_append(slist, bHttp->ua);
  curl_easy_setopt(bHttp->curl, CURLOPT_HTTPHEADER, slist); 

  //Progress callback
  if (bHttp->progressfunc) {
    curl_easy_setopt(bHttp->curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(bHttp->curl, CURLOPT_PROGRESSFUNCTION, boincHttpProgressInternalFunc);
    curl_easy_setopt(bHttp->curl, CURLOPT_PROGRESSDATA, bHttp);
  }

  //Downloading
  res = curl_easy_perform(bHttp->curl);
  fclose(file);
  rename(tmpfile, localFile);

  curl_slist_free_all(slist);

  singleHttp->progressfunc = NULL;
  singleHttp->progressdata = NULL;

  //Set progress to 100% at the end of the download
  if (bHttp->progressfunc != NULL)
    bHttp->progressfunc(bHttp->progressdata, 100.0);

  //Error management
  if (res) {
    long statLong ;
    curl_easy_getinfo(bHttp->curl, CURLINFO_HTTP_CODE, &statLong);

    if (statLong != 200) {
      struct stat resStat;
      if (!stat(localFile, &resStat)) {
        if (resStat.st_size == 0) {
          boincLogf(kBoincError, 0, "unlink %s", localFile);
          unlink(localFile);
	}
      }
      return boincErrorCreatef(kBoincError, statLong, "HTTP returns %d code", statLong);
    }
    return boincErrorCreate(kBoincError, 2, curl_easy_strerror(res));
  }
   
  return NULL;
}

BoincError boincHttpGet(const char* url, const char* localFile) 
{
  BoincError err = boincHttpInit();
  if (err)
    return err;

  boincDebugf("HTTP GET %s will be saved in %s", url, localFile);
  curl_easy_setopt(singleHttp->curl, CURLOPT_URL, url);
  curl_easy_setopt(singleHttp->curl, CURLOPT_READFUNCTION, NULL);
  curl_easy_setopt(singleHttp->curl, CURLOPT_READDATA, NULL);
  curl_easy_setopt(singleHttp->curl, CURLOPT_POST, 0);
  curl_easy_setopt(singleHttp->curl, CURLOPT_POSTFIELDSIZE, 0);
  return boincHttpCommon(singleHttp, localFile);

}


BoincError boincHttpPost(const char* url, const char* localFile, unsigned int contentLength, BoincHttpReadFun readFunc, void* userData) 
{
  BoincError err = boincHttpInit();
  if (err)
    return err;
  
  curl_easy_setopt(singleHttp->curl, CURLOPT_URL, url);
  curl_easy_setopt(singleHttp->curl, CURLOPT_READFUNCTION, readFunc);
  curl_easy_setopt(singleHttp->curl, CURLOPT_READDATA, userData);
  curl_easy_setopt(singleHttp->curl, CURLOPT_POST, 1);
  curl_easy_setopt(singleHttp->curl, CURLOPT_POSTFIELDSIZE, contentLength);

  boincDebugf("HTTP POST %s will be save in %s", url, localFile);

  return boincHttpCommon(singleHttp, localFile);

}
