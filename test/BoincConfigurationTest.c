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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "BoincConfigurationTest.h"
//#include "BoincConfiguration.h"
#include "BoincXMLParser.h"
#include "BoincTest.h"
#include <stdlib.h>
#define BUFF_SIZE 255

BoincError testBoincConfigurationInitCleanup(void * data)
{
  char * dir = TESTDIR;
  BoincConfiguration * conf = boincConfigurationCreate(dir);
  BoincError res = NULL;
  char confRes[BUFF_SIZE];
  remove(TMPDIR "config.xml");

  boincConfigurationGetString(conf, kBoincProjectDirectory, confRes, BUFF_SIZE);
  if (!confRes[0]) {
    boincConfigurationDestroy(conf);
    res = boincErrorCreate(kBoincError, 1, "Cannot retrieve project directory from configurator");
    return res;
  }
  if (strcmp(dir, confRes))
    res = boincErrorCreate(kBoincError, 2, "Wrong project directory stored");
  
  boincConfigurationGetString(conf, kBoincProjectConfigurationFile, confRes, BUFF_SIZE);
  if (! confRes[0])
    res = boincErrorCreate(kBoincError, 3, "Configuration file not affected");
  boincConfigurationDestroy(conf);
  return res;
}

typedef struct _BoincTestArg{
  char stringValue[150];
  double numberValue;
  int isRO;
  int index;
  BoincConfiguration * conf;
} BoincTestArg;

BoincError testBoincConfigurationArg(void * data)
{
  BoincTestArg * arg = (BoincTestArg*) data;
  if (!arg->isRO) {
    if (strlen(arg->stringValue)) {
      if (boincErrorOccured(boincConfigurationSetString(arg->conf, arg->index, arg->stringValue)))
	return boincErrorCreate(kBoincError, 1, "Cannot affect string value");
    }else{
      if (boincErrorOccured(boincConfigurationSetNumber(arg->conf, arg->index, arg->numberValue)))
	return boincErrorCreate(kBoincError, 2, "Cannont affect number value");
    }
  }else {
    if (strlen(arg->stringValue)) {
      BoincError err = boincConfigurationSetString(arg->conf, arg->index, arg->stringValue);
      if (!boincErrorOccured(err))
	return boincErrorCreate(kBoincError, 3, "Should not affect string value");
    }else{
      if (!boincErrorOccured(boincConfigurationSetNumber(arg->conf, arg->index, arg->numberValue)))
	return boincErrorCreate(kBoincError, 4, "Should not affect number value");
    }
  }
  if (arg->stringValue[0]) {
    char res[BUFF_SIZE];
    boincConfigurationGetString(arg->conf, arg->index, res, BUFF_SIZE);
    boincDebugf("conf string res: %s", res);
    if (res[0])
      return NULL;
  }
  double res = boincConfigurationGetNumber(arg->conf, arg->index);
  boincDebugf("conf number res: %f", res);
  if (boincConfigurationIsNumber(arg->index))
    return NULL;
  return boincErrorCreate(kBoincError, 5, "Could not retrieve value");
}

BoincError testBoincConfigurationDefaultConf(void * data) 
{
  BoincConfiguration * conf = boincConfigurationCreate( TMPDIR );
  boincConfigurationLoad(conf);
  char file[BUFF_SIZE];
  boincConfigurationGetString(conf, kBoincProjectConfigurationFile, file, BUFF_SIZE);
  char test[BUFF_SIZE];
  boincConfigurationGetString(conf, kBoincHostID, test, BUFF_SIZE);
  if (!test[0])
    return boincErrorCreate(kBoincError, 2, "could not retrieve Host ID");
  struct stat buffer;
  if (!stat(file, &buffer))
    return boincErrorCreatef(kBoincError, 3, "conf file %s should not exist", file);
  boincConfigurationStore(conf);
  if (stat(file, &buffer)) {
    return boincErrorCreatef(kBoincError, 4, "conf file %s should be created", file);
  }
  remove(file);
  boincConfigurationDestroy(conf);
  if (buffer.st_size < 2)
    return boincErrorCreatef(kBoincError, 5, "conf file %s should not be empty", file);
  return 0;
}

BoincError testBoincConfigurationWrongArg(void * data) 
{
  BoincTestArg * arg = (BoincTestArg *) data;
  if  (strlen(arg->stringValue)) {
    if (boincErrorOccured(boincConfigurationSetString(arg->conf, arg->index, arg->stringValue)))
      return NULL;
    return boincErrorCreate(kBoincError, 1, "Should not affect wrong string value");
  }
  if (boincErrorOccured(boincConfigurationSetNumber(arg->conf, arg->index, arg->numberValue)))
    return NULL;
  return boincErrorCreate(kBoincError, 2, "Should no affected wrong number value");
}

BoincError testBoincConfigurationStore(void * data) {
  BoincConfiguration * conf = boincConfigurationCreate(TMPDIR);
  boincConfigurationLoad(conf);
  char * str = "url://inutile";
  boincConfigurationSetString(conf, kBoincProjectURL, str);
  boincConfigurationStore(conf);
  boincConfigurationDestroy(conf);
  conf = boincConfigurationCreate(TMPDIR);
  char file[BUFF_SIZE];
  boincConfigurationGetString(conf, kBoincProjectConfigurationFile, file, BUFF_SIZE);
  boincConfigurationLoad(conf);
  BoincError res = NULL;
  char test[BUFF_SIZE];
  boincConfigurationGetString(conf, kBoincProjectURL, test, BUFF_SIZE);
  if (!test[0])
    return boincErrorCreate(kBoincError, 1, "Should retrieve project url from default conf");
  if (strcmp(str, test))
    res = boincErrorCreatef(kBoincError, 2, "Should retrieve the same url project (%s, %s)", str, test);
  boincDebugf("test file: %s", file);
  remove(file);
  boincConfigurationDestroy(conf);
  return res;
}

