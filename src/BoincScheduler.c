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
#include "BoincLite.h"
#include "BoincMem.h"
#include "BoincError.h"
#include "BoincUtil.h"
#include "BoincProxy.h"
#include "BoincMutex.h"
#include "BoincHash.h"
#include "BoincWorkUnit.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#define WORKUNIT_SUBDIR "%d"
#define STATUS_FILENAME "workunit.status"


struct _BoincScheduler {
	BoincConfiguration * configuration;
	BoincProxy * proxy;
	int nbworkunits;
	BoincWorkUnit ** workunits;
	void * callback;
	void * userData;
	BoincQueue * queue;
	BoincStatusChangedCallback statusCallback;
	void *statusData;
	BoincMutex mutex;
};

//////////////////////////////////////////////////////////////

/*     BoincStatusInfo   */

typedef struct _BoincStatusInfo {
  int code;
  char* label;
  int storable;
} BoincStatusInfo;

static BoincStatusInfo BoincStatusInfos[] = {
  {kBoincWorkUnitCreated, "CREATE", 0},           //0
  {kBoincWorkUnitInitializing, "INITIALIZING", 0},//1
  {kBoincWorkUnitDownloading, "DOWNLOADING", 1},  //2
  {kBoincWorkUnitWaiting, "WAITING", 1},          //3
  {kBoincWorkUnitComputing,"COMPUTING", 1},       //4
  {kBoincWorkUnitFinished,"FINISHED", 1},         //5
  {kBoincWorkUnitUploading,"UPLOADING", 1},       //6
  {kBoincWorkUnitCompleted,"COMPLETED", 1},       //7
  {kBoincWorkUnitLoading, "LOADING", 0},          //8
  {kBoincWorkUnitFailed, "FAILED", 0},            //9
};


/*     BoincEvent   */

typedef struct _BoincEvent {
	int eventType;
	int index;
	unsigned int nbtries;
} BoincEvent;

static BoincEvent * boincEventCreate(int eventType, int index)
{
	BoincEvent * event = boincNew(BoincEvent);
	event->eventType = eventType;
	event->index = index;
	return event;
}

//////////////////////////////////////////////////////////////

/* BoincScheduler */

BoincScheduler* boincSchedulerCreate(BoincConfiguration* config, BoincComputeWorkUnitCallback callback, void* userData)
{
	boincDebug("scheduler create");
	BoincScheduler * scheduler = boincNew(BoincScheduler);
	if (scheduler == NULL) {
		boincLog(kBoincError, -1, "Out of memory");
		return NULL;
	}
	scheduler->configuration = config;
	scheduler->callback = callback;
	scheduler->userData = userData;
	scheduler->nbworkunits = 2;
	scheduler->queue = boincQueueCreate();
	scheduler->mutex = boincMutexCreate("sched");

	if (scheduler->queue == NULL) {
		boincFree(scheduler);
		boincLog(kBoincError, -1, "Out of memory");
		return NULL;
	}

	scheduler->workunits = (BoincWorkUnit**) boincArray(BoincWorkUnit, scheduler->nbworkunits);

	boincDebug("create -> event -1");
	boincQueuePut(scheduler->queue, 0, boincEventCreate(-1, 0));

	return scheduler;
}

void boincSchedulerDestroy(BoincScheduler* scheduler)
{
	if (scheduler->mutex != NULL) {
		boincMutexLock(scheduler->mutex);
		boincMutexUnlock(scheduler->mutex);
		boincMutexDestroy(scheduler->mutex);
		scheduler->mutex = NULL;
	}

	boincDebug("scheduler destroy");
	if (scheduler->proxy) {
		boincProxyDestroy(scheduler->proxy);
		scheduler->proxy = NULL;
	}

	if (scheduler->workunits) {
		for (int i = 0 ; i < scheduler->nbworkunits; i++) {
			if (scheduler->workunits[i] != NULL) {
				boincWorkUnitDestroy(scheduler->workunits[i]);
				scheduler->workunits[i] = NULL;
			}
		}
		boincFree(scheduler->workunits);
		scheduler->workunits = NULL;
	}

	boincQueueDestroy(scheduler->queue);

	boincFree(scheduler);
}


