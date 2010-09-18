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
#include "BoincProxyTest.h"
#include "BoincWorkUnit.h"
#include "BoincError.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "BoincHttp.h"
#include "BoincMem.h"


typedef struct _BoincProxyTest {
  BoincProxy * proxy;
  BoincWorkUnit * wu;
  BoincConfiguration * configuration;
} BoincProxyTest;

BoincConfiguration * boincProxyGetConfiguration(BoincProxy* proxy);

BoincProxy * initProxy(BoincProxyTest * test) 
{
  memset(test, 0, sizeof(BoincProxyTest));
  test->configuration = boincConfigurationCreate(TMPDIR);
  if (!test->configuration)
    return NULL;
  if (boincErrorOccured(boincConfigurationLoad(test->configuration))) {
    boincConfigurationDestroy(test->configuration);
    return NULL;
  }
  boincErrorDestroy(boincConfigurationSetString(test->configuration, kBoincProjectURL, "http://matisse.cslparis.lan/FamousAtCSL/"));
  test->proxy = boincProxyCreate(test->configuration);
  if (!test->proxy) {
    boincConfigurationDestroy(test->configuration);
    return NULL;
  }

  BoincError err = boincProxyInit(test->proxy);
  if (boincErrorOccured(err))
    return NULL;

  return test->proxy;
}

void cleanupProxy(BoincProxyTest * test) {
  if (test->wu != NULL) {
    boincWorkUnitDestroy(test->wu);
    test->wu = NULL;
  }
  if (test->proxy != NULL) {
    boincProxyDestroy(test->proxy);
    test->proxy = NULL;
  }
  if (test->configuration != NULL) {
    boincConfigurationDestroy(test->configuration);
    test->configuration = NULL;
  }
  return ;
}


/*


  Status loading/saving is now handled by the scheduler. This test
  should be moved to the scheduler tests. [PH]



BoincError testBoincProxyLoadStatus(void* data) {

  BoincError err = NULL;

  BoincProxyTest test ;

  BoincProxy * proxy = initProxy(&test);
  BoincWorkUnit * wu = boincWorkUnitCreate(TMPDIR);
  BoincProxyWorkUnit pwu = {proxy, wu};
  boincProxyChangeWorkUnitStatus(&pwu, kBoincWorkUnitWaiting);
  boincProxyDestroyWorkUnit(wu);
  cleanupProxy(&test);

  proxy = initProxy(&test);
  wu = boincWorkUnitCreate(TMPDIR);
  err = boincProxyLoadWorkUnitStatus(wu);

  if (!err && (wu->status != kBoincWorkUnitWaiting))
    err = boincErrorCreate(kBoincError, 2, "workunit should be WAITING");

  boincWorkUnitDestroy(wu);
  cleanupProxy(&test);

  return err;
}
*/

BoincError testBoincProxyCreate(void* data) {
  BoincProxyTest * test = (BoincProxyTest*) data;
  BoincProxy * proxy = NULL;
  BoincError err = NULL;

  test->configuration = boincConfigurationCreate(TMPDIR);
  if (!test->configuration)
    return boincErrorCreate(kBoincError, -1, "cannot create configurator");
  err = boincConfigurationLoad(test->configuration);
  if (err) {
    boincConfigurationDestroy(test->configuration);
    return err;
  }
  boincErrorDestroy(boincConfigurationSetString(test->configuration, kBoincProjectURL, "http://matisse.cslparis.lan/FamousAtCSL/"));
  proxy = boincProxyCreate(test->configuration);
  test->proxy = proxy;
  if (!proxy) {
    boincConfigurationDestroy(test->configuration);
    test->configuration = NULL;
    return boincErrorCreate(kBoincError, 1, "Could not create boincProxy");
  }

  err = boincProxyInit(proxy);
  if (err)
    return err;

  struct stat buffer;
  if (stat(TMPDIR "project.xml", &buffer))
    err = boincErrorCreate(kBoincError, 2, "cannot access to server project configuration file");
  else if (!buffer.st_size)
    err = boincErrorCreate(kBoincError, 2, "server project configuration file should not be empty");
  cleanupProxy(test);

  return err;
}

