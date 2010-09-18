/*
  BoincLite, a light-weight library for BOINC clients.

  Copyright (C) 2008 Sony Computer Science Laboratory Paris 
  Authors: Tangui Morlier, Peter Hanappe, Anthony Beurive

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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "BoincConfigurationOSCallbacks.h"
#include "BoincXMLParser.h"
#include "BoincMem.h"
#include "BoincUtil.h"
#include "BoincError.h"
#include "BoincMutex.h"

typedef struct _BoincConfigurationParameter BoincConfigurationParameter;

struct _BoincConfigurationParameter {
        char* value;
};

struct _BoincConfiguration {
        BoincConfigurationParameter parameter[kBoincLastIndexEnum];
        BoincMutex mutex;
};


#define MAX_STRING_BUFF_SIZE 100
enum BoincConfigurationTypeEnum {
        BoincConfigurationString = 1,   //is a string
        BoincConfigurationNumber = 2,   //is a number
        BoincConfigurationCallback = 4, //value comes from a callback function
        BoincConfigurationStatic = 8, //user cannot modify the attribute, if a callback is set, the callback will be called only once and not every time
        BoincConfigurationDontStore = 16,
};

/**
 * If the callback, the function should return the result through the str parameter
 * the number of used char is returned
 */
typedef unsigned int (*BoincConfigurationCallbackFunc)(char * str, unsigned int max_str_size, BoincConfiguration * configuration);

typedef struct _BoincConfigurationParameterDefinition {
        int index;
        int type;
        char * name;
        BoincConfigurationCallbackFunc callback;
} BoincConfigurationParameterDefinition;