static void boincSchedulerSetWorkUnitError(BoincScheduler * scheduler, BoincWorkUnit* workunit, BoincError error)
{
	boincLogError(error);

	boincMutexLock(scheduler->mutex);

	boincWorkUnitSetError(workunit, error);

	boincMutexUnlock(scheduler->mutex);

	if (scheduler->statusCallback != NULL) {
		scheduler->statusCallback(scheduler->statusData, workunit->index);
	}
}

BoincError boincSchedulerSetStatusChangedCallback(BoincScheduler * scheduler, 
						  BoincStatusChangedCallback callback, void *userData)
{
	scheduler->statusCallback = callback;
	scheduler->statusData = userData;
	if (scheduler->proxy != NULL) {
		boincProxySetStatusChangedCallback(scheduler->proxy, callback, userData);
	}
	return NULL;
}

static int boincSchedulerGetWorkUnitIndex(BoincScheduler * scheduler, BoincWorkUnit* wu)
{
	return wu->index;
}


static BoincError boincSchedulerInitializeWorkUnit(BoincScheduler* scheduler, BoincWorkUnit* wu)
{
	boincDebug("retrieve wu");

	boincDebug("Requesting workunit from server");

	// Get the data from the server

	BoincProxyWorkUnit pwu = {scheduler->proxy, wu};
	BoincError err = boincProxyRequestWork(&pwu);
	if (err != NULL) {
		return err;
	}

	// Tell the scheduler the workunit is ready for the download
	err = boincQueuePut(scheduler->queue, boincQueueNow(), boincEventCreate(kBoincWorkUnitDownloading, 
										boincSchedulerGetWorkUnitIndex(scheduler, wu)));
	if (err != NULL) {
		return err;
	}

	boincLogf(6, 1, "New workunit: id=xxx, name=%s", wu->name);

	return NULL;
}

static BoincError boincSchedulerDownload(BoincScheduler* scheduler, BoincWorkUnit* wu)
{
	boincDebug("scheduler download");
	if (wu == NULL)
		return boincErrorCreate(kBoincError, 1, "boincSchedulerDownload: workunit null :(");

	// Do the thing

	BoincFileInfo * fi;
	BoincError err = NULL;
	int filesDownloaded = 0;
	int filesTotal = 0;

	// Only download the workunit files. The application files
	// must be provided by the parent application
	for (fi = wu->fileInfo ; fi ; fi = fi->next) {
		if (fi->flags & kBoincFileGenerated)
			continue;
		filesTotal++;
	}

	for (fi = wu->fileInfo ; fi ; fi = fi->next) {

		if (fi->flags & kBoincFileGenerated) {
			continue;
		}
		if (!boincWorkUnitFileNeedsDownload(wu, fi)) {
			filesDownloaded++;
			continue;
		}

		BoincProxyWorkUnit pwu = {scheduler->proxy, wu};
		err = boincProxyDownloadFile(&pwu, fi, filesDownloaded, filesTotal);
		if (err) {
			return err;
		} 

		filesDownloaded++;
	}

	boincLogf(6, 1, "%d files downloaded", filesDownloaded);
	
	// Inform the scheduler that the download is finished

	err = boincQueuePut(scheduler->queue, boincQueueNow(), boincEventCreate(kBoincWorkUnitWaiting, 
										boincSchedulerGetWorkUnitIndex(scheduler, wu)));
	if (err != NULL) {
		return err;
	}

	return NULL;
}

static BoincError boincSchedulerCompute(BoincScheduler* scheduler, BoincWorkUnit* wu)
{
	boincDebug("scheduler compute");

	// Send the workunit to the parent application. Good luck.

	BoincComputeWorkUnitCallback compute_callback = (BoincComputeWorkUnitCallback) scheduler->callback;
	wu->result->cpu_time = time(NULL);
	compute_callback(scheduler, wu, scheduler->userData);

	return NULL;
}

