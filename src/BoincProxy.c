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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "config.h"
#include "BoincProxy.h"
#include "BoincHttp.h"
#include "BoincXMLParser.h"
#include "BoincError.h"
#include "BoincMem.h"
#include "BoincHash.h"
#include "BoincUtil.h"
#include "BoincWorkUnit.h"

struct _BoincProxy {
        BoincConfiguration * conf;
        BoincStatusChangedCallback statusCallback;
        void * statusData;	
};

enum BoincProxyUrlPath {
        kBoincProxyProjectConfig = 0,
        kBoincProxyLookupAccount,
};

static char* projectConfigPath = "get_project_config.php";
static char* accountPath = "lookup_account.php";

#define BUFF_SIZE 255


typedef struct _BoincProxyUrl {
        char * urlbase;
        char * urlpath;
        int nbargs;
        const char ** args;
} BoincProxyUrl;

typedef struct _BoincProxyPostData {
        BoincProxyWorkUnit* pwu;
        int nbFiles;
        char ** files;
        int fileOffset;
        FILE * fh;
        unsigned int bytesRead;
        unsigned int contentLength;
} BoincProxyPostData;


static char* boincProxyGetAbsolutePathFromProxy(BoincProxy * proxy, const char * relativeFilename, char * resFile, int resSize) 
{
        boincConfigurationGetString(proxy->conf, kBoincProjectDirectory, resFile, resSize);
        return appendFilename(resFile, resSize, relativeFilename);
}


static char * boincProxyConstructUrl(BoincProxyUrl * s_url, char * resUrl, int resSize) 
{
        boincDebugf("boincConstructUrl %s%s", s_url->urlbase, s_url->urlpath);
        if (!s_url->urlbase || !s_url->urlpath)
                return NULL;
        if (initFilename(resUrl, resSize, s_url->urlbase) == NULL) {
                return NULL;
        }
        if (appendFilename(resUrl, resSize, s_url->urlpath) == NULL) {
                return NULL;
        }

        int size = strlen(resUrl);
        for (int i = 0; i < s_url->nbargs ; i ++) {
                size += strlen(s_url->args[i]) +1;
                if (size > resSize)
                        return NULL; 
                if (i % 2) {
                        strcat(resUrl, "=");
                }else{
                        if (i)
                                strcat(resUrl, "&");
                        else
                                strcat(resUrl, "?");
                }
                strcat(resUrl, s_url->args[i]);
        }
        boincDebugf("construct url final: %s", resUrl);
        return resUrl;
}

static void boincProxySetPostContentLength(BoincProxyPostData * data) 
{
        data->contentLength = 0;
        struct stat bStat;
        for (int i = 0 ; i < data->nbFiles ; i++) {
                stat(data->files[i], &bStat);
                data->contentLength +=  bStat.st_size;
        }
}

static size_t boincProxyPostCallback(void *uploadData, size_t size, size_t nmemb, void* userdata) 
{
        BoincProxyPostData * data = (BoincProxyPostData *) userdata;
        boincDebugf("boincProxPostCallback (%d/%d)", data->fileOffset, data->nbFiles);
        if (data->fh == NULL) {
                if (data->fileOffset >= data->nbFiles)
                        return 0;
                boincDebugf("boincProxPostCallback opens %s", data->files[data->fileOffset]);
                data->fh = boincFopen(data->files[data->fileOffset++], "r");
                if (data->fh == NULL)
                        return 0;
        }
        size_t read = fread(uploadData, size, nmemb, data->fh);
        if (!read || feof(data->fh)) {
                boincDebug("boincProxPostCallback closes the opened file");
                boincFclose(data->fh);
                data->fh = NULL;
        }
        boincDebugf("boincProxPostCallback reads %d bytes", read);

        data->bytesRead += read;

        if (data->pwu != NULL) {
                float progress = 100.0f * (float) data->bytesRead / data->contentLength;
                boincProxyChangeWorkUnitProgress(data->pwu, progress);
        }

        return read;
}

static BoincError boincProxyPost(BoincProxy * proxy, BoincProxyUrl * s_url, BoincProxyPostData * data, const char * path)
{
        char theurl[BUFF_SIZE];
        if (boincProxyConstructUrl(s_url, theurl, BUFF_SIZE) == NULL) {
                return boincErrorCreate(kBoincError, kBoincInternal, "URL too long");
        }
        boincProxySetPostContentLength(data); 
        BoincError err = boincHttpPost(theurl, path, data->contentLength, boincProxyPostCallback, data);
        if (data->fh != NULL) {
                boincDebug("boincProxPostCallback closes the opened file");
                boincFclose(data->fh);
                data->fh = NULL;
        }
        return err;
}

static BoincError boincProxyGet(BoincProxy * proxy , BoincProxyUrl * s_url , const char * path) 
{
        boincDebugf("boincProxyGet urlpath %s", s_url->urlpath);

        char theurl[BUFF_SIZE];
        if (boincProxyConstructUrl(s_url, theurl, BUFF_SIZE) == NULL) {
                return boincErrorCreate(kBoincError, kBoincInternal, "URL too long");    
        }
        return boincHttpGet(theurl, path);
}


BoincProxy* boincProxyCreate(BoincConfiguration* configuration) 
{
        boincDebugf("boincProxyCreate");

        BoincProxy* proxy = boincNew(BoincProxy);
        if (!proxy) {
                boincLog(kBoincError, -1, "Out of memory");
                return NULL;
        }
        proxy->conf = configuration;
        return proxy;
}

BoincError boincProxyInit(BoincProxy * proxy) 
{
        BoincError err = boincHttpInit();
        if (err) {
                return err;
        }

        char projectURL[BUFF_SIZE];
        boincConfigurationGetString(proxy->conf, kBoincProjectURL, projectURL, BUFF_SIZE);

        const char * args[] = {};
        BoincProxyUrl s_url = { projectURL, projectConfigPath, sizeof(args) / sizeof(*args), args};

        boincDebugf("boincProxyCreate path: %s/%s", s_url.urlbase, s_url.urlpath);

        char projectFile[BUFF_SIZE];
        boincProxyGetAbsolutePathFromProxy(proxy, "project.xml", projectFile, BUFF_SIZE);

        err = boincProxyGet(proxy, &s_url, projectFile);
        if (err) {
                return err;
        }

        err = boincConfigurationUpdateFromFile(proxy->conf, projectFile);
        if (err) {
                return err;
        }
        return NULL;
}