BoincError testBoincConfigurationCorrupted(void * data) {
  BoincTestArg * arg = (BoincTestArg *) data;
  BoincError err = boincConfigurationStore(arg->conf);
  if (err)
    return err;
  system("head -n 5 /tmp/config.xml > /tmp/config1.xml ; mv /tmp/config1.xml /tmp/config.xml");
  BoincConfiguration * conf = boincConfigurationCreate(TMPDIR);
  err = boincConfigurationLoad(conf);
  remove("/tmp/config.xml");
  boincConfigurationDestroy(conf);
  if (boincErrorOccured(err)) {
    return NULL;
  }
  return boincErrorCreate(kBoincError, 1, "the xml config was corruped, no error detected :(");
}

int testBoincConfiguration(BoincTest * test) {
  boincTestInitMajor(test, "BoincConfiguration");
  boincTestRunMinor(test, "BoincConfigurationInit", testBoincConfigurationInitCleanup, NULL);

  //Test default loading
  boincTestRunMinor(test, "BoincConfigurationDefaultConf", testBoincConfigurationDefaultConf, NULL);
  
  BoincTestArg arg;
  arg.isRO = 0;
  arg.stringValue[0] = 0;
  arg.conf = boincConfigurationCreate(TMPDIR);

  //Test wrong type
  arg.index = kBoincProjectURL;
  arg.stringValue[0] = 0 ; arg.numberValue = 12;
  boincTestRunMinor(test, "BoincConfiguration test number value on a string one", testBoincConfigurationWrongArg , (void*) &arg);
  arg.index = kBoincHostCpuIops;
  arg.stringValue[0] = 0 ; strcpy(arg.stringValue, "http://www.csl.sony.fr/");
boincTestRunMinor(test, "BoincConfiguration test string value on a number one", testBoincConfigurationWrongArg , (void*) &arg);
  //Test each arg type 
  arg.index = kBoincProjectURL;
  arg.stringValue[0] = 0 ; strcpy(arg.stringValue, "http://www.csl.sony.fr/");
  boincTestRunMinor(test, "BoincConfiguration kBoincProjectURL", testBoincConfigurationArg , (void*) &arg);
  arg.index = kBoincHostID;
  arg.stringValue[0] = 0 ; strcpy(arg.stringValue, "u123");
  boincTestRunMinor(test, "BoincConfiguration kBoincHostID", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostID;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincHostID", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincUserEmail;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincUserID", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincUserFullname;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincUserName", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincUserPassword;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincUserPassword", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincUserAuthenticator;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincUserAuthenticator", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincUserTeamName;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincUserTeamName", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostDomainName;
  arg.isRO = 1;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincHostDomainName", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostIpAddress;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincHostIpAddress", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostCpuVendor;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincHostCpuVendor", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostCpuModel;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincHostCpuModel", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostCpuFeatures;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincHostCpuFeatures", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostOSName; 
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincHostOSName", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostOSVersion;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincHostOSVersion", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostAccelerators;
  strcpy(arg.stringValue, "123456");
  arg.isRO = 0;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostAccelerators", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHttpProxy;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincHttpProxy", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincProjectDirectory;
  arg.isRO = 1;
  strcpy(arg.stringValue, "123456");
  boincTestRunMinor(test, "BoincConfiguration kBoincProjectDirectory", testBoincConfigurationArg, (void*) &arg);
  arg.isRO = 0;
  arg.index = kBoincUserTotalCredit;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincUserTotalCredit", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostTimezone;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostTimezone", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostNCpu;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  arg.isRO = 1;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostNCpu", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostCpuFpops;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostCpuFpops", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostCpuIops;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostCpuIops", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostCpuMemBW;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostCpuMemBW", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostCpuCalculated;
  arg.isRO = 0;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostCpuCalculated", testBoincConfigurationArg, (void*) &arg);
  arg.isRO = 1;
  arg.index = kBoincHostMemBytes;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostMemBytes", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostMemCache;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostMemCache", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostMemSwap;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostMemSwap", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostMemTotal;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostMemTotal", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostDiskTotal;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostDiskTotal", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincHostDiskFree;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincHostDiskFree", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincNetworkUpBandwidth;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincNetworkUpBandwidth", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincNetworkUpAverage;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincNetworkUpAverage", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincNetworkUpAvgTime;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincNetworkUpAvgTime", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincNetworkDownBandwidth;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincNetworkDownBandwidth", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincNetworkDownAverage;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincNetworkDownAverage", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincNetworkDownAvgTime;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincNetworkDownAvgTime", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincTotalDiskUsage;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincTotalDiskUsage", testBoincConfigurationArg, (void*) &arg);
  arg.index = kBoincProjectDiskUsage;
  arg.stringValue[0] = 0; arg.numberValue = 1.23;
  boincTestRunMinor(test, "BoincConfiguration kBoincProjectDiskUsage", testBoincConfigurationArg, (void*) &arg);
  boincTestRunMinor(test, "BoincConfiguration store conf file", testBoincConfigurationStore, NULL);
  boincTestRunMinor(test, "BoincConfiguration load corrupted xml conf file", testBoincConfigurationCorrupted, (void*)&arg);
  boincConfigurationDestroy(arg.conf);
  boincTestEndMajor(test);
  return 0;
}