static BoincError boincSchedulerUpload(BoincScheduler* scheduler, BoincWorkUnit* wu)
{
	boincDebug("scheduler upload");

	// Transfer the bits

	BoincFileInfo * fi;
	BoincError err;
	int fileNum = 0;
	int filesTotal = 0;

	for (fi = wu->result->fileInfo ; fi ; fi = fi->next) {
		if (! (fi->flags & kBoincFileGenerated) || ! (fi->flags & kBoincFileUpload)) {
			continue;
		}
		filesTotal++;
	}

	for (fi = wu->result->fileInfo ; fi ; fi = fi->next) {
		if (! (fi->flags & kBoincFileGenerated) || ! (fi->flags & kBoincFileUpload))
			continue;

		BoincProxyWorkUnit pwu = { scheduler->proxy, wu };

		err = boincProxyUploadFile(&pwu, fi, fileNum, filesTotal);
		if (err) {
			return err;
		}
		
		fileNum++;
	}

	boincLogf(6, 1, "%d file(s) uploaded", fileNum);

	// Tell the scheduler that he can erase our friend.

	err = boincQueuePut(scheduler->queue, boincQueueNow(), boincEventCreate(kBoincWorkUnitCompleted, 
										boincSchedulerGetWorkUnitIndex(scheduler, wu)));
	if (err != NULL) {
		return err;
	}

	return NULL;
}

static BoincError boincSchedulerErase(BoincScheduler* scheduler, int index)
{
	boincDebug("scheduler erase");
	BoincError err = NULL;
      
	BoincWorkUnit * oldwu = scheduler->workunits[index];

	// Erase the files of the workunit
	if ((err = boincRemoveDirectory(oldwu->workingDir)) != NULL) {
		return err;
	}
       
	// Create the new workunit
	BoincWorkUnit * newwu = boincWorkUnitCreate(oldwu->workingDir);
	if (newwu == NULL) {
		return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
	}
	newwu->index = index;
	
	// Store the workunit pointer
	boincMutexLock(scheduler->mutex);	
	scheduler->workunits[index] = newwu;
	boincMutexUnlock(scheduler->mutex);

	// Destroy the old workunit
	boincWorkUnitDestroy(oldwu);

	// Tell the scheduler
	err = boincQueuePut(scheduler->queue, boincQueueNow(), boincEventCreate(kBoincWorkUnitCreated, index));
	if (err != NULL) {
		return err;
	}

	return NULL;
}

static BoincError boincSchedulerReadWorkUnitStatus(BoincScheduler* scheduler, BoincWorkUnit * wu)
{
	int statusfile_len = strlen(wu->workingDir) + 1 + strlen(STATUS_FILENAME) + 1;
	char * statusfile = boincArray(char, statusfile_len + 1);
	initFilename(statusfile, statusfile_len, wu->workingDir);
	appendFilename(statusfile, statusfile_len, STATUS_FILENAME);

	boincDebugf("Checking for status file %s", statusfile);

	struct stat buffer;
	if (stat(statusfile, &buffer)) {
		boincDebugf("Status file not found, skipping: %s", statusfile);
		boincFree(statusfile);
		return NULL; // Not an error
	}

	FILE * fh = boincFopen(statusfile, "r");
	if (!fh) {
		boincFree(statusfile);
		return boincErrorCreatef(kBoincError, kBoincFileSystem, "Cannot read file %s", statusfile);
	}
	
	char status[20];
	int r = fscanf(fh, "%s", status);
	
	boincFclose(fh);
	boincFree(statusfile);

	if (r == 0) {
		return boincErrorCreate(kBoincError, kBoincFileSystem, "Failed to read the status");
	}

	boincDebugf("Found status %s", status);

	wu->status = 0;
	for (int i = 0; i < kBoincWorkUnitLastStatus; i++) {
		if (!strcmp(status, BoincStatusInfos[i].label)) {
			wu->status = i;
			boincDebug("Status loaded");
			break;
		}
	}

	return NULL;
}