void boincProxyDestroy(BoincProxy* proxy) 
{  
        boincHttpCleanup();
        boincFree(proxy);
        proxy = NULL;
}

static int boincProxyAuthXMLBegin(const char* elementName, const char** attr, void* userData)
{
        boincDebugf("boincProxyAuthXMLBegin: %s", elementName);
        if (!strcmp(elementName, "error_msg"))
                return 1;
        if (!strcmp(elementName, "authenticator"))
                return 2;
        return -1;
}

static BoincError boincProxyAuthXMLEnd(int index, const char* elementValue, void* userData)
{
        boincDebugf("boincProxyAuthXMLEnd: index: %d", index);
        if (index == 1)
                return boincErrorCreate(kBoincError, kBoincAuthentication, elementValue); 
        if (index == 2) {
                char** handle = (char**) userData;
                *handle = strdup(elementValue);
                if (*handle == NULL) {
                        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                }
        }
        return NULL;
}

BoincError boincProxyAuthenticate(BoincProxy* proxy) 
{
        char mailBuff[BUFF_SIZE];
        char passBuff[BUFF_SIZE];

        // Check for a valid email/password 
        if (!boincConfigurationHasParameter(proxy->conf, kBoincUserEmail)) {
                return boincErrorCreate(kBoincError, kBoincAuthentication, "Can't authenticate: email not set");
        }
        boincConfigurationGetString(proxy->conf, kBoincUserEmail, mailBuff, BUFF_SIZE);
        if (strlen(mailBuff) == 0) {
                return boincErrorCreate(kBoincError, kBoincAuthentication, "Can't authenticate: empty email");
        }

        if (!boincConfigurationHasParameter(proxy->conf, kBoincUserPassword)) {
                return boincErrorCreate(kBoincError, kBoincAuthentication, "Can't authenticate: password not set");
        }
        boincConfigurationGetString(proxy->conf, kBoincUserPassword, passBuff, BUFF_SIZE);
        if (strlen(passBuff) == 0) { // The password hash should be 32 characters long
                return boincErrorCreate(kBoincError, kBoincAuthentication, "Can't authenticate: empty password hash");
        }

        const char * args[] = {"email_addr", mailBuff, "passwd_hash", passBuff };
        char projectURL[BUFF_SIZE];
        boincConfigurationGetString(proxy->conf, kBoincProjectURL, projectURL, BUFF_SIZE);

        BoincProxyUrl url = { projectURL, accountPath, 4, args };

        char xmlFile[BUFF_SIZE];
        boincProxyGetAbsolutePathFromProxy(proxy, "auth.xml", xmlFile, BUFF_SIZE);

        BoincError err = boincProxyGet(proxy, &url, xmlFile);
        if (err) {
                return err;
        }

        // Check for the authenticator in response
        char* authenticator = NULL;
        
        err = boincXMLParse(xmlFile, boincProxyAuthXMLBegin, boincProxyAuthXMLEnd, &authenticator);
        if (err) {
                return err;
        }
        if (authenticator == NULL) {
                return boincErrorCreate(kBoincError, kBoincAuthentication, "Authentication failed");
        }
        err = boincConfigurationSetString(proxy->conf, kBoincUserAuthenticator, authenticator);
        if (err) {
                return err;
        }
        free(authenticator);

        // Store the configuration
        return boincConfigurationStore(proxy->conf);
}

BoincError boincProxyChangeAuthentication(BoincProxy* proxy,
                                          const char* email, 
                                          const char* password)
{
        char passwordHash[33];
        boincPasswordHash(email, password, passwordHash);

        const char* args[] = {"email_addr", email, "passwd_hash", passwordHash };
        char projectURL[BUFF_SIZE];
        boincConfigurationGetString(proxy->conf, kBoincProjectURL, projectURL, BUFF_SIZE);

        BoincProxyUrl url = { projectURL, accountPath, 4, args };

        char xmlFile[BUFF_SIZE];
        boincProxyGetAbsolutePathFromProxy(proxy, "auth.xml", xmlFile, BUFF_SIZE);

        BoincError err = boincProxyGet(proxy, &url, xmlFile);
        if (err) {
                return err;
        }

        // Check for the authenticator in response
        char* authenticator = NULL;
        
        err = boincXMLParse(xmlFile, boincProxyAuthXMLBegin, boincProxyAuthXMLEnd, &authenticator);
        if (err) {
                return err;
        }
        if (authenticator == NULL) {
                return boincErrorCreate(kBoincError, kBoincAuthentication, "Authentication failed");
        }
        err = boincConfigurationSetString(proxy->conf, kBoincUserAuthenticator, authenticator);
        free(authenticator);
        if (err) return err;

        err = boincConfigurationSetString(proxy->conf, kBoincUserEmail, email);
        if (err) return err;

        err = boincConfigurationSetString(proxy->conf, kBoincUserPassword, passwordHash);
        if (err) return err;

        // Store the configuration
        return boincConfigurationStore(proxy->conf);
}

enum {
        kBoincWUError = 1,
        kBoincWUMessage,
        kBoincWUEFlops,
        kBoincWUEMemory,
        kBoincWUEDisk,
        kBoincWUName,
        kBoincWUFileinfoName,
        kBoincWUFileinfoUrl,
        kBoincWUFileinfoFilsignature,
        kBoincWUFileinfoNBytes,
        kBoincWUFileinfoChecksum,
        kBoincWUFileref,
        kBoincWUOpenName,
        kBoincWUDeadline,
        kBoincWUAppVersion,
        kBoincWUApiVersion,
        kBoincWUFileinfoXMLsignature,
        kBoincWUFileinfoMaxNBytes,
        kBoincWUDelay,
        kBoincWUResultName,
};