static BoincConfigurationParameterDefinition parameterDefinitions[]  = {
        { kBoincProjectURL, BoincConfigurationString, "kBoincProjectURL", NULL},
        { kBoincCGIURL, BoincConfigurationString | BoincConfigurationDontStore, "scheduler", NULL},
        { kBoincHostID, BoincConfigurationString, "hostid", NULL},
        { kBoincUserEmail, BoincConfigurationString, "kBoincUserEmail", NULL},
        { kBoincUserFullname, BoincConfigurationString, "kBoincUserFullname", NULL},
        { kBoincUserPassword, BoincConfigurationString, "kBoincUserPassword", NULL},
        { kBoincUserAuthenticator, BoincConfigurationString | BoincConfigurationDontStore, "authenticator", NULL},
        { kBoincUserTotalCredit, BoincConfigurationNumber, "kBoincUserTotalCredit", NULL},
        { kBoincUserTeamName, BoincConfigurationString, "kBoincUserTeamName", NULL},
        { kBoincHostTimezone, BoincConfigurationNumber, "kBoincHostTimezone", NULL},
        { kBoincHostDomainName, BoincConfigurationString, "kBoincHostDomainName", boincConfigurationGetHostDomainName},
        { kBoincHostIpAddress, BoincConfigurationString | BoincConfigurationStatic, "kBoincHostIpAddress", boincConfigurationGetHostIpAddress},
        { kBoincHostCPID, BoincConfigurationString, "kBoincHostCPID", NULL},
        { kBoincHostNCpu, BoincConfigurationNumber | BoincConfigurationStatic, "kBoincHostNCpu", boincConfigurationGetNCpu},
        { kBoincHostCpuVendor, BoincConfigurationString| BoincConfigurationStatic, "kBoincHostCpuVendor", boincConfigurationGetHostCpuVendor},
        { kBoincHostCpuModel, BoincConfigurationString| BoincConfigurationStatic, "kBoincHostCpuModel", boincConfigurationGetHostCpuModel},
        { kBoincHostCpuFeatures, BoincConfigurationString| BoincConfigurationStatic, "kBoincHostCpuFeatures", boincConfigurationGetHostCpuFeatures},
        { kBoincHostCpuFpops, BoincConfigurationNumber, "kBoincHostCpuFpops", boincConfigurationGetHostCpuFpops },
        { kBoincHostCpuIops, BoincConfigurationNumber, "kBoincHostCpuIops", boincConfigurationGetHostCpuIops},
        { kBoincHostCpuMemBW, BoincConfigurationNumber, "kBoincHostCpuMemBW", boincConfigurationGetHostCpuMemBW },
        { kBoincHostCpuCalculated, BoincConfigurationNumber, "kBoincHostCpuCalculated", NULL},
        { kBoincHostMemBytes, BoincConfigurationNumber| BoincConfigurationStatic, "kBoincHostMemBytes", boincConfigurationGetHostMemBytes},
        { kBoincHostMemCache, BoincConfigurationNumber| BoincConfigurationStatic, "kBoincHostMemCache", boincConfigurationGetHostMemCache},
        { kBoincHostMemSwap, BoincConfigurationNumber| BoincConfigurationStatic, "kBoincHostMemSwap", boincConfigurationGetHostMemSwap},
        { kBoincHostMemTotal, BoincConfigurationNumber| BoincConfigurationStatic, "kBoincHostMemTotal", boincConfigurationGetHostMemTotal},
        { kBoincHostDiskTotal, BoincConfigurationNumber| BoincConfigurationStatic, "kBoincHostDiskTotal", boincConfigurationGetHostDiskTotal},
        { kBoincHostDiskFree, BoincConfigurationNumber, "kBoincHostDiskFree", boincConfigurationGetHostDiskFree},
        { kBoincHostOSName, BoincConfigurationString | BoincConfigurationStatic, "kBoincHostOSName", boincConfigurationGetOSName},
        { kBoincHostOSVersion, BoincConfigurationString, "kBoincHostOSVersion", boincConfigurationGetHostOSVersion},
        { kBoincHostAccelerators, BoincConfigurationString, "kBoincHostAccelerators", NULL},
        { kBoincNetworkUpBandwidth, BoincConfigurationNumber, "kBoincNetworkUpBandwidth", boincConfigurationGetNetworkUpBandwidth},
        { kBoincNetworkUpAverage, BoincConfigurationNumber, "kBoincNetworkUpAverage", boincConfigurationGetNetworkUpAverage},
        { kBoincNetworkUpAvgTime, BoincConfigurationNumber, "kBoincNetworkUpAvgTime", boincConfigurationGetNetworkUpAvgTime},
        { kBoincNetworkDownBandwidth, BoincConfigurationNumber, "kBoincNetworkDownBandwidth", boincConfigurationGetNetworkDownBandwidth},
        { kBoincNetworkDownAverage, BoincConfigurationNumber, "kBoincNetworkDownAverage", boincConfigurationGetNetworkDownAverage},
        { kBoincNetworkDownAvgTime, BoincConfigurationNumber, "kBoincNetworkDownAvgTime", boincConfigurationGetNetworkDownAvgTime},
        { kBoincTotalDiskUsage, BoincConfigurationNumber, "kBoincTotalDiskUsage", boincConfigurationGetTotalDiskUsage},
        { kBoincProjectDiskUsage, BoincConfigurationNumber, "kBoincProjectDiskUsage", boincConfigurationGetProjectDiskUsage},
        { kBoincHttpProxy, BoincConfigurationString, "kBoincHttpProxy", NULL},
        { kBoincProjectDirectory, BoincConfigurationString | BoincConfigurationStatic | BoincConfigurationDontStore, "kBoincProjectDirectory", NULL},
        { kBoincProjectConfigurationFile, BoincConfigurationString | BoincConfigurationStatic | BoincConfigurationDontStore, "kBoincProjectConfigurationFile", NULL},
        { kBoincProjectConfigurationTmpFile, BoincConfigurationString | BoincConfigurationStatic | BoincConfigurationDontStore, "kBoincProjectConfigurationTmpFile", NULL},
        { kBoincPlatformName, BoincConfigurationString | BoincConfigurationStatic, "kBoincPlatformName",  boincConfigurationGetPlatformName},
        { kBoincWorkUnitDirectory, BoincConfigurationString, "kBoincWorkUnitDirectory", NULL},
        { kBoincUserTotalWorkunits, BoincConfigurationNumber, "kBoincUserTotalWorkunits", NULL},
        { kBoincLanguage, BoincConfigurationString, "kBoincLanguage", NULL},
};

static int boincConfigurationGetParameterFromName(const char * name) 
{
        for (int i = 0 ; i < kBoincLastIndexEnum ; i++) {
                if (!strcmp(parameterDefinitions[i].name, name))
                        return i;
        }
        return -1;
}

int boincConfigurationIsString(int parameter) 
{
        return (parameter < kBoincLastIndexEnum) 
                && (parameterDefinitions[parameter].type & BoincConfigurationString);
}

int boincConfigurationIsNumber(int parameter) 
{
        return (parameter < kBoincLastIndexEnum) 
                && (parameterDefinitions[parameter].type & BoincConfigurationNumber);
}

static int boincConfigurationIsCallback(int parameter) 
{
        return (parameter < kBoincLastIndexEnum) && (parameterDefinitions[parameter].callback);
}

static int boincConfigurationIsStatic(int parameter) 
{
        return (parameter < kBoincLastIndexEnum) 
                && (!strlen(parameterDefinitions[parameter].name) 
                    || (parameterDefinitions[parameter].type & BoincConfigurationStatic));
}

static int boincConfigurationIsStorable(int parameter) 
{
        return (parameter < kBoincLastIndexEnum) 
                && strlen(parameterDefinitions[parameter].name) 
                && !(parameterDefinitions[parameter].type & BoincConfigurationDontStore);
}

