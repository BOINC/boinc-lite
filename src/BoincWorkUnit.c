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
#include "BoincWorkUnit.h"
#include "BoincMem.h"
#include "BoincError.h"
#include "BoincUtil.h"
#include "BoincHash.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>


/*     FileInfo   */

static void boincFileInfoDestroy(BoincFileInfo * fi) 
{
	if (fi == NULL) {
		return;
	}
	if (fi->name) {
		boincFree(fi->name);
		fi->name = NULL;
	}
	if (fi->openname) {
		boincFree(fi->openname);
		fi->openname = NULL;
	}
	if (fi->url) {
		boincFree(fi->url);
		fi->url = NULL;
	}
	if (fi->checksum) {
		boincFree(fi->checksum);
		fi->checksum = NULL;
	}
	if (fi->signature) {
		boincFree(fi->signature);
		fi->signature = NULL;
	}
	if (fi->xmlSignature) {
		boincFree(fi->xmlSignature);
		fi->xmlSignature = NULL;
	}
	boincFree(fi);
}


BoincWorkUnit * boincWorkUnitCreate(const char * workunitDir) 
{
	BoincWorkUnit * wu = boincNew(BoincWorkUnit);
	if (wu == NULL) {
		boincLog(kBoincError, kBoincSystem, "Out of memory");
		return NULL;
	}

	wu->status = kBoincWorkUnitCreated;

	wu->workingDir = boincArray(char, strlen(workunitDir) + 1);
	if (wu->workingDir == NULL) {
		boincFree(wu);
		boincLog(kBoincError, kBoincSystem, "Out of memory");
		return NULL;
	}

	strcpy(wu->workingDir, workunitDir);

	struct stat buffer;
	if (stat(wu->workingDir, &buffer)) {
		boincDebugf("Creating workunit directory: %s", wu->workingDir);
		if (mkdir(wu->workingDir, S_IRWXU)) {
			wu->error = boincErrorCreatef(kBoincFatal, kBoincFileSystem, "Cannot create the workunit directory %d", wu->workingDir);
		}
	}

	return wu;
}

void boincWorkUnitDestroy(BoincWorkUnit* wu) 
{
	if (wu == NULL) {
		return;
	}
	if (wu->name) {
		boincFree(wu->name);
		wu->name = NULL;
	}
	if (wu->workingDir) {
		boincFree(wu->workingDir);
		wu->workingDir = NULL;
	}
	if (wu->app) {
		if (wu->app->name)
			boincFree(wu->app->name);
		wu->app->name = NULL;
		if (wu->app->friendlyName)
			boincFree(wu->app->friendlyName);
		wu->app->friendlyName = NULL;
		if (wu->app->version)
			boincFree(wu->app->version);
		wu->app->version = NULL;
		if (wu->app->apiVersion)
			boincFree(wu->app->apiVersion);
		wu->app->apiVersion = NULL;

		boincFree(wu->app);
		wu->app = NULL;
	}

	if (wu->result) {
		if (wu->result->name) {
			boincFree(wu->result->name);
			wu->result->name = NULL;
		}
		boincFree(wu->result);
		wu->result = NULL;
	}
	if (wu->error) {
		boincErrorDestroy(wu->error);
		wu->error = NULL;
	}

	BoincFileInfo * fi = NULL;
	for(fi = wu->globalFileInfo ; fi ; fi = wu->globalFileInfo) {
		boincDebug("destroy fileInfo");
		wu->globalFileInfo = fi->gnext;
		boincFileInfoDestroy(fi);
	}
	boincFree(wu);
}

void boincWorkUnitGetStatus(BoincWorkUnit* workunit, BoincWorkUnitStatus* status)
{
	status->status = workunit->status;
	status->progress = workunit->progress;
	status->delay = workunit->delay;

	if (workunit->error) {
		status->errorType = boincErrorType(workunit->error);
		status->errorCode = boincErrorCode(workunit->error);
		if (boincErrorMessage(workunit->error) != NULL) {
			snprintf(status->errorMessage, 255, "%s", boincErrorMessage(workunit->error));
			status->errorMessage[255] = 0;
		}
	}
}

int boincWorkUnitSetStatus(BoincWorkUnit* workunit, int status)
{
	int changed = 0;
	if (workunit->status != status) {
		workunit->status = status;
		workunit->progress = 0;
		workunit->dateStatusChange = boincQueueNow();
		changed = 1;
	}
	return changed;
}

void boincWorkUnitSetError(BoincWorkUnit* workunit, BoincError error)
{
	if (workunit->error != NULL) {
		boincErrorDestroy(workunit->error);
		workunit->error = NULL;
	}
	workunit->error = error;
}

char* boincWorkUnitGetPath(BoincWorkUnit * wu, BoincFileInfo * fi, char * resFile, unsigned int resSize) 
{
	// The mutex is not locked because the only moment that the
	// application has a pointer to a workunit is during the computation
	// and the scheduler will not make any changes to the workunit
	// in that state.
	if (fi) {
		return boincGetAbsolutePath(wu->workingDir, fi->openname, resFile, resSize);
	}
	return boincGetAbsolutePath(wu->workingDir, NULL, resFile, resSize);
}

int boincWorkUnitFileExists(BoincWorkUnit * workunit, BoincFileInfo * file) 
{
	if (!file || !file->openname) {
		return 0;
	}

	char path[255];
	boincDebugf("%s %s", workunit->workingDir, file->openname);
	boincWorkUnitGetPath(workunit, file, path, 255);

	struct stat buf;
	if (stat(path, &buf)) {
		boincDebugf("boincProxyFileExists stat said %s does not exist", path);
		return 0;
	}

	return 1; 
}

int boincWorkUnitFileNeedsDownload(BoincWorkUnit * workunit, BoincFileInfo * file) 
{
	if (!boincWorkUnitFileExists(workunit, file)) {
		return 1;
	}
	if (!file->checksum || !strlen(file->checksum)) {
		return 1;
	}

	char hash[33];
	char path[255];
	boincWorkUnitGetPath(workunit, file, path, 255);
	boincErrorDestroy(boincHashMd5File(path, hash));
	if (strcmp(file->checksum, hash)) {
		return 1;
	}

	boincDebug("file %s does not need to be downloaded");
	return 0;
}