static BoincError boincSchedulerLoad(BoincScheduler* scheduler)
{
	boincDebug("load");
	BoincError err = NULL;


	boincMutexLock(scheduler->mutex);

	// Create the proxy
	scheduler->proxy = boincProxyCreate(scheduler->configuration);
	if (scheduler->statusCallback)
		boincSchedulerSetStatusChangedCallback(scheduler, scheduler->statusCallback, scheduler->statusData);

	if (scheduler->proxy == NULL) {
		err = boincErrorCreate(kBoincFatal, -1, "Out of memory");
		goto unlock_and_return;
	}
  
	err = boincProxyInit(scheduler->proxy);
	if (err) {
		goto unlock_and_return;
	}

	// Authenticate for the current session, if necessary
	if (!boincConfigurationHasParameter(scheduler->configuration, kBoincUserAuthenticator)) {
		err = boincProxyAuthenticate(scheduler->proxy);
		if (err) {
			goto unlock_and_return;
		}
	}

	for (int index = 0; index < scheduler->nbworkunits; index++) {

		// Try to load the single, pre-defined workunit  
		char wu_path[255];
		char subdir[50];

		boincConfigurationGetString(scheduler->configuration, kBoincProjectDirectory, wu_path, 255);
		sprintf(subdir, WORKUNIT_SUBDIR, index);
		appendFilename(wu_path, 255, subdir);

		// Create a new workunit.
		BoincWorkUnit * wu = boincWorkUnitCreate(wu_path);
		if (wu == NULL) {
			err = boincErrorCreate(kBoincFatal, -1, "Out of memory");
			goto unlock_and_return;
		}
		scheduler->workunits[index] = wu;
		wu->index = index;

		// Try loading the status from disk
		err = boincSchedulerReadWorkUnitStatus(scheduler, wu);
		
		if ((err == NULL) && (wu->status >= kBoincWorkUnitDownloading)) {
			err = boincProxyLoadWorkUnit(wu);
		}

		if (err) {
			// Because there is no a recovery scheme, the
			// workunit and its directory is
			// erased . This avoids falling in the same error situation
			// the next time the application starts.
			// FIXME: there should be a way to inform the server about
			// these errors.
			err = boincRemoveDirectory(wu_path);
			if (err)
				goto unlock_and_return; // If the directory could not removed, the application is in a bad situation

			boincWorkUnitDestroy(wu);

			wu = boincWorkUnitCreate(wu_path);
			if (wu == NULL) {
				err = boincErrorCreate(kBoincFatal, -1, "Out of memory");
				goto unlock_and_return;
			}
			scheduler->workunits[index] = wu;
			wu->index = index;

		} else {
			boincDebugf("Workunit %d is now loaded", index);
		}
		
		int t = (wu->status == kBoincWorkUnitComputing)? 0 : boincQueueNow(); // FIXME: a hack to put computing workunits first in queue
		
		// Post an event to get the workunit started.
		boincQueuePut(scheduler->queue, t, boincEventCreate(wu->status, index));
	}

unlock_and_return:

	boincMutexUnlock(scheduler->mutex);

	
	// Communicate the status to the parent application.
	// (This has to be done with the mutex unlocked.)
	if (!err) {
		for (int index = 0; index < scheduler->nbworkunits; index++) {
			if (scheduler->statusCallback != NULL) {
				scheduler->statusCallback(scheduler->statusData, index);
			}
		}
	}

	return err;
}

BoincError boincSchedulerChangeWorkUnitStatus(BoincScheduler* scheduler, BoincWorkUnit* workunit, int status)
{
	int index = boincSchedulerGetWorkUnitIndex(scheduler, workunit);
	boincDebugf("boincSchedulerChangeWorkUnitStatus: index %d, status %d", index, status);
	return boincQueuePut(scheduler->queue, boincQueueNow(), boincEventCreate(status, index));
}

BoincError boincSchedulerSetWorkUnitProgress(BoincScheduler * scheduler, BoincWorkUnit* workunit, float progress)
{
	BoincProxyWorkUnit pwu = {scheduler->proxy, workunit};
	BoincError err = boincProxyChangeWorkUnitProgress(&pwu, progress);
	return err;
}

/**
 * Regenerate an event with delay
 *
 * If the max tries is reached, the event is destroyed
 */
static BoincError boincSchedulerRegenerate(BoincScheduler * sched, int delay, BoincEvent * event, int maxtries, BoincError error)
{
	event->nbtries++;
  
	if (delay <= 0) {
		delay = 10 + event->nbtries * 10; // make sure there's a minimum delay
	}

	if ((maxtries > 0) && (event->nbtries == maxtries)) {
		boincFree(event);
                if (error != NULL) {
                        BoincError r = boincErrorCreate(kBoincFatal, 
                                                        boincErrorCode(error), 
                                                        boincErrorMessage(error));
                        boincErrorDestroy(error);
                        return r;
                } else {
                        return boincErrorCreate(kBoincFatal, 0, "Maximum number of tries exceeded");
                }
	}
	return boincQueuePut(sched->queue, boincQueueNow() + delay, event);
}