static void boincProxyAffectCurrentFileInfoFromName(BoincWorkUnit * wu, const char * name) 
{
        BoincFileInfo* fi = NULL;
        boincDebugf("boincProxyGetFileInfoFromName: %s", name);
        for (fi = wu->globalFileInfo; fi ; fi = fi->gnext) {
                boincDebugf("strcmp '%s' '%s'", name, fi->name);
                if (!strcmp(name, fi->name)) {
                        if ( *(wu->currentFileInfo) ) 
                                fi->next = *(wu->currentFileInfo);
                        *(wu->currentFileInfo) = fi;
                        boincDebugf("fileinfo affected to %d", fi);
                        return ;
                }
        }
        return ;
}

static int boincProxyWUXMLBegin(const char* elementName, const char** attr, void* userData) 
{
        BoincWorkUnit * wu = (BoincWorkUnit*) userData;
        if (!strcmp(elementName, "request_delay")) {
                return kBoincWUDelay;
        }
        if (!strcmp(elementName, "message")){
                boincDebug("message found");
                if (attr[0]) {// && !boincXMLAttributeCompare(attr, "priority", "high")) {
                        return kBoincWUMessage;
                }
                return -1;
        }

        if (!strcmp(elementName, "workunit>name")) {
                return kBoincWUName;
        }
        if (!strcmp(elementName, "workunit>rsc_fpops_est")) {
                return kBoincWUEFlops;
        }
        if (!strcmp(elementName, "workunit>rsc_disk_bound")) {
                return kBoincWUEDisk;
        }
        if (!strcmp(elementName, "workunit>rsc_memory_bound")) {
                return kBoincWUEMemory;
        }
        if (!strcmp(elementName, "file_info")) {
                BoincFileInfo* fileInfo = boincNew(BoincFileInfo);
                if (fileInfo == NULL) {
                        boincLog(kBoincError, -1, "Out of memory");
                        return kBoincWUError;
                }
                fileInfo->gnext = wu->globalFileInfo;
                wu->globalFileInfo = fileInfo;
                boincDebugf("new file info created: %d", fileInfo);
                return -1;
        }
        if (!strcmp(elementName, "file_info>name")) {
                return kBoincWUFileinfoName;
        }
        if (!strcmp(elementName, "file_info>url")) {
                return kBoincWUFileinfoUrl ;
        }
        if (!strcmp(elementName, "file_info>executable")) {
                boincDebug("CurrentFileInfo: Executable flag set");
                wu->globalFileInfo->flags |= kBoincFileExecutable;
                return -1;
        }
        if (!strcmp(elementName, "file_info>generated_locally")) {
                boincDebug("CurrentFileInfo: Generated flag set");
                wu->globalFileInfo->flags |= kBoincFileGenerated;
                return -1;
        }
        if (!strcmp(elementName, "file_info>upload_when_present")) {
                boincDebug("CurrentFileInfo: Upload flag set");
                wu->globalFileInfo->flags |= kBoincFileUpload;
                return -1;
        }
        if (!strcmp(elementName, "file_info>file_signature")) {
                return kBoincWUFileinfoFilsignature ;
        }
        if (!strcmp(elementName, "file_info>xml_signature")) {
                return kBoincWUFileinfoXMLsignature ;
        }
        if (!strcmp(elementName, "file_info>nbytes")) {
                return kBoincWUFileinfoNBytes ;
        }
        if (!strcmp(elementName, "file_info>max_nbytes")) {
                return kBoincWUFileinfoMaxNBytes;
        }
        if (!strcmp(elementName, "file_info>md5_cksum")) {
                return kBoincWUFileinfoChecksum ;
        }
        if (!strcmp(elementName, "app_version")) {
                wu->app = boincNew(BoincApp);
                if (wu->app == NULL) {
                        boincLog(kBoincError, -1, "Out of memory");
                        return kBoincWUError;
                }
                return -1;
        }
        if (!strcmp(elementName, "result")) {
                wu->result = boincNew(BoincResult);
                if (wu->result == NULL) {
                        boincLog(kBoincError, -1, "Out of memory");
                        return kBoincWUError;
                }
                wu->result->cpu_time = 0;
                return -1;
        }
        if (!strcmp(elementName, "app_version>file_ref>file_name")) {
                wu->currentFileInfo = &(wu->app->fileInfo);
                return kBoincWUFileref;
        }
        if (!strcmp(elementName, "workunit>file_ref>file_name")) {
                wu->currentFileInfo = &(wu->fileInfo);
                return kBoincWUFileref;
        }
        if (!strcmp(elementName, "result>file_ref>file_name")) {
                wu->currentFileInfo = &(wu->result->fileInfo);
                return kBoincWUFileref;
        }
        if (!strcmp(elementName, "result>name")) {
                return kBoincWUResultName;
        }
        if (!strcmp(elementName, "app_version>file_ref>open_name")) {
                return kBoincWUOpenName;
        }
        if (!strcmp(elementName, "workunit>file_ref>open_name")) {
                return kBoincWUOpenName;
        }
        if (!strcmp(elementName, "result>file_ref>open_name")) {
                boincDebugf("result open_name");
                return kBoincWUOpenName;
        }
        if (!strcmp(elementName, "result>report_deadline")) {
                return kBoincWUDeadline;
        }
        if (!strcmp(elementName, "app_version>file_ref>main_program")) {
                boincDebugf("Flag info: %s to %d", elementName, *(wu->currentFileInfo));
                BoincFileInfo * fi = *(wu->currentFileInfo);
                boincDebug("CurrentFileInfo: MainProgram flag set");
                fi->flags |= kBoincFileMainProgram;
                return -1;
        }
        if (!strcmp(elementName, "app_version>version_num")) {
                return kBoincWUAppVersion;
        }
        if (!strcmp(elementName, "app_version>api_version")) {
                return kBoincWUApiVersion;
        }
        return -1;
}