int boincConfigurationIsReadOnly(int parameter) 
{
        return boincConfigurationIsStatic(parameter)
                || boincConfigurationIsCallback(parameter);
}


static BoincError boincConfigurationSetInternalString(BoincConfiguration* configuration, int parameter, const char* value) 
{
        int strsize = strlen(value) + 1;
        BoincError err = NULL;

        boincDebugf("boincConfiugrationSetInternalString %s = %s", 
                    parameterDefinitions[parameter].name , value);

        boincMutexLock(configuration->mutex);

        if (configuration->parameter[parameter].value == NULL) {
                configuration->parameter[parameter].value = boincArray(char, strsize);

        } else if (strsize > strlen(configuration->parameter[parameter].value) + 1) {
                boincFree(configuration->parameter[parameter].value);
                configuration->parameter[parameter].value = boincArray(char, strsize);
        }

        if (configuration->parameter[parameter].value == NULL)
                err = boincErrorCreate(kBoincFatal, 1, "Out of memory");
        else 
                memcpy(configuration->parameter[parameter].value, value, strsize);

        boincMutexUnlock(configuration->mutex);

        return err;
}


BoincConfiguration* boincConfigurationCreate(const char* dir) 
{
        BoincError err;

        // Mem creation
        BoincConfiguration * conf = boincNew(BoincConfiguration);
        if (conf == NULL)
                return NULL;

        conf->mutex = boincMutexCreate("conf");
        if (conf->mutex == NULL) {
                boincFree(conf);
                return NULL;
        }

        // Copy dir name
        if (boincErrorOccured(boincConfigurationSetInternalString(conf, kBoincProjectDirectory, dir)) ) {
                boincConfigurationDestroy(conf);
                return NULL;  //(-1, "Cannot affect new values to the configurator");
        }

        int len = strlen(dir) + strlen( "config.xml" ) + 6;
        char * filename = boincArray(char, len); // boincArray zeros the memory
        if (filename == NULL) {
                boincLog(kBoincError, -1, "Out of memory");
                boincConfigurationDestroy(conf);
                return NULL;
        }
        initFilename(filename, len, dir);
        appendFilename(filename, len, "config.xml");

        if ((err = boincConfigurationSetInternalString(conf, kBoincProjectConfigurationFile, filename)) != NULL) {
                boincLogError(err);
                boincFree(filename);
                boincConfigurationDestroy(conf);
                return NULL;
        }

        strcat(filename, ".tmp");
        if ((err = boincConfigurationSetInternalString(conf, kBoincProjectConfigurationTmpFile, filename)) != NULL) {
                boincLogError(err);
                boincErrorDestroy(err);
                boincFree(filename);
                boincConfigurationDestroy(conf);
                return NULL; 
        }

        boincFree(filename);

        return conf;
}

BoincError boincConfigurationDestroy(BoincConfiguration* configuration) 
{
        if (!configuration)
                return NULL;

        boincMutexLock(configuration->mutex);
        boincMutexUnlock(configuration->mutex);
        //printf("Mutex destroyed\n");
        boincMutexDestroy(configuration->mutex);

        // Free each allocated string
        for (int i = 0 ; i < kBoincLastIndexEnum ; i++) {
                if (configuration->parameter[i].value) {
                        boincFree(configuration->parameter[i].value);
                        configuration->parameter[i].value = NULL;
                }
        }

        // Free BoincConfiguration
        boincFree(configuration);

        return 0;
}


int boincConfigurationXMLBegin(const char* elementName, const char** attr, void* userData) 
{
        return boincConfigurationGetParameterFromName(elementName);
}

BoincError boincConfigurationXMLEnd(int index, const char* elementValue, void* userData) 
{
        BoincConfiguration * conf = (BoincConfiguration*) userData;
        return boincConfigurationSetInternalString(conf, index, elementValue);
}


BoincError boincConfigurationUpdateFromFile(BoincConfiguration * configuration, const char * xmlFileName) 
{  
        boincDebugf("boincConfigurationUpdateFromFile Updating conf from %s", xmlFileName);
        BoincError err = boincXMLParse(xmlFileName, boincConfigurationXMLBegin, boincConfigurationXMLEnd, configuration);
        if (err) {
                return err;
        }
        return boincConfigurationStore(configuration);
}

BoincError boincConfigurationCreateDefault(BoincConfiguration * configuration) 
{
        boincErrorDestroy(boincConfigurationSetInternalString(configuration, kBoincHostID, "0"));
        return boincConfigurationOSCreateDefault(configuration);
}