BoincError testBoincProxyAuthenticateError(void * data) {
  BoincProxyTest * test = (BoincProxyTest*) data;
  BoincProxy * proxy = test->proxy;
  if (!proxy)
    return boincErrorCreate(kBoincError, -1, "no proxy allocated");
  if (boincErrorOccured(boincConfigurationSetString(test->configuration, kBoincUserEmail, "tanguim@gmail.com")))
    return boincErrorCreate(kBoincError, 1, "configurator pb with username");
  if (boincErrorOccured(boincConfigurationSetString(test->configuration, kBoincUserPassword, "blablabla123")))
    return boincErrorCreate(kBoincError, 2, "configurator pb with password");
  BoincError err = boincProxyAuthenticate(proxy);
  if (!err)
    return boincErrorCreate(kBoincError, 3, "Authenticate should return an error");
  boincErrorDestroy(err);
  return NULL;
}

BoincError testBoincProxyAuthenticate(void * data) {
  BoincProxyTest * test = (BoincProxyTest*) data;
  BoincProxy * proxy = test->proxy;
  if (!proxy)
    return boincErrorCreate(kBoincError, -1, "no proxy allocated");
  if (boincErrorOccured(boincConfigurationSetString(test->configuration, kBoincUserEmail, "tanguim@gmail.com")))
    return boincErrorCreate(kBoincError, 1, "configurator pb with username");
  if (boincErrorOccured(boincConfigurationSetString(test->configuration, kBoincUserPassword, "b781076bb15ca436151e8f612e8f9003")))
    return boincErrorCreate(kBoincError, 2, "configurator pb with password");
  BoincError err = boincProxyAuthenticate(proxy);
  if (err)
    return err;
  BoincConfiguration * conf = boincProxyGetConfiguration(proxy);
  char teststr[10];
  if (!boincConfigurationHasParameter(conf, kBoincUserAuthenticator) || 
      !strlen(boincConfigurationGetString(conf, kBoincUserAuthenticator, teststr, 10)))
    return boincErrorCreate(kBoincError, 3, "Cannont access to the authenticator value from boincConfiguration");
  return NULL;
}

BoincError testBoincProxyGetWorkUnit(void * data) 
{
  BoincProxyTest * test = (BoincProxyTest*) data;
  BoincProxy * proxy = (BoincProxy*) test->proxy;
  if (!proxy) {
    return boincErrorCreate(kBoincError, -1, "no proxy allocated");
  }

  BoincWorkUnit * wu = boincWorkUnitCreate(TMPDIR);
  BoincProxyWorkUnit pwu = {proxy, wu};
  BoincError err = boincProxyRequestWork(&pwu);
  if (err) {
    return err;
  }
  test->wu = wu;

  if (!wu->name || !wu->estimatedFlops)
    return boincErrorCreate(kBoincError, 1, "Workunit not initialised");
  /*
    if (wu->status != kBoincWorkUnitDefined)
    return boincErrorCreate(kBoincError, 2, "Workunit status should be defined");
  */
  if (!wu->fileInfo || !strlen(wu->fileInfo->name))
    return boincErrorCreate(kBoincError, 4, "The workunit should have a file info");
  if (!wu->app || ! wu->app->fileInfo || !strlen(wu->app->fileInfo->name))
    return boincErrorCreate(kBoincError, 3, "The workunit should have a file its app");
  if (!wu->result || ! wu->result->fileInfo || !strlen(wu->result->fileInfo->name))
    return boincErrorCreate(kBoincError, 5, "The workunit should have a file for its results");
  boincDebugf("res file info %d -> xml %s", wu->result->fileInfo, wu->result->fileInfo->xmlSignature);
  if (!wu->result->fileInfo->xmlSignature || !strlen(wu->result->fileInfo->xmlSignature)) 
    return boincErrorCreate(kBoincError, 7, "result first file info should have xml signature");
  if (!wu->result->fileInfo->maxBytes) 
    return boincErrorCreate(kBoincError, 7, "result first file info should have max nBytes");
  if (!wu->workingDir || !strlen(wu->workingDir)) 
    return boincErrorCreatef(kBoincError, 6, "working dir not set (%s)", wu->workingDir);
  return NULL;
}