static BoincError boincProxyWUXMLEnd(int index, const char* elementValue, void* userData)
{
        BoincWorkUnit * wu = (BoincWorkUnit*) userData;
        switch (index) {
        case kBoincWUDelay:
                wu->delay = atoi(elementValue);
                return NULL;
        case kBoincWUError:
                return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory"); // see error cases in boincProxyWUXMLBegin
        case kBoincWUMessage:
                boincLogf(kBoincError, 0, elementValue);
                break;
        case kBoincWUName:
                if (! wu->name) {
                        boincDebugf("Name: %s", elementValue);
                        wu->name = boincArray(char, strlen(elementValue)+1);
                        if (wu->name == NULL) {
                                return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                        }
                        strcpy(wu->name, elementValue);
                }
                break;
        case kBoincWUResultName:
                if (! wu->result->name) {
                        wu->result->name = boincArray(char, strlen(elementValue)+1);
                        if (wu->name == NULL) {
                                return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                        }
                        strcpy(wu->result->name, elementValue);
                }
                break;
        case kBoincWUEFlops:
                boincDebugf("Flops: %s", elementValue);
                wu->estimatedFlops = atol(elementValue);
                break ;
        case kBoincWUEDisk:
                boincDebugf("Disk: %s", elementValue);
                wu->estimatedDisk = atol(elementValue);
                break ;
        case kBoincWUEMemory:
                boincDebugf("Memory: %s", elementValue);
                wu->estimatedDisk = atoi(elementValue);
                break ;
        case kBoincWUFileinfoName:
                boincDebugf("FileinfoName: %s", elementValue);
                wu->globalFileInfo->name = boincArray(char, strlen(elementValue)+1);
                if (wu->globalFileInfo->name == NULL) {
                        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                }
                strcpy(wu->globalFileInfo->name, elementValue);
                break ;
        case kBoincWUFileinfoUrl:
                boincDebugf("Url for %s: %s", wu->globalFileInfo->name, elementValue);
                wu->globalFileInfo->url = boincArray(char, strlen(elementValue)+1);
                if (wu->globalFileInfo->url == NULL) {
                        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                }
                strcpy(wu->globalFileInfo->url, elementValue);
                break ;
        case kBoincWUFileinfoFilsignature:
                boincDebugf("Signature for %s", wu->globalFileInfo->name);
                wu->globalFileInfo->signature = boincArray(char, strlen(elementValue)+1);
                if (wu->globalFileInfo->signature == NULL) {
                        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                }
                strcpy(wu->globalFileInfo->signature, elementValue);
                break ;
        case kBoincWUFileinfoXMLsignature:
                boincDebugf("XML Signature for %s (%s)", wu->globalFileInfo->name, elementValue);
                wu->globalFileInfo->xmlSignature = boincArray(char, strlen(elementValue)+1);
                if (wu->globalFileInfo->xmlSignature == NULL) {
                        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                }
                strcpy(wu->globalFileInfo->xmlSignature, elementValue);
                break ;
        case kBoincWUFileinfoNBytes:
                boincDebugf("nBytes for %s: %s", wu->globalFileInfo->name, elementValue);
                wu->globalFileInfo->nBytes = atoi(elementValue);
        case kBoincWUFileinfoMaxNBytes:
                boincDebugf("maxBytes for %s: %s", wu->globalFileInfo->name, elementValue);
                wu->globalFileInfo->maxBytes = atoi(elementValue);
                break; 
        case kBoincWUFileinfoChecksum:
                boincDebugf("checksum for %s", wu->globalFileInfo->name);
                wu->globalFileInfo->checksum = boincArray(char, strlen(elementValue)+1);
                if (wu->globalFileInfo->checksum == NULL) {
                        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                }
                strcpy(wu->globalFileInfo->checksum, elementValue);
                break ;
        case kBoincWUFileref:
                boincDebugf("FileRef for %s", elementValue);
                boincProxyAffectCurrentFileInfoFromName(wu, elementValue);
                break;
        case kBoincWUDeadline:
                boincDebugf("ResultDeadline: %s", elementValue);
                wu->result->deadline = atoi(elementValue);
                break;
        case kBoincWUAppVersion:
                boincDebugf("AppVersion: %s", elementValue);
                wu->app->version = boincArray(char, strlen(elementValue)+1);    
                if (wu->app->version == NULL) {
                        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                }
                strcpy(wu->app->version, elementValue);
                break;
        case kBoincWUApiVersion:
                boincDebugf("APIVersion: %s", elementValue);
                wu->app->apiVersion = boincArray(char, strlen(elementValue)+1);    
                if (wu->app->apiVersion == NULL) {
                        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                }
                strcpy(wu->app->apiVersion, elementValue);
                break;
        case kBoincWUOpenName:
                boincDebugf("OpenName: %s to %d", elementValue, *(wu->currentFileInfo));
                BoincFileInfo * fi = *(wu->currentFileInfo);
                if (!fi->openname) {
                        fi->openname = boincArray(char, strlen(elementValue)+1);
                        if (fi->openname == NULL) {
                                return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                        }
                        strcpy(fi->openname, elementValue);
                }
                break;
        default:
                break; // do nothing
        }
        return NULL;
}


static char * boincGetRequestWorkFile(BoincWorkUnit * wu, char* xmlFileRes)
{
        return boincGetAbsolutePath(wu->workingDir, "workResponse.xml", xmlFileRes, BUFF_SIZE);
}

BoincError boincProxyLoadWorkUnit(BoincWorkUnit* wu)
{
        char xmlFileres[BUFF_SIZE];

        BoincError err = boincXMLParse(boincGetRequestWorkFile(wu, xmlFileres), boincProxyWUXMLBegin, boincProxyWUXMLEnd, (void*) wu);
        if (!err && !wu->globalFileInfo && wu->delay) {
                return boincErrorCreateWithDelay(kBoincServer, "Empty work unit", wu->delay);
        }
        wu->delay = 0;
        return err;
}