static BoincError boincSchedulerSetWorkUnitStatus(BoincScheduler * scheduler, int index, int status)
{
	if ((index < 0) && (index >= scheduler->nbworkunits)) {
		return boincErrorCreatef(kBoincError, kBoincInternal, "Index out of bounds: %d", index);
	}

	BoincWorkUnit * wu = NULL;
	BoincError err = NULL;
	int storeStatus = 0;

	boincMutexLock(scheduler->mutex);

	wu = scheduler->workunits[index];

	storeStatus = boincWorkUnitSetStatus(wu, status);

	if (storeStatus && BoincStatusInfos[status].storable) {
		
		// This section should not be interrupted 

		char tmp_status_file[255];
		initFilename(tmp_status_file, 255, wu->workingDir);
		appendFilename(tmp_status_file, 255, "." STATUS_FILENAME);
		
		boincDebugf("Opening temporary status file: %s", tmp_status_file);
		
		FILE * fh = boincFopen(tmp_status_file, "w+");
		if (!fh) {
			err = boincErrorCreatef(kBoincError, kBoincFileSystem, "Cannot open %s (w+)", tmp_status_file);
		}

		if (err == NULL) {
			int len = strlen(BoincStatusInfos[status].label);
			if (fwrite(BoincStatusInfos[status].label, len, 1, fh) != 1) {
				err = boincErrorCreatef(kBoincError, kBoincFileSystem, "Failed to write the status");
			}
			boincFclose(fh);
		}

		if (err == NULL) {
		
			char status_file[255];
			initFilename(status_file, 255, wu->workingDir); 
			appendFilename(status_file, 255, STATUS_FILENAME);
		
			boincDebugf("Renaming temporary status file to %s", status_file);
			
			err = boincRenameFile(tmp_status_file, status_file);
		}

		//
	}

	boincMutexUnlock(scheduler->mutex);

	if (scheduler->statusCallback != NULL) {
		scheduler->statusCallback(scheduler->statusData, wu->index);
	}

	return NULL;
}

static int boincSchedulerCountDownloading(BoincScheduler * scheduler)
{
	int num = 0;

	boincMutexLock(scheduler->mutex);

	for (int i = 0; i < scheduler->nbworkunits; i++) {
		if (scheduler->workunits[i] != NULL) {
			if ((scheduler->workunits[i]->status == kBoincWorkUnitInitializing) 
			    || (scheduler->workunits[i]->status == kBoincWorkUnitDownloading) 
			    || (scheduler->workunits[i]->status == kBoincWorkUnitWaiting)) { 
				num++;
			}
		}
	}

	boincMutexUnlock(scheduler->mutex);

	return num;
}

static int boincSchedulerCountComputing(BoincScheduler * scheduler)
{
	int num = 0;

	boincMutexLock(scheduler->mutex);

	for (int i = 0; i < scheduler->nbworkunits; i++) {
		if (scheduler->workunits[i] != NULL) {
			if ((scheduler->workunits[i]->status == kBoincWorkUnitComputing)) { 
				num++;
			}
		}
	}

	boincMutexUnlock(scheduler->mutex);

	return num;
}

static int boincSchedulerCountUploading(BoincScheduler * scheduler)
{
	int num = 0;

	boincMutexLock(scheduler->mutex);

	for (int i = 0; i < scheduler->nbworkunits; i++) {
		if (scheduler->workunits[i] != NULL) {
			if (scheduler->workunits[i]->status == kBoincWorkUnitUploading) { 
				num++;
			}
		}
	}

	boincMutexUnlock(scheduler->mutex);

	return num;
}

BoincError boincSchedulerCompleted(BoincScheduler * scheduler, 
                                   BoincWorkUnit* workunit, 
                                   unsigned int failed)
{
        BoincError err = boincSchedulerSetWorkUnitStatus(scheduler, 
                                                         workunit->index, 
                                                         kBoincWorkUnitCompleted);
        if (err != NULL) {
                return err;
        }
        
        BoincProxyWorkUnit pwu = {scheduler->proxy, workunit};
        err = boinProxyWorkUnitExecuted(&pwu, failed);
        
        if (err != NULL) {
                return err;
        }
        
        err = boincSchedulerErase(scheduler, workunit->index);
        return err;
}

int boincSchedulerHasEvents(BoincScheduler* scheduler)
{
	return boincQueueHasEvent(scheduler->queue);
}

