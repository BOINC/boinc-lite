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
#include "BoincSchedulerTest.h"
#include "BoincError.h"
#include "BoincTestOSCalls.h"
#include "BoincMem.h"
#include "BoincUtil.h"
#include "BoincProxy.h"
//#include "BoincConfiguration.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

int computetest(BoincScheduler * scheduler, BoincWorkUnit * wu, void* userData) {
  sleep(10);

  BoincFileInfo * fi;
  for (fi = wu->result->fileInfo ; fi ; fi = fi->next) {
    char uploadFile[255];
    boincWorkUnitGetPath(wu, fi, uploadFile, 255);
    
    FILE * file = fopen(uploadFile, "w+");
    fprintf(file, "blablabalablablaabla");
    fclose(file);
  }

  boincErrorDestroy(boincSchedulerChangeWorkUnitStatus(scheduler, wu, kBoincWorkUnitFinished));

  return 1;
}

struct compute_data_t {
	BoincScheduler * scheduler;
	BoincWorkUnit * wu;
	void* userData;
};

void* compute_thread_main(void* data) 
{
	struct compute_data_t* compute_data = (struct compute_data_t*) data;
	computetest(compute_data->scheduler, compute_data->wu, compute_data->userData);
	free(compute_data);
	return NULL;
}

int computetestAsync(BoincScheduler * scheduler, BoincWorkUnit * wu, void* userData) 
{
  pthread_t compute_thread;
  
  struct compute_data_t* compute_data = (struct compute_data_t*) boincNew(struct compute_data_t);
  if (compute_data == NULL) {
    return -1;
  }
  compute_data->scheduler = scheduler;
  compute_data->wu = wu;
  compute_data->userData = userData;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&compute_thread, &attr, compute_thread_main, compute_data);
  
  return 0;
}

struct _BoincScheduler {
  BoincConfiguration * configuration;
  BoincProxy * proxy;
  int nbworkunits;
  BoincWorkUnit ** workunits;
  void * callback;
  void * userData;
  BoincQueue * queue;
};

BoincScheduler * initScheduler(BoincComputeWorkUnitCallback callback)
{
  mkdir(PROJECT_SCHEDULER_TEST_DIR, S_IRWXU);
  
  BoincConfiguration * conf = boincConfigurationCreate(PROJECT_SCHEDULER_TEST_DIR);
  BoincError err = boincConfigurationSetString(conf, kBoincProjectURL, "http://matisse.cslparis.lan/FamousAtCSL/");
  if (!err)
    err = boincConfigurationSetString(conf, kBoincUserEmail, "tanguim@gmail.com");
  if (!err)
    err = boincConfigurationSetString(conf, kBoincUserPassword, "b781076bb15ca436151e8f612e8f9003");

  if (err) {
    boincConfigurationDestroy(conf);
    boincErrorDestroy(err);
    return NULL;
  }

  BoincScheduler * scheduler = boincSchedulerCreate(conf, callback, NULL);

  scheduler->nbworkunits = 1;
  
  return scheduler;
}

void cleanupScheduler(BoincScheduler * scheduler)
{
  BoincConfiguration * conf = boincSchedulerGetConfiguration(scheduler);
  boincSchedulerDestroy(scheduler);
  boincConfigurationDestroy(conf);
}

BoincWorkUnit* boincSchedulerGetWorkUnit(BoincScheduler* scheduler, int index);

char * testBoincSchedulerGetPath(BoincScheduler * scheduler, char * str, const char * file)
{
  BoincWorkUnit * wu = boincSchedulerGetWorkUnit(scheduler, 0);
  if (wu)
    strcpy(str, wu->workingDir);
  else {
    boincConfigurationGetString(boincSchedulerGetConfiguration(scheduler), kBoincProjectDirectory, str, 255);
    appendFilename(str, 255, "0");
  }
  appendFilename(str, 255, file);
  return str;
}

char * testBoincSchedulerReadStatus(BoincScheduler * scheduler, char * str)
{
  char file[255];
  testBoincSchedulerGetPath(scheduler, file, "workunit.status");

  FILE * fh = fopen(file, "r");
  fscanf(fh, "%s", str);
  fclose(fh);
  return str;

}
void testBoincSchedulerWriteStatus(BoincScheduler * scheduler, char * str)
{
  char file[255];
  testBoincSchedulerGetPath(scheduler, file, "workunit.status");

  char backupfile[255];
  testBoincSchedulerGetPath(scheduler, backupfile, "workunit.status.time");

  sleep(1);

  boincDebug(backupfile);

  FILE * fh = fopen(backupfile, "w+");
  fclose(fh);

  fh = fopen(file, "w+");
  fprintf(fh, "%s", str);
  fclose(fh);

}