BoincError boinProxyWorkUnitExecuted(BoincProxyWorkUnit * pwu, unsigned int error)
{
        char* createFile = "resultRequest.xml";
        char filepath[BUFF_SIZE];

        initFilename(filepath, BUFF_SIZE, pwu->wu->workingDir);
        if (appendFilename(filepath, BUFF_SIZE, createFile) == NULL) {
                return boincErrorCreate(kBoincError, kBoincFileSystem, "File path too long");
        }

        FILE * file = boincFopen(filepath, "w+");
        if (file == NULL) {
                return boincErrorCreatef(kBoincError, kBoincFileSystem, "Cannot open file %s", filepath);
        }

        BoincProxy * proxy = pwu->proxy;

        char strBuff[BUFF_SIZE];
        fprintf(file, "<scheduler_request>\n");
        fprintf(file, "<authenticator>%s</authenticator>\n", 
                boincConfigurationGetString(proxy->conf, kBoincUserAuthenticator, strBuff, BUFF_SIZE));
        fprintf(file, "<hostid>%s</hostid>\n", boincConfigurationGetString(proxy->conf, kBoincHostID, strBuff, BUFF_SIZE));
        fprintf(file, "<rpc_seqno>0</rpc_seqno>\n");
        fprintf(file, "<core_client_major_version>6</core_client_major_version>\n"); //1
        fprintf(file, "<core_client_minor_version>2</core_client_minor_version>\n"); //1
        fprintf(file, "<core_client_release>19</core_client_release>\n");            //1
        fprintf(file, "<work_req_seconds>1.000000</work_req_seconds>\n");
        fprintf(file, "<resource_share_fraction>1.000000</resource_share_fraction>\n");
        fprintf(file, "<rrs_fraction>1.000000</rrs_fraction>\n");
        fprintf(file, "<prrs_fraction>1.000000</prrs_fraction>\n");
        fprintf(file, "<estimated_delay>0.000000</estimated_delay>\n");
        fprintf(file, "<duration_correction_factor>1.000000</duration_correction_factor>\n");
        fprintf(file, "<client_cap_plan_class>1</client_cap_plan_class>\n");
        fprintf(file, "<platform_name>%s</platform_name>\n", boincConfigurationGetString(proxy->conf, kBoincPlatformName, strBuff, BUFF_SIZE));
        printf("%s %s\n", pwu->wu->name, pwu->wu->result->name);
        fprintf(file, "<result>\n");
        fprintf(file, "    <name>%s</name>\n", pwu->wu->result->name);
        fprintf(file, "    <final_cpu_time>%u.000000</final_cpu_time>\n", pwu->wu->result->cpu_time);
        fprintf(file, "    <exit_status>%u</exit_status>\n", error);
        fprintf(file, "    <state>%u</state>\n", (error) ? 3 : 5);
        fprintf(file, "    <platform>%s</platform>\n",boincConfigurationGetString(proxy->conf, kBoincPlatformName, strBuff, BUFF_SIZE));
        fprintf(file, "    <version_num>%s</version_num>\n", pwu->wu->app->version);
        fprintf(file, "    <app_version_num>%s</app_version_num>\n", pwu->wu->app->version);
        fprintf(file, "    <stderr_out>\n");
        fprintf(file, "    </stderr_out>\n");
        fprintf(file, "</result>\n");  
        fprintf(file, "<other_results>\n");
        fprintf(file, "</other_results>\n");
        fprintf(file, "<in_progress_results>\n");
        fprintf(file, "</in_progress_results>\n");
        fprintf(file, "</scheduler_request>\n");
        boincFclose(file);

        boincDebugf("boinProxyWorkUnitExecuted: data writen in %s", filepath);

        BoincProxyUrl url = {boincConfigurationGetString(proxy->conf, kBoincCGIURL, strBuff, BUFF_SIZE),
                             "", 0, NULL};
        char * files[] = {filepath};
        BoincProxyPostData post = { NULL, 1,  files, 0, NULL, 0, 0 };

        char xmlFileres[BUFF_SIZE];
        BoincError err = boincProxyPost(proxy, &url, &post, boincGetAbsolutePath(pwu->wu->workingDir, "resultResponse.xml", xmlFileres, BUFF_SIZE));

        if (err) {
                boincLogError(err);
                return err;
        }
  
        err = boincConfigurationUpdateFromFile(proxy->conf, xmlFileres);
        if (err) {
                boincErrorDestroy(err); // if the update failed, don't stop
                err = NULL;
        }
        return NULL;
}