BoincError boincConfigurationLoad(BoincConfiguration* configuration) 
{
        char conffile[MAX_STRING_BUFF_SIZE];
        boincConfigurationGetString(configuration, kBoincProjectConfigurationFile, conffile, MAX_STRING_BUFF_SIZE);
        BoincError err = boincConfigurationUpdateFromFile(configuration, conffile);
        if (err) {
                if (boincErrorIsMajor(err))
                        return err;
                boincErrorDestroy(err);
                return boincConfigurationCreateDefault(configuration);
        }
        return NULL;
}

BoincError boincConfigurationStore(BoincConfiguration* configuration) 
{
        int i;
        char confbuff[MAX_STRING_BUFF_SIZE];
        char confarg[MAX_STRING_BUFF_SIZE];
        boincConfigurationGetString(configuration, kBoincProjectConfigurationTmpFile, confbuff, MAX_STRING_BUFF_SIZE -1);

        FILE * fh = boincFopen(confbuff, "w+");
        if (fh == NULL)
                return boincErrorCreatef(kBoincError, -1, "Cannot open %s in write mode", confbuff);

        fprintf(fh, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        fprintf(fh, "<BoincLiteConfiguration>\n");
        for (i = 0 ; i < kBoincLastIndexEnum ; i++) {
                if (!boincConfigurationIsStorable(i))
                        continue;
                if (!boincConfigurationHasParameter(configuration, i))
                        continue;
                fprintf(fh, "<%s>%s</%s>\n", parameterDefinitions[i].name, 
                        boincConfigurationGetString(configuration, i, confarg, MAX_STRING_BUFF_SIZE ), 
                        parameterDefinitions[i].name);
        }
        fprintf(fh, "</BoincLiteConfiguration>\n");
        boincFclose(fh);

        char newconf[MAX_STRING_BUFF_SIZE];
        boincConfigurationGetString(configuration, kBoincProjectConfigurationFile, newconf, MAX_STRING_BUFF_SIZE - 1);

        return boincRenameFile(confbuff, newconf);
}

int boincConfigurationHasParameter(BoincConfiguration* configuration, int parameter) 
{ 
        boincMutexLock(configuration->mutex);
        int r = (parameter < kBoincLastIndexEnum) && (configuration->parameter[parameter].value != NULL);
        boincMutexUnlock(configuration->mutex);
        return r;
}

BoincError boincConfigurationSetString(BoincConfiguration* configuration, int parameter, const char* value) 
{
        if (boincConfigurationIsReadOnly(parameter))
                return boincErrorCreatef(kBoincError, -1, "boincConfigurationSetString(%s) with parameter %s cannot be modified",  
                                         parameterDefinitions[parameter].name, value);

        if (! (parameterDefinitions[parameter].type & BoincConfigurationString))
                return boincErrorCreatef(kBoincError, -2, "boincConfigurationSetString(%s) with parameter %s is not a string",  
                                         parameterDefinitions[parameter].name, value);

        return boincConfigurationSetInternalString(configuration, parameter, value);
}

BoincError boincConfigurationSetNumber(BoincConfiguration* configuration, int parameter, double value) 
{
        if (!boincConfigurationIsNumber(parameter)) 
                return boincErrorCreatef(kBoincError, -1, "boincConfigurationSetNumber(%s) with parameter %f is not a number",  
                                         parameterDefinitions[parameter].name, value);

        if (boincConfigurationIsReadOnly(parameter))
                return boincErrorCreatef(kBoincError, -2, "boincConfigurationSetNumber(%s) with parameter %f cannot be modified", 
                                         parameterDefinitions[parameter].name, value);

        char nb[MAX_STRING_BUFF_SIZE];
        snprintf(nb, MAX_STRING_BUFF_SIZE, "%49.49f", value);
        return boincConfigurationSetInternalString(configuration, parameter, nb);
}

char* boincConfigurationGetString(BoincConfiguration* configuration, int parameter, char * str, int len) 
{
        str[0] = 0;
        if (boincConfigurationIsCallback(parameter)) {
                (*parameterDefinitions[parameter].callback)(str, len, configuration);
                str[len-1] = 0; // make sure the string is null terminated

        } else {
                if (parameter < kBoincLastIndexEnum && configuration->parameter[parameter].value) {
                        boincMutexLock(configuration->mutex);
                        strncpy(str, configuration->parameter[parameter].value, len);
                        boincMutexUnlock(configuration->mutex);

                        str[len-1] = 0;
                }
        }
        return str;
}

double boincConfigurationGetNumber(BoincConfiguration* configuration, int parameter) 
{
        if (! boincConfigurationIsNumber(parameter) && ! boincConfigurationIsCallback(parameter))
                return 0;

        char nb[MAX_STRING_BUFF_SIZE];
        boincConfigurationGetString(configuration, parameter, nb, MAX_STRING_BUFF_SIZE);
        return atof(nb);
}
										   
