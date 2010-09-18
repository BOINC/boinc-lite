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
#include "BoincHttpTest.h"
#include "BoincHttp.h"
#include "BoincTest.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

BoincError TestBoincHttpGet200(void * data)
{
  /**
   * Download a page and check if the page result has a non null size
   */

  //  if (boincHttpGet("http://www.csl.sony.fr/", tmpfile, 0))
  BoincError res = boincHttpGet("http://www.google.jp/", TMPFILE);
  if (!res) {
    struct stat buffer;
    if (stat(TMPFILE, &buffer))
      res = boincErrorCreate(kBoincError, -3, "cannot access to the result file");
    else 
      if (! buffer.st_size)
	res = boincErrorCreate(kBoincError, -4, "result file should not be empty");
  }
  remove(TMPFILE);
  return res;
}

int dataCalledProgress;
void testBoincHttpProgressFunc(void * calledprogress, float percent)
{
  int * calledprogressint = (int*) calledprogress;
  *calledprogressint = 1;
}

BoincError TestBoincHttpGet200WithProgress(void * data)
{
  /**
   * Download a page and check if the page result has a non null size
   */
  dataCalledProgress = 0;
  boincErrorDestroy(boincHttpSetProgressCallback(testBoincHttpProgressFunc,  (void *) &dataCalledProgress, 0.0f, 100.0f));
  BoincError res = boincHttpGet("http://www.google.jp/", TMPFILE);
  if (!res) {
    struct stat buffer;
    if (stat(TMPFILE, &buffer))
      res = boincErrorCreate(kBoincError, -3, "cannot access to the result file");
    else 
      if (! buffer.st_size)
	res = boincErrorCreate(kBoincError, -4, "result file should not be empty");
      else if (dataCalledProgress == 0)
	res = boincErrorCreate(kBoincError, -5, "progress not called");
  }
  remove(TMPFILE);
  return res;
}


BoincError TestBoincHttpGet200WithoutProgress(void * data)
{
  /**
   * Download a page and check if the page result has a non null size
   */
  dataCalledProgress = 0;

  BoincError res = boincHttpGet("http://www.google.jp/", TMPFILE);
  if (!res) {
    struct stat buffer;
    if (stat(TMPFILE, &buffer))
      res = boincErrorCreate(kBoincError, -3, "cannot access to the result file");
    else 
      if (! buffer.st_size)
	res = boincErrorCreate(kBoincError, -4, "result file should not be empty");
      else if (dataCalledProgress != 0)
	res = boincErrorCreate(kBoincError, -5, "progress called");
  }
  remove(TMPFILE);
  return res;
}

/**
 * Request a 404 page and check if the function returns an error
 */
BoincError TestBoincHttpGet404(void * data)
{
  BoincError res = boincHttpGet("http://www.google.jp/404.html", TMPFILE);
  if (boincErrorCode(res) == 404) {
    boincErrorDestroy(res);
    res = NULL;
  }
  remove(TMPFILE);
  return res;
}

/**
 * Request a page in POST
 */
BoincError TestBoincHttpPost200(void * data) 
{
  BoincError res = boincHttpPost("http://www.csl.sony.fr/", TMPFILE, 0, NULL, NULL);
  remove(TMPFILE);
  return res;
}

BoincError TestBoincHttpPost404(void * data) 
{
  BoincError res = boincHttpPost("http://www.sony.fr/404.html", TMPFILE, 0, NULL, NULL);
  if (boincErrorCode(res) == 404) {
    boincErrorDestroy(res);
    res = NULL;
  }
  remove(TMPFILE);
  return res;
}
unsigned int post_index = 0;
size_t TestBoincHttpPostCallback( void *uploadData, size_t size, size_t nmemb, void *userData )
{
  char * tmp = (char *) userData;
  if (! tmp[post_index])
    return 0;
  strncpy(uploadData, &tmp[post_index++], 1);
  return 1 * sizeof(char);
}

BoincError TestBoincHttpPostForm(void * data) 
{
  post_index = 0;
  char* form  = "go=1&search=boinc";

  BoincError res = boincHttpPost("http://fr.wikipedia.org/wiki/Sp%C3%A9cial:Recherche", TMPFILE, strlen(form), TestBoincHttpPostCallback, form);
  if (!res) {
    if (grep(TMPFILE, "BOINC"))
      if (zgrep(TMPFILE, "BOINC"))
        res = boincErrorCreate(kBoincError, 999, "String 'BOINC' not found in result file");
  }
  remove(TMPFILE);
  return res;
}

BoincError TestBoincHttpPostFormError(void * dada) 
{
  post_index = 0;
  char * form  = "go=1&search=boinc";
  BoincError res = boincHttpPost("http://www.google.com/search", TMPFILE, strlen(form), TestBoincHttpPostCallback, form);
  if (boincErrorCode(res) == 405) {
    boincErrorDestroy(res);
    res = NULL;
  }
  remove(TMPFILE);
  return res;
}


int testBoincHttp(BoincTest * test) {
  boincTestInitMajor(test, "BoincHttp");
  boincHttpInit();
  boincTestRunMinor(test, "BoincHttpGet 200", TestBoincHttpGet200, NULL);
  boincTestRunMinor(test, "BoincHttpGet 404", TestBoincHttpGet404, NULL);
  boincTestRunMinor(test, "BoincHttpPost 200", TestBoincHttpPost200, NULL);
  boincTestRunMinor(test, "BoincHttpPost 404", TestBoincHttpPost404, NULL);
  boincTestRunMinor(test, "BoincHttpPost Form", TestBoincHttpPostForm, NULL);
  boincTestRunMinor(test, "BoincHttpPost Form Error", TestBoincHttpPostFormError, NULL);
  boincTestRunMinor(test, "HTTP GET after a Post (callback revocation)", TestBoincHttpGet200, NULL);
  boincTestRunMinor(test, "HTTP GET with progress", TestBoincHttpGet200WithProgress, NULL);
  boincTestRunMinor(test, "HTTP GET without progress", TestBoincHttpGet200WithoutProgress, NULL);
  boincHttpCleanup();
  boincTestEndMajor(test);
  return 0;
}