int testBoincSchedulerCompareFileWithStatus(BoincScheduler * scheduler, char * str)
{
  char status[255];
  testBoincSchedulerGetPath(scheduler, status, "workunit.status.time");
  struct stat statusstat;
  stat(status, &statusstat);

  char file[255];
  testBoincSchedulerGetPath(scheduler, file, str);
  struct stat filestat;
  stat(file, &filestat);
  
  return filestat.st_mtime - statusstat.st_mtime;
}

typedef struct _statusChanged {
  BoincScheduler* scheduler;
  int status;
  int reached;
} statusChangedSt;

void statusChanged(void *userData, int index) {
  statusChangedSt * sc = (statusChangedSt *) userData;
  BoincWorkUnitStatus workunitStatus;
  boincSchedulerGetWorkUnitStatus(sc->scheduler, &workunitStatus, index);
  if (sc->status == workunitStatus.status)
    sc->reached = 1;
}

BoincError testBoincSchedulerExecuteUntil(BoincScheduler * scheduler, int wantedStatus)
{
  BoincError err;

  statusChangedSt sc = { scheduler, 0, 0};
  sc.status = wantedStatus;
  boincSchedulerSetStatusChangedCallback(scheduler, statusChanged, &sc);
  
  err = boincSchedulerHandleEvents(scheduler);
  if (err)
    return err;
  while(1) {
    if (!boincSchedulerHasEvents(scheduler)) {
      sleep(1);
      continue;
    }
    if ((err = boincSchedulerHandleEvents(scheduler)) != NULL) {
      break;
    }
    if (sc.reached)
      break;
  }
  if (!err)
    err = boincSchedulerHandleEvents(scheduler);
  return err;
}