BoincError boincProxyRequestWork(BoincProxyWorkUnit * pwu) 
{
        char* createFile = "workRequest.xml";
        char filepath[BUFF_SIZE];

        initFilename(filepath, BUFF_SIZE, pwu->wu->workingDir);
        if (appendFilename(filepath, BUFF_SIZE, createFile) == NULL) {
                return boincErrorCreate(kBoincError, kBoincFileSystem, "File path too long");
        }

        FILE * file = boincFopen(filepath, "w+");
        if (file == NULL) {
                return boincErrorCreatef(kBoincError, kBoincFileSystem, "Cannot open file %s", filepath);
        }

        BoincProxy * proxy = pwu->proxy;

        char strBuff[BUFF_SIZE];
        fprintf(file, "<scheduler_request>\n");
        fprintf(file, "<authenticator>%s</authenticator>\n", 
                boincConfigurationGetString(proxy->conf, kBoincUserAuthenticator, strBuff, BUFF_SIZE));
        fprintf(file, "<hostid>%s</hostid>\n", boincConfigurationGetString(proxy->conf, kBoincHostID, strBuff, BUFF_SIZE));
        fprintf(file, "<rpc_seqno>0</rpc_seqno>\n");
        fprintf(file, "<core_client_major_version>6</core_client_major_version>\n"); //1
        fprintf(file, "<core_client_minor_version>2</core_client_minor_version>\n"); //1
        fprintf(file, "<core_client_release>19</core_client_release>\n");            //1
        fprintf(file, "<work_req_seconds>1.000000</work_req_seconds>\n");
        fprintf(file, "<resource_share_fraction>1.000000</resource_share_fraction>\n");
        fprintf(file, "<rrs_fraction>1.000000</rrs_fraction>\n");
        fprintf(file, "<prrs_fraction>1.000000</prrs_fraction>\n");
        fprintf(file, "<estimated_delay>0.000000</estimated_delay>\n");
        fprintf(file, "<duration_correction_factor>1.000000</duration_correction_factor>\n");
        fprintf(file, "<client_cap_plan_class>1</client_cap_plan_class>\n");
        fprintf(file, "<platform_name>%s</platform_name>\n", boincConfigurationGetString(proxy->conf, kBoincPlatformName, strBuff, BUFF_SIZE));
        fprintf(file, "<working_global_preferences>\n");
        fprintf(file, "<global_preferences>\n");
        fprintf(file, "<mod_time>0.000000</mod_time>\n");
        fprintf(file, "<run_on_batteries/>\n");
        fprintf(file, "<run_if_user_active/>\n");
        fprintf(file, "<suspend_if_no_recent_input>0.000000</suspend_if_no_recent_input>\n");
        fprintf(file, "<start_hour>0.000000</start_hour>\n");
        fprintf(file, "<end_hour>0.000000</end_hour>\n");
        fprintf(file, "<net_start_hour>0.000000</net_start_hour>\n");
        fprintf(file, "<net_end_hour>0.000000</net_end_hour>\n");
        fprintf(file, "<confirm_before_connecting/>\n");
        fprintf(file, "<work_buf_min_days>0.100000</work_buf_min_days>\n");
        fprintf(file, "<work_buf_additional_days>0.250000</work_buf_additional_days>\n");
        fprintf(file, "<max_ncpus_pct>100.000000</max_ncpus_pct>\n");
        fprintf(file, "<cpu_scheduling_period_minutes>60.000000</cpu_scheduling_period_minutes>\n");
        fprintf(file, "<disk_interval>60.000000</disk_interval>\n");
        fprintf(file, "<disk_max_used_gb>10.000000</disk_max_used_gb>\n");
        fprintf(file, "<disk_max_used_pct>50.000000</disk_max_used_pct>\n");
        fprintf(file, "<disk_min_free_gb>0.100000</disk_min_free_gb>\n");
        fprintf(file, "<vm_max_used_pct>75.000000</vm_max_used_pct>\n");
        fprintf(file, "<ram_max_used_busy_pct>50.000000</ram_max_used_busy_pct>\n");
        fprintf(file, "<ram_max_used_idle_pct>90.000000</ram_max_used_idle_pct>\n");
        fprintf(file, "<idle_time_to_run>3.000000</idle_time_to_run>\n");
        fprintf(file, "<max_bytes_sec_up>0.000000</max_bytes_sec_up>\n");
        fprintf(file, "<max_bytes_sec_down>0.000000</max_bytes_sec_down>\n");
        fprintf(file, "<cpu_usage_limit>100.000000</cpu_usage_limit>\n");
        fprintf(file, "</global_preferences>\n");
        fprintf(file, "</working_global_preferences>\n");
        fprintf(file, "<cross_project_id></cross_project_id>\n");
        fprintf(file, "<time_stats>\n");
        fprintf(file, "<on_frac>1.000000</on_frac>\n");
        fprintf(file, "<connected_frac>1.000000</connected_frac>\n");
        fprintf(file, "<active_frac>0.999988</active_frac>\n");
        fprintf(file, "<cpu_efficiency>1.000000</cpu_efficiency>\n");
        fprintf(file, "</time_stats>\n");
        fprintf(file, "<net_stats>\n");
        fprintf(file, "<bwup>%s</bwup>\n", boincConfigurationGetString(proxy->conf, kBoincNetworkUpBandwidth, strBuff, BUFF_SIZE));
        fprintf(file, "<avg_up>%s</avg_up>\n", boincConfigurationGetString(proxy->conf, kBoincNetworkUpAverage, strBuff, BUFF_SIZE));
        fprintf(file, "<avg_time_up>%s</avg_time_up>\n", boincConfigurationGetString(proxy->conf, kBoincNetworkUpAvgTime, strBuff, BUFF_SIZE));
        fprintf(file, "<bwdown>%s</bwdown>\n", boincConfigurationGetString(proxy->conf, kBoincNetworkDownBandwidth, strBuff, BUFF_SIZE));
        fprintf(file, "<avg_down>%s</avg_down>\n", boincConfigurationGetString(proxy->conf, kBoincNetworkDownAverage, strBuff, BUFF_SIZE));
        fprintf(file, "<avg_time_down>%s</avg_time_down>\n", boincConfigurationGetString(proxy->conf, kBoincNetworkDownAvgTime, strBuff, BUFF_SIZE));
        fprintf(file, "</net_stats>\n");
        fprintf(file, "<host_info>\n");
        fprintf(file, "<timezone>%s</timezone>\n", boincConfigurationGetString(proxy->conf, kBoincHostTimezone, strBuff, BUFF_SIZE));
        fprintf(file, "<domain_name>%s</domain_name>\n", boincConfigurationGetString(proxy->conf, kBoincHostDomainName, strBuff, BUFF_SIZE));
        fprintf(file, "<ip_addr>%s</ip_addr>\n", boincConfigurationGetString(proxy->conf, kBoincHostIpAddress, strBuff, BUFF_SIZE));
        fprintf(file, "<host_cpid>%s</host_cpid>\n", boincConfigurationGetString(proxy->conf, kBoincHostCPID, strBuff, BUFF_SIZE));
        fprintf(file, "<p_ncpus>%s</p_ncpus>\n", boincConfigurationGetString(proxy->conf, kBoincHostNCpu, strBuff, BUFF_SIZE));
        fprintf(file, "<p_vendor>%s</p_vendor>\n", boincConfigurationGetString(proxy->conf, kBoincHostCpuVendor, strBuff, BUFF_SIZE));
        fprintf(file, "<p_model>%s</p_model>\n", boincConfigurationGetString(proxy->conf, kBoincHostCpuModel, strBuff, BUFF_SIZE));
        fprintf(file, "<p_features>%s</p_features>\n", boincConfigurationGetString(proxy->conf, kBoincHostCpuFeatures, strBuff, BUFF_SIZE));
        fprintf(file, "<p_fpops>%s</p_fpops>\n", boincConfigurationGetString(proxy->conf, kBoincHostCpuFpops, strBuff, BUFF_SIZE));
        fprintf(file, "<p_iops>%s</p_iops>\n", boincConfigurationGetString(proxy->conf, kBoincHostCpuIops, strBuff, BUFF_SIZE));
        fprintf(file, "<p_membw>%s</p_membw>\n", boincConfigurationGetString(proxy->conf, kBoincHostCpuMemBW, strBuff, BUFF_SIZE));
        fprintf(file, "<p_calculated>%s</p_calculated>\n", boincConfigurationGetString(proxy->conf, kBoincHostCpuCalculated, strBuff, BUFF_SIZE));
        fprintf(file, "<m_nbytes>%s</m_nbytes>\n", boincConfigurationGetString(proxy->conf, kBoincHostMemBytes, strBuff, BUFF_SIZE));
        fprintf(file, "<m_cache>%s</m_cache>\n", boincConfigurationGetString(proxy->conf, kBoincHostMemCache, strBuff, BUFF_SIZE));
        fprintf(file, "<m_swap>%s</m_swap>\n", boincConfigurationGetString(proxy->conf, kBoincHostMemSwap, strBuff, BUFF_SIZE));
        fprintf(file, "<d_total>%s</d_total>\n", boincConfigurationGetString(proxy->conf, kBoincHostDiskTotal, strBuff, BUFF_SIZE));
        fprintf(file, "<d_free>%s</d_free>\n", boincConfigurationGetString(proxy->conf, kBoincHostDiskFree, strBuff, BUFF_SIZE));
        fprintf(file, "<os_name>%s</os_name>\n", boincConfigurationGetString(proxy->conf, kBoincHostOSName, strBuff, BUFF_SIZE));
        fprintf(file, "<os_version>%s</os_version>\n", boincConfigurationGetString(proxy->conf, kBoincHostOSVersion, strBuff, BUFF_SIZE));
        fprintf(file, "<accelerators>%s</accelerators>\n", boincConfigurationGetString(proxy->conf, kBoincHostAccelerators, strBuff, BUFF_SIZE));
        fprintf(file, "</host_info>\n");
        fprintf(file, "<disk_usage>\n");
        fprintf(file, "<d_boinc_used_total>%s</d_boinc_used_total>\n", boincConfigurationGetString(proxy->conf, kBoincTotalDiskUsage, strBuff, BUFF_SIZE));
        fprintf(file, "<d_boinc_used_project>%s</d_boinc_used_project>\n", boincConfigurationGetString(proxy->conf, kBoincProjectDiskUsage, strBuff, BUFF_SIZE));
        fprintf(file, "</disk_usage>\n");
        fprintf(file, "<result>\n");
        fprintf(file, "</result>\n");
        fprintf(file, "<other_results>\n");
        fprintf(file, "</other_results>\n");
        fprintf(file, "<in_progress_results>\n");
        fprintf(file, "</in_progress_results>\n");
        fprintf(file, "</scheduler_request>\n");
        boincFclose(file);

        boincDebugf("boincProxyRequestWork: data writen in %s", filepath);

        BoincProxyUrl url = {boincConfigurationGetString(proxy->conf, kBoincCGIURL, strBuff, BUFF_SIZE),
                             "", 0, NULL};
        char * files[] = {filepath};
        BoincProxyPostData post = { NULL, 1,  files, 0, NULL, 0, 0 };

        char xmlFileres[BUFF_SIZE];
        BoincError err = boincProxyPost(proxy, &url, &post, boincGetRequestWorkFile(pwu->wu, xmlFileres));

        if (err) {
                boincLogError(err);
                return err;
        }
  
        err = boincConfigurationUpdateFromFile(proxy->conf, xmlFileres);
        if (err) {
                boincLogError(err);
                boincErrorDestroy(err); // if the update failed, don't stop
                err = NULL;
        }

        err = boincProxyLoadWorkUnit(pwu->wu);
        if (err) {
                return err;
        }

        return NULL;
}