BoincError boincSchedulerHandleEvents(BoincScheduler* scheduler)
{
	boincDebug("handle events");

	BoincError err = NULL;
	BoincEvent * event = (BoincEvent*) boincQueueGet(scheduler->queue);
  
	BoincWorkUnit * wu = NULL;
	int index = -1;

	if (event->eventType != -1) {
		index = event->index;
		wu = scheduler->workunits[index];
	}

	if (event) {
    
		boincDebugf("Deal event %d (i:%d)", event->eventType, event->index);
		//boincDebugSchedulerStates(scheduler);

		switch (event->eventType) {
      
		case -1:
			err = boincSchedulerLoad(scheduler);
			if (err) {
				// If it's fatal, don't battle with destiny. 
				if (boincErrorType(err) == kBoincFatal) {
					boincFree(event);
					return err;
				}
	
				// If it's not a fatal error, try again
				boincLogError(err);
	
				boincDebug("loading regenerate");
				return boincSchedulerRegenerate(scheduler, 15, event, 4, err);

			} else {
				boincFree(event);
			}
			break;
      
		case kBoincWorkUnitCreated:
      
			// Set the state to created
			err = boincSchedulerSetWorkUnitStatus(scheduler, index, kBoincWorkUnitCreated);
			if (err != NULL) {
				boincSchedulerSetWorkUnitError(scheduler, wu, err);
				return boincSchedulerRegenerate(scheduler, 0, event, 0, NULL);
			}

			if (boincSchedulerCountDownloading(scheduler) == 0) {

				err = boincSchedulerSetWorkUnitStatus(scheduler, index, kBoincWorkUnitInitializing);
				if (err != NULL) {
					boincSchedulerSetWorkUnitError(scheduler, wu, err);
					// The status could probably not be written : retry unfinitely
					return boincSchedulerRegenerate(scheduler, 0, event, 0, NULL);
				}
				
				boincFree(event);
				return boincQueuePut(scheduler->queue, boincQueueNow(), boincEventCreate(kBoincWorkUnitInitializing, index));

			} else {
				return boincSchedulerRegenerate(scheduler, 5, event, 0, NULL);
			}
			break;
      
		case kBoincWorkUnitInitializing:
            
			err = boincSchedulerInitializeWorkUnit(scheduler, wu);
			if (err != NULL) {
				boincSchedulerSetWorkUnitError(scheduler, wu, err);
				return boincSchedulerRegenerate(scheduler, boincErrorDelay(err), event, 0, NULL);

			} 
			boincFree(event);
			break;
      
		case kBoincWorkUnitDownloading:

			// Set the state to downloading
			err = boincSchedulerSetWorkUnitStatus(scheduler, index, kBoincWorkUnitDownloading);
			if (err != NULL) {
				boincSchedulerSetWorkUnitError(scheduler, wu, err);
				return boincSchedulerRegenerate(scheduler, 0, event, 0, NULL);
			}
      
			err = boincSchedulerDownload(scheduler, scheduler->workunits[index]);
			if (err) {
				boincSchedulerSetWorkUnitError(scheduler, wu, err);
				return boincSchedulerRegenerate(scheduler, boincErrorDelay(err), event, 0, NULL);

			} 
			boincFree(event);
			break;
      
		case kBoincWorkUnitWaiting:
      
			// Set the state to waiting
			err = boincSchedulerSetWorkUnitStatus(scheduler, index, kBoincWorkUnitWaiting);
			if (err != NULL) {
				boincSchedulerSetWorkUnitError(scheduler, wu, err);
				return boincSchedulerRegenerate(scheduler, 0, event, 0, NULL);
			}
      
			// If no workunit is computing, promote this one
			if (boincSchedulerCountComputing(scheduler) == 0) {
				// Set the state to computing
				err = boincSchedulerSetWorkUnitStatus(scheduler, index, kBoincWorkUnitComputing);
				if (err != NULL) {
					boincSchedulerSetWorkUnitError(scheduler, wu, err);
					return boincSchedulerRegenerate(scheduler, 0, event, 0, NULL);
				}
				boincFree(event);
				return boincQueuePut(scheduler->queue, boincQueueNow(), boincEventCreate(kBoincWorkUnitComputing, index));
			} else {
				// Otherwise, resend an event
				return boincSchedulerRegenerate(scheduler, 5, event, 0, NULL);
			}
			break;
      
		case kBoincWorkUnitComputing:
            
			err = boincSchedulerCompute(scheduler, scheduler->workunits[index]);
			if (err) {
				boincSchedulerSetWorkUnitError(scheduler, wu, err);
				return boincSchedulerRegenerate(scheduler, boincErrorDelay(err), event, 0, NULL);

			} 
			boincFree(event);
			break;
      
		case kBoincWorkUnitFinished:
      
			err = boincSchedulerSetWorkUnitStatus(scheduler, index, kBoincWorkUnitFinished);
			wu->result->cpu_time = time(NULL) - wu->result->cpu_time;
			if (err != NULL) {
				boincSchedulerSetWorkUnitError(scheduler, wu, err);
				return boincSchedulerRegenerate(scheduler, 0, event, 0, NULL);
			}
      
			// If no workunit is uploading, start this one
			if (boincSchedulerCountUploading(scheduler) == 0) {

				err = boincSchedulerSetWorkUnitStatus(scheduler, index, kBoincWorkUnitUploading);
				if (err != NULL) {
					boincSchedulerSetWorkUnitError(scheduler, wu, err);
					return boincSchedulerRegenerate(scheduler, 0, event, 0, NULL);
				}
				boincFree(event);
				return boincQueuePut(scheduler->queue, boincQueueNow(), boincEventCreate(kBoincWorkUnitUploading, index));

			} else {
				// Sorry
				return boincSchedulerRegenerate(scheduler, 5, event, 0, NULL);
			}
			break;

		case kBoincWorkUnitUploading:
            
			err = boincSchedulerUpload(scheduler, scheduler->workunits[index]);
			if (err) {
				if ((boincErrorCode(err) == kBoincFileSystem) 
				    && (event->nbtries >= 10)) {
					err = boincSchedulerSetWorkUnitStatus(scheduler, index, kBoincWorkUnitFailed);
					if (err != NULL) {
						boincSchedulerSetWorkUnitError(scheduler, wu, err);
						return boincSchedulerRegenerate(scheduler, 0, event, 0, NULL);
					}
					return boincQueuePut(scheduler->queue, boincQueueNow(), boincEventCreate(kBoincWorkUnitFailed, index));
				} 
				boincSchedulerSetWorkUnitError(scheduler, wu, err);
				return boincSchedulerRegenerate(scheduler, boincErrorDelay(err), event, 0, NULL);
			} 
			boincFree(event);
			break;
      
		case kBoincWorkUnitFailed:
		case kBoincWorkUnitCompleted:
      
                        err = boincSchedulerCompleted(scheduler, wu, (event->eventType == kBoincWorkUnitFailed));
                        if (err != NULL) {
                                boincSchedulerSetWorkUnitError(scheduler, wu, err);
                                return boincSchedulerRegenerate(scheduler, 0, event, 0, NULL);
                        }
                        
                        if (event->eventType == kBoincWorkUnitCompleted) {
                                int totalWorkunits = 1 + boincConfigurationGetNumber(scheduler->configuration, 
                                                                                     kBoincUserTotalWorkunits); 
                                boincConfigurationSetNumber(scheduler->configuration, 
                                                            kBoincUserTotalWorkunits, 
                                                            totalWorkunits);
                                return boincConfigurationStore(scheduler->configuration);
                        }
                        
                        boincFree(event);
                        break;    
		}
	}

	return err;
}

int boincSchedulerCountWorkUnits(BoincScheduler* scheduler)
{
	return scheduler->nbworkunits;
}

// Only used in BoincSchedulerTest.c
BoincWorkUnit* boincSchedulerGetWorkUnit(BoincScheduler* scheduler, int index)
{
	if ((index < 0) || (index >= boincSchedulerCountWorkUnits(scheduler)))
		return NULL;
	return scheduler->workunits[index];
}

BoincConfiguration* boincSchedulerGetConfiguration(BoincScheduler* scheduler)
{
	return scheduler->configuration;
}

void boincSchedulerGetWorkUnitStatus(BoincScheduler* scheduler, BoincWorkUnitStatus* status, int index)
{
	memset(status, 0, sizeof(BoincWorkUnitStatus));

	if ((index < 0) || (index >= boincSchedulerCountWorkUnits(scheduler)))
		return;

	boincMutexLock(scheduler->mutex);

	boincWorkUnitGetStatus(scheduler->workunits[index], status);

	boincMutexUnlock(scheduler->mutex);
}


BoincError boincSchedulerChangeAuthentication(BoincScheduler* scheduler, const char* email, const char* password)
{
        return boincProxyChangeAuthentication(scheduler->proxy, email, password);
}