BoincError testBoincProxyDownloadFile (void * data) {
  BoincProxyTest * test = (BoincProxyTest*) data;
  if (!test->proxy)
    return boincErrorCreate(kBoincError, -1, "no proxy allocated");
  BoincFileInfo * fi = test->wu->app->fileInfo;
  char filepath[255];
  boincDebugf("openname: %s -> %s", fi->name, fi->openname);
  boincWorkUnitGetPath(test->wu, fi, filepath, 255);
  struct stat buffer;
  if (!stat(filepath, &buffer))
    return boincErrorCreatef(kBoincError, 1, "file %s should not exist", filepath);
  BoincProxyWorkUnit pwu = {test->proxy, test->wu};
  BoincError err = boincProxyDownloadFile(&pwu, fi, 0, 1);
  if (err)
    return err;
  if (!boincWorkUnitFileExists(test->wu, fi))
    return boincErrorCreatef(kBoincError, 4, "file %s should exists and have the same checksum than its fileinfo", filepath);
  remove(filepath);
  if (buffer.st_size < 10)
    return boincErrorCreatef(kBoincError, 3, "file %s should not be empty", filepath);
  return NULL;
}

BoincError testBoincProxyUploadFile (void * data) 
{
  BoincProxyTest * test = (BoincProxyTest*) data;
  if (!test->proxy)
    return boincErrorCreate(kBoincError, -1, "no proxy allocated");
  BoincFileInfo * fi = test->wu->result->fileInfo;
  char filepath[255];
  boincDebugf("openname: %s -> %s", fi->name, fi->openname);
  boincWorkUnitGetPath(test->wu, fi, filepath, 255);
  FILE * fh = fopen(filepath, "w+");
  fprintf(fh, "bonjour chers amis !!!!");
  fclose(fh);
  BoincProxyWorkUnit pwu = {test->proxy, test->wu};
  BoincError err = boincProxyUploadFile(&pwu, fi, 0, 1);
  if (err)
    return err;
  return NULL;
}

BoincError testBoincProxyDestroyAll(void * data) {
  BoincProxyTest * test = (BoincProxyTest*) data;
  if (test->wu) {
    boincWorkUnitDestroy(test->wu);
    test->wu = NULL;
  }
  boincProxyDestroy(test->proxy);
  test->proxy = NULL;
  return NULL;
}

int testBoincProxy(BoincTest * test) 
{
  int failed = 0;

  boincTestInitMajor(test, "BoincProxy");
  boincHttpInit();

  BoincProxyTest proxytest;
  memset(&proxytest, 0, sizeof(BoincProxyTest));

  boincTestRunMinor(test, "BoincProxy create&destroy", testBoincProxyCreate, &proxytest);
  proxytest.proxy = initProxy(&proxytest);

  boincTestRunMinor(test, "BoincProxy authenticate error", testBoincProxyAuthenticateError, &proxytest);
  boincTestRunMinor(test, "BoincProxy authenticate", testBoincProxyAuthenticate, &proxytest);

  failed = boincTestRunMinor(test, "BoincProxy workunit", testBoincProxyGetWorkUnit, &proxytest);
  if (!failed) {
    boincTestRunMinor(test, "BoincProxy uploadfile", testBoincProxyUploadFile, &proxytest);
    boincTestRunMinor(test, "BoincProxy downloadfile", testBoincProxyDownloadFile, &proxytest);
  }

  boincTestRunMinor(test, "BoincProxy destroy", testBoincProxyDestroyAll, &proxytest);
  cleanupProxy(&proxytest);

  //boincTestRunMinor(test, "BoincProxy load status", testBoincProxyLoadStatus, NULL);

  boincTestEndMajor(test);
  return 0;
}