static int boincProxyUploadXMLBegin(const char* elementName, const char** attr, void* userData) 
{
        if (!strcmp(elementName, "status"))
                return 2;
        if (!strcmp(elementName, "message"))
                return 3;
        return -1;
}

static BoincError boincProxyUploadXMLEnd(int index, const char* elementValue, void* userData)
{
        int * status = (int*) userData;
        if (index == 2) {
                boincDebugf("Upload set status: %s", elementValue);
                *status = atoi(elementValue);
        }
        if (index == 3 && *status != 0) {
                boincDebugf("Upload set message: %s", elementValue);
                return boincErrorCreate(kBoincError, *status, elementValue);
        }
        return NULL;
}

BoincError boincProxyUploadFile(BoincProxyWorkUnit * pwu, BoincFileInfo* fileinfo, int filenum, int filetotal)
{
        BoincWorkUnit * wu = pwu->wu;

        BoincProxyUrl url = {fileinfo->url, "", 0, NULL};

        char preuploadHeaderFile[BUFF_SIZE];
        boincGetAbsolutePath(wu->workingDir, "preuploadHeader.xml", preuploadHeaderFile, BUFF_SIZE);
        FILE * file = boincFopen(preuploadHeaderFile, "w+");
        fprintf(file, "<data_server_request>\n");
        fprintf(file, "    <core_client_major_version>6</core_client_major_version>\n");
        fprintf(file, "    <core_client_minor_version>2</core_client_minor_version>\n");
        fprintf(file, "    <core_client_release>19</core_client_release>\n");
        fprintf(file, "    <get_file_size>%s</get_file_size>\n", fileinfo->name);
        fprintf(file, "</data_server_request>\n");
        boincFclose(file);

        char * prefiles[] = {preuploadHeaderFile};
        BoincProxyPostData prepost = { pwu, 1,  prefiles, 0, NULL, 0, 0 };
        char preuploadResult[BUFF_SIZE];
        boincGetAbsolutePath(wu->workingDir, "preuploadResponse.xml", preuploadResult, BUFF_SIZE);
        BoincError err = boincProxyPost(pwu->proxy, &url, &prepost, preuploadResult);

        if (err)
                return err;

        char uploadData[BUFF_SIZE];
        boincWorkUnitGetPath(wu, fileinfo, uploadData, BUFF_SIZE);

        struct stat bStat;
        if (stat(uploadData, &bStat))
                return boincErrorCreatef(kBoincError, kBoincFileSystem, "cannot access to %s", uploadData);

        fileinfo->nBytes = bStat.st_size;

        if (!fileinfo->checksum) {
                fileinfo->checksum = boincArray(char, 33);
                if (fileinfo->checksum == NULL) {
                        return boincErrorCreate(kBoincFatal, kBoincSystem, "Out of memory");
                }
        }
        boincHashMd5File(uploadData, fileinfo->checksum);

        char uploadHeaderFile[BUFF_SIZE];
        boincGetAbsolutePath(wu->workingDir, "uploadHeader.xml", 
                             uploadHeaderFile, BUFF_SIZE);
        file = boincFopen(uploadHeaderFile, "w+");

        if (file == NULL) {
                return boincErrorCreatef(kBoincError, kBoincFileSystem, 
                                         "cannot create/open xml data file %s", 
                                         uploadHeaderFile);
        }
        fprintf(file, "<data_server_request>\n");
        fprintf(file, "    <core_client_major_version>6</core_client_major_version>\n");
        fprintf(file, "    <core_client_minor_version>2</core_client_minor_version>\n");
        fprintf(file, "    <core_client_release>19</core_client_release>\n");
        fprintf(file, "<file_upload>\n");
        fprintf(file, "<file_info>\n");
        fprintf(file, "    <name>%s</name>\n", fileinfo->name);
        if (fileinfo->flags & kBoincFileGenerated)
                fprintf(file, "    <generated_locally/>\n");
        if (fileinfo->flags & kBoincFileUpload)
                fprintf(file, "    <upload_when_present/>\n");
        fprintf(file, "    <max_nbytes>%d</max_nbytes>\n", fileinfo->maxBytes);
        fprintf(file, "    <url>%s</url>\n", fileinfo->url);
        fprintf(file, "<xml_signature>%s</xml_signature>\n", fileinfo->xmlSignature);
        fprintf(file, "</file_info>\n");
        fprintf(file, "<nbytes>%d</nbytes>\n", fileinfo->nBytes);
        fprintf(file, "<md5_cksum>%s</md5_cksum>\n", fileinfo->checksum);
        fprintf(file, "<offset>0</offset>\n");
        fprintf(file, "<data>\n");
        boincFclose(file);

        char * files[] = {uploadHeaderFile, uploadData};
        BoincProxyPostData post = { pwu, 2,  files, 0, NULL, 0, 0 };

        char uploadResult[BUFF_SIZE];
        boincGetAbsolutePath(wu->workingDir, "uploadResponse.xml", uploadResult, BUFF_SIZE);

        err = boincProxyPost(pwu->proxy, &url, &post, uploadResult);

        if (err)
                return err;

        int status = 0;
        err = boincXMLParse(uploadResult, boincProxyUploadXMLBegin, boincProxyUploadXMLEnd, &status);
        return err;
}