BoincError testBoincSchedulerUpload(void * data)
{
  BoincScheduler * scheduler = initScheduler(computetest);

  testBoincSchedulerWriteStatus(scheduler, "UPLOADING");

  sleep(2);

  BoincError err = testBoincSchedulerExecuteUntil(scheduler, kBoincWorkUnitUploading);

  char status[25];

  if (!err && strcmp(testBoincSchedulerReadStatus(scheduler, status), "UPLOADING"))
    err = boincErrorCreatef(kBoincError, 1, "workunit should be UPLOADING (not %s)", status);

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "uploadResponse.xml") <= 0)
    err = boincErrorCreate(kBoincError, 2, "workunit uploadResponse.xml should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "out.zip") > 0)
    err = boincErrorCreate(kBoincError, 3, "workunit out.zip should not have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "workRequest.xml") > 0)
    err = boincErrorCreate(kBoincError, 4, "workunit workResponse.xml should not have been modified");

  cleanupScheduler(scheduler);
  
  return err;
}

BoincError testBoincSchedulerCompute(void * data)
{
  BoincScheduler * scheduler = initScheduler(computetest);

  testBoincSchedulerWriteStatus(scheduler, "WAITING");

  BoincError err = testBoincSchedulerExecuteUntil(scheduler, kBoincWorkUnitUploading);

  char status[25];

  if (!err && strcmp(testBoincSchedulerReadStatus(scheduler, status), "UPLOADING"))
    err = boincErrorCreatef(kBoincError, 1, "workunit should be UPLOADING (not %s)", status);

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "uploadResponse.xml") <= 0)
    err = boincErrorCreate(kBoincError, 2, "workunit uploadResponse.xml should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "out.zip") < 0)
    err = boincErrorCreate(kBoincError, 3, "workunit out.zip should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "workRequest.xml") > 0)
    err = boincErrorCreate(kBoincError, 4, "workunit workRequest.xml should not have been modified");

  cleanupScheduler(scheduler);
  
  return err;
}

BoincError testBoincSchedulerDownload(void * data)
{
  BoincScheduler * scheduler = initScheduler(computetest);

  //Change input file to force download
  char file[255];
  testBoincSchedulerGetPath(scheduler, file, "in.zip");
  FILE * fh = fopen(file, "w+");
  fprintf(fh, "zzzzzzzz");
  fclose(fh);
  
  testBoincSchedulerWriteStatus(scheduler, "DOWNLOADING");

  BoincError err = testBoincSchedulerExecuteUntil(scheduler, kBoincWorkUnitUploading);

  char status[25];

  if (!err && strcmp(testBoincSchedulerReadStatus(scheduler, status), "UPLOADING"))
    err = boincErrorCreatef(kBoincError, 1, "workunit should be UPLOADING (not %s)", status);

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "uploadResponse.xml") <= 0)
    err = boincErrorCreate(kBoincError, 2, "workunit uploadResponse.xml should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "out.zip") < 0)
    err = boincErrorCreate(kBoincError, 3, "workunit out.zip should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "in.zip") < 0)
    err = boincErrorCreate(kBoincError, 4, "workunit in.zip should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "workRequest.xml") > 0)
    err = boincErrorCreate(kBoincError, 5, "workunit workResponse.xml should not have been modified");

  cleanupScheduler(scheduler);
  
  return err;
}


BoincError testBoincSchedulerInitialize(void * data)
{
  BoincScheduler * scheduler = initScheduler(computetest);

  testBoincSchedulerWriteStatus(scheduler, "INITIALIZING");

  BoincError err = testBoincSchedulerExecuteUntil(scheduler, kBoincWorkUnitUploading);

  char status[25];

  if (!err && strcmp(testBoincSchedulerReadStatus(scheduler, status), "UPLOADING"))
    err = boincErrorCreatef(kBoincError, 1, "workunit should be UPLOADING (not %s)", status);

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "uploadResponse.xml") <= 0)
    err = boincErrorCreate(kBoincError, 2, "workunit uploadResponse.xml should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "out.zip") < 0)
    err = boincErrorCreate(kBoincError, 3, "workunit out.zip should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "in.zip") < 0)
    err = boincErrorCreate(kBoincError, 4, "workunit in.zip should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "workRequest.xml") < 0)
    err = boincErrorCreate(kBoincError, 5, "workunit workResponse.xml should have been modified");

  cleanupScheduler(scheduler);
  
  return err;
}

BoincError testBoincSchedulerWithoutFile(void * data)
{
  BoincScheduler * scheduler = initScheduler(computetest);

  char file[255];
  testBoincSchedulerGetPath(scheduler, file, "workunit.status");
  unlink(file);

  BoincError err = testBoincSchedulerExecuteUntil(scheduler, kBoincWorkUnitUploading);

  char status[25];

  if (!err && strcmp(testBoincSchedulerReadStatus(scheduler, status), "UPLOADING"))
    err = boincErrorCreatef(kBoincError, 1, "workunit should be UPLOADING (not %s)", status);

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "uploadResponse.xml") <= 0)
    err = boincErrorCreate(kBoincError, 2, "workunit uploadResponse.xml should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "out.zip") < 0)
    err = boincErrorCreate(kBoincError, 3, "workunit out.zip should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "in.zip") < 0)
    err = boincErrorCreate(kBoincError, 4, "workunit in.zip should have been modified");

  if (!err && testBoincSchedulerCompareFileWithStatus(scheduler, "workRequest.xml") < 0)
    err = boincErrorCreate(kBoincError, 5, "workunit workResponse.xml should have been modified");

  cleanupScheduler(scheduler);
  
  return err;
}

BoincError testBoincSchedulerComplete(void * data)
{
  BoincScheduler * scheduler = initScheduler(computetest);

  BoincError err = testBoincSchedulerExecuteUntil(scheduler, kBoincWorkUnitUploading);

  char status[25];

  if (!err && strcmp(testBoincSchedulerReadStatus(scheduler, status), "UPLOADING"))
    err = boincErrorCreatef(kBoincError, 1, "workunit should be UPLOADING (not %s)", status);
  
  cleanupScheduler(scheduler);
  
  return err;
}

BoincError testBoincSchedulerCompleteAsync(void * data)
{
  BoincScheduler * scheduler = initScheduler(computetestAsync);

  BoincError err = testBoincSchedulerExecuteUntil(scheduler, kBoincWorkUnitUploading);

  cleanupScheduler(scheduler);
  
  return err;
}


BoincError testBoincSchedulerCleaning(void * data)
{
  BoincScheduler * scheduler = initScheduler(computetest);
  
  char wupath[255];
  testBoincSchedulerGetPath(scheduler, wupath, "workunit.status");

  BoincError err = testBoincSchedulerExecuteUntil(scheduler, kBoincWorkUnitCompleted);
 
  struct stat filestat;
  if (!err && !stat(wupath, &filestat))
    err = boincErrorCreatef(kBoincError, -1, "status file %s should be destroyed", wupath);
 
  cleanupScheduler(scheduler);
  
  return err;
}

int testBoincScheduler(BoincTest * test) 
{
  boincTestInitMajor(test, "BoincScheduler");
  rmrf(PROJECT_SCHEDULER_TEST_DIR);
  boincTestRunMinor(test, "BoincScheduler run complete", testBoincSchedulerComplete, NULL);
  /*  boincTestRunMinor(test, "BoincScheduler run upload", testBoincSchedulerUpload, NULL);
  boincTestRunMinor(test, "BoincScheduler run compute", testBoincSchedulerCompute, NULL);
  boincTestRunMinor(test, "BoincScheduler run download", testBoincSchedulerDownload, NULL);
  boincTestRunMinor(test, "BoincScheduler run without file", testBoincSchedulerWithoutFile, NULL);*/
  rmrf(PROJECT_SCHEDULER_TEST_DIR);
  boincTestRunMinor(test, "BoincScheduler async run complete", testBoincSchedulerCompleteAsync, NULL);
  boincTestRunMinor(test, "BoincScheduler cleaning", testBoincSchedulerCleaning, NULL);
  boincTestEndMajor(test);
  return 0;
}