BoincError boincProxyChangeWorkUnitProgress(BoincProxyWorkUnit* pwu, float progress)
{
        if (pwu->wu != NULL) 
                pwu->wu->progress = progress;
  
        if (pwu->proxy->statusCallback != NULL) {
                pwu->proxy->statusCallback(pwu->proxy->statusData, pwu->wu->index);
        }

        return NULL;
}

BoincError boincProxyDownloadFile(BoincProxyWorkUnit* pwu, BoincFileInfo* file, int filenum, int filetotal) 
{
        char * filename;
        if (!file->url || !strlen(file->url))
                return boincErrorCreate(kBoincError, kBoincInternal, "no url provided");

        if (file->openname && strlen(file->openname) ) {
                filename = file->openname;
        } else {
                filename = file->name;
        }

        char fullpath[BUFF_SIZE];
        boincGetAbsolutePath(pwu->wu->workingDir, filename, fullpath, BUFF_SIZE);
  
        boincDebugf("wd: %s ; fn: %s => %s", pwu->wu->workingDir, filename, fullpath);

        BoincProxyUrl url = {file->url, "", 0, NULL};
		
        if (filetotal < 1) {
                filetotal = 1;
        }
        boincHttpSetProgressCallback((BoincHttpProgressCallback) boincProxyChangeWorkUnitProgress, pwu, filenum, filetotal);

        BoincError err = boincProxyGet(pwu->proxy, &url, fullpath);

        mode_t mode = S_IWUSR | S_IRUSR;
        if ((file->flags & kBoincFileExecutable) || (file->flags & kBoincFileMainProgram)) {
                boincDebugf("boincProxyDownloadFile: File executable !");
                mode |= S_IXUSR;
        }
        chmod(fullpath, mode);

        return err;
}

void boincProxySetStatusChangedCallback(BoincProxy * proxy, 
					BoincStatusChangedCallback callback, 
					void *userData) 
{
        proxy->statusCallback = callback;
        proxy->statusData = userData;
}


/* Only used in BoincProxyTest.c */
BoincConfiguration * boincProxyGetConfiguration(BoincProxy* proxy)
{
	return proxy->conf;
}

