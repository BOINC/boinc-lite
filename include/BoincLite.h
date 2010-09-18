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


  Quick Start:
  
  BoincScheduler tries to define the minimum API to realise a simple
  BOINC client. Its main goal is to download work units and upload the
  results. It has a fair number of restrictions:

  - It handles only one BOINC project.

  - It doesn't manage the resource usage (CPU, memory, disk) or
    scheduling between projects and work units. If you need this,
    you'll have to implement this in your application.

  - It doesn't provide any glue to build a user interface. Again,
    you'll have to manage this yourself.

  - Only one workunit is computed at once and at most one workunit is
    waiting to be executed. 

  - The downloads and uploads are performed sequentialy.

  - It doesn't use multi-threading but it is straighforward to spin of
    separate threads for the scheduler and for the computation of the
    work unit.

  The following code is an example of a single-threaded application
  that computes three work units and then quits.

  // Example:

  int completedWorkUnits = 0

  int computeWorkUnit(BoincScheduler* scheduler, BoincWorkUnit* workunit, void* userData)
  {
      // Do the computation.

      boincSchedulerSetWorkUnitStatus(scheduler, wu, kBoincWorkUnitFinished, 0);
      completedWorkUnits += 1;
  }

  int main(int argc, char** argv)
  {
      int error = 0;
      char* projectDirectory = "/var/lib/boinc-lite/";
      char* url = "http://boinc.jungle.org/project/";
      char* userid = "monkey";
      char* password = "banana";

      // Create the configuration and load it from disk (if present).
      BoincConfiguration* configuration = boincConfigurationCreate(projectDirectory);
      error = boincConfigurationLoad(configuration);

      // Make sure there's a project URL and a BOINC account before
      // creating the scheduler.
      if (!boincConfigurationHasParameter(configuration, kBoincProjectURL)) {
          boincConfigurationSetString(configuration, kBoincProjectURL, url);
      }
      if (!boincConfigurationHasParameter(configuration, kBoincUserID)) {
          boincConfigurationSetString(configuration, kBoincUserID, userid);
      }
      if (!boincConfigurationHasParameter(configuration, kBoincUserPassword)) {
          boincConfigurationSetString(configuration, kBoincUserPassword, password);
      }

      // Create the scheduler and load its previous state (if any).
      BoincScheduler* scheduler = boincSchedulerCreate(configuration, computeWorkUnit, NULL);
      
      while (1) {
          // Update the state of the scheduler. Whenever a work unit is
	  // ready to be computed, the scheduler will call the
	  // computeWorkUnit function
	  error = boincSchedulerHandleEvents(scheduler);
	  if (error != 0)  {
	      break;
	  }
	  if (completedWorkUnits == 3) {
	      break;
          }
      }

      // Save the state of the scheduler to disk
      boincSchedulerSuspend(scheduler);
      boincSchedulerDestroy(scheduler);      

      // Save the configuration
      boincConfigurationStore();
      boincConfigurationDestroy();
  } 



*/ 

#ifndef __BoincLite_H__
#define __BoincLite_H__


#ifdef __cplusplus
extern "C" {
#endif


/*** Forward declarations ***/

typedef struct _BoincError* BoincError;
typedef struct _BoincLogger BoincLogger;
typedef struct _BoincConfiguration BoincConfiguration;
typedef struct _BoincScheduler BoincScheduler;



/*** BoincScheduler ***/

typedef struct _BoincFileInfo BoincFileInfo;

struct _BoincFileInfo {
	char* name;
	char* openname;
	char* url;
	char* checksum;
	char* signature;
	char* xmlSignature;
	unsigned int nBytes;
	unsigned int maxBytes;
	int flags;
	BoincFileInfo* next;
	BoincFileInfo* gnext;
};


typedef struct _BoincApp {
	char* name;
	char* friendlyName;
	char* version;
	char* apiVersion;
	BoincFileInfo* fileInfo;
} BoincApp;

typedef struct _BoincResult {
	char* name;
	unsigned int deadline;
	BoincFileInfo* fileInfo;
        unsigned int cpu_time;
} BoincResult;

typedef struct _BoincWorkUnit {
	char* name;
	double estimatedFlops;
	double estimatedMemory;
	double estimatedDisk;
	BoincApp* app;
	BoincFileInfo* fileInfo;
	BoincFileInfo* globalFileInfo;
	BoincResult* result;
	char* workingDir;	
	int dateStatusChange;
	int status;
	float progress;
	int delay;
	BoincError error;	
	int index;
	BoincFileInfo ** currentFileInfo;
} BoincWorkUnit;

typedef struct _BoincWorkUnitStatus {
	int status;
	float progress;
	int delay;
	int errorType;
	int errorCode;
	char errorMessage[256];
} BoincWorkUnitStatus;

enum {
	kBoincWorkUnitCreated = 0,

	kBoincWorkUnitInitializing,    // has progress info
	kBoincWorkUnitDefined,
	kBoincWorkUnitDownloading,           // has progress info
	kBoincWorkUnitWaiting,
	kBoincWorkUnitComputing,             // has progress info
	kBoincWorkUnitFinished,
	kBoincWorkUnitUploading,             // has progress info
	kBoincWorkUnitCompleted,
	kBoincWorkUnitLoading,         // has progress info (progress of reading data from disk)
	kBoincWorkUnitFailed,         // has error code
	kBoincWorkUnitLastStatus,
};

/** Compute callback
 *
 *  Must call boincSchedulerSetWorkUnitStatus to inform at least the workunit computation is finished
 */
typedef int (*BoincComputeWorkUnitCallback)(BoincScheduler* scheduler, BoincWorkUnit* workunit, void* userData);


/** \brief Create a new scheduler
 *
 *  Create a new scheduler.
 *
 *  \return a pointer to the scheduler or NULL in case of a failure.
 */ 
BoincScheduler* boincSchedulerCreate(BoincConfiguration* configuration,
				     BoincComputeWorkUnitCallback callback, 
				     void* userData);


/** \brief Release all the resources of the scheduler. 
 *
 *  \return zero if no error occured.
 */
void boincSchedulerDestroy(BoincScheduler* scheduler);



typedef void (*BoincStatusChangedCallback)(void *userData, int index);

BoincError boincSchedulerSetStatusChangedCallback(BoincScheduler * scheduler, 
						  BoincStatusChangedCallback callback, 
						  void *userData);



int boincSchedulerHasEvents(BoincScheduler* scheduler);


/** \brief Update the state of the scheduler.
 *
 *  The controlling application should call
 *  boincSchedulerHandleEvents() at regular intervals so it can handle
 *  events and update its state the asynchrounous tasks.
 *
 *  \return zero if no error occured.
 */
BoincError boincSchedulerHandleEvents(BoincScheduler* scheduler);


/** \brief How many work units are available locally.
 *
 *  Normally, there should ne be more than three work units that
 *  exists locally: the running work unit, the next work unit, and the
 *  previous work units if it hasn't finished uploading yet.
 *
 *  \return the number of defined workunits 
 */
int boincSchedulerCountWorkUnits(BoincScheduler* scheduler);


/** \brief Obtain the status information of a work unit.
 */
void boincSchedulerGetWorkUnitStatus(BoincScheduler* scheduler, BoincWorkUnitStatus* status, int index);


/** \brief Signal the scheduler the progress of the a work unit.
 *  
 *  The controlling application should regularly call
 *  boincSchedulerChangeWorkUnitStatus during the computation to
 *  indicate the scheduler the progress of the active work unit or
 *  signal any state change (finished, error, ...). Using this
 *  information, the scheduler can plannify the downloads and uploads
 *  of work units.
 *
 *  \return non-zero if an error occured.
 */
BoincError boincSchedulerChangeWorkUnitStatus(BoincScheduler* scheduler, 
					      BoincWorkUnit* workunit, 
					      int status);
  

BoincError boincSchedulerSetWorkUnitProgress(BoincScheduler* scheduler, 
					     BoincWorkUnit* workunit, 
					     float progress);

/** \brief Change the login used to connect to the server.
 *
 *  This utility function has several side effects. First, it checks
 *  whether the login is valid. If successful, it puts the new id,
 *  password, and the new authenticator string returned by the BOINC
 *  server in the configuration and saves it for future use. After a
 *  successful call to this function, all subsequent request to the
 *  server will use the new login.
 */
BoincError boincSchedulerChangeAuthentication(BoincScheduler* scheduler,
					      const char* email, 
					      const char* password);

/** \brief Obtain a pointer to the configuration object
 */
BoincConfiguration* boincSchedulerGetConfiguration(BoincScheduler* scheduler);




/*** BoincConfiguration ***/

enum BoincConfigurationIndex {
	kBoincProjectURL = 0,          // string
	kBoincCGIURL,                  // string
	kBoincHostID,                  // string
	kBoincUserEmail,               // string
	kBoincUserFullname,                // string
	kBoincUserPassword,            // string
	kBoincUserAuthenticator,       // string
	kBoincUserTotalCredit,         // number
	kBoincUserTeamName,            // string
	kBoincHostTimezone,            // number
	kBoincHostDomainName,          // string
	kBoincHostIpAddress,           // string
	kBoincHostCPID,                // string
	kBoincHostNCpu,                // number
	kBoincHostCpuVendor,           // string
	kBoincHostCpuModel,            // string
	kBoincHostCpuFeatures,         // string
	kBoincHostCpuFpops,            // number
	kBoincHostCpuIops,             // number
	kBoincHostCpuMemBW,            // number
	kBoincHostCpuCalculated,       // number
	kBoincHostMemBytes,            // number
	kBoincHostMemCache,            // number
	kBoincHostMemSwap,             // number
	kBoincHostMemTotal,            // number
	kBoincHostDiskTotal,           // number
	kBoincHostDiskFree,            // number
	kBoincHostOSName,              // string
	kBoincHostOSVersion,           // string
	kBoincHostAccelerators,        // string
	kBoincNetworkUpBandwidth,      // number
	kBoincNetworkUpAverage,        // number
	kBoincNetworkUpAvgTime,        // number
	kBoincNetworkDownBandwidth,    // number
	kBoincNetworkDownAverage,      // number
	kBoincNetworkDownAvgTime,      // number
	kBoincTotalDiskUsage,          // number
	kBoincProjectDiskUsage,        // number
	kBoincHttpProxy,               // string
	kBoincProjectDirectory,        // string
	kBoincProjectConfigurationFile,// string
	kBoincProjectConfigurationTmpFile, // string
	kBoincPlatformName,            // string  
	kBoincWorkUnitDirectory,       // string
	kBoincUserTotalWorkunits,      // number
	kBoincLanguage,                // string
	kBoincLastIndexEnum,           //DO NOT USE : internal use only
};


/** \brief Create a new configuration.
 *
 *  boincConfigurationCreate allocates the ressources for the
 *  configuration. It takes as argument, the path of the directory
 *  where it can load/store the configuration file(s).
 *  
 *  \param[in] dir diretory where the configuration is stored (root project directory)
 *
 *  \returns a pointer to the new configuration object 
 */
BoincConfiguration* boincConfigurationCreate(const char* dir);


/** \brief Destroy a configuration structure
 *
 *  \returns NULL or BoincError in case of error
 */
BoincError boincConfigurationDestroy(BoincConfiguration* configuration);


BoincError boincConfigurationLoad(BoincConfiguration* configuration);


BoincError boincConfigurationStore(BoincConfiguration* configuration);


/** \brief Check availability of a parameter.
 *
 *  \param[in] paramater index of the parameter (cf. BoincConfigurationIndex enum)
 *
 *  \returns zero is the parameter is set, non-zero otherwise.
 */
int boincConfigurationHasParameter(BoincConfiguration* configuration, int parameter);


/** \brief Set a parameter value.
 *
 *  The following functions are used to set the value of a
 *  parameter. 
 *  \param[in] paramater index of the parameter to set (cf. BoincConfigurationIndex)
 *  \param[in] value the string value to set
 *
 *  \returns zero on success, non-zero otherwise.
 */
BoincError boincConfigurationSetString(BoincConfiguration* configuration, int parameter, const char* value);


/** \brief Set a parameter value.
 *
 *  The following functions are used to set the value of a
 *  parameter. 
 *  \param[in] paramater index of the parameter to set (cf. BoincConfigurationIndex)
 *  \param[in] value the number value to set
 *
 *  \returns zero on success, non-zero otherwise.
 */
BoincError boincConfigurationSetNumber(BoincConfiguration* configuration, int parameter, double value);


/** \brief Update a configuration struture from xml file
 *
 *  If the xml file contains configuration tags for a known parameter, the parameter is updated
 * 
 *  \param[in] xmlFileName the path to the xml file can contain configuration information
 *
 */
BoincError boincConfigurationUpdateFromFile(BoincConfiguration * configuration, const char * xmlFileName);


/** \brief returns true if the parameter is a string
 */
int boincConfigurationIsString(int parameter);


/** \brief returns true if the parameter is a number
 */
int boincConfigurationIsNumber(int parameter);


/** \brief returns true if the parameter is readonly
 */
int boincConfigurationIsReadOnly(int parameter);


/** \brief Obtain a parameter value.
 *
 *  \param[in] paramater index of the parameter to set (cf. BoincConfigurationIndex)
 *  \param[out] str string where the configuration value will be stored
 *  \param[in] len max length of str
 *
 *  \returns the value as a string pointer or a floating point 
 */
char* boincConfigurationGetString(BoincConfiguration* configuration, int parameter, char* str, int len);


/** \brief Obtain a parameter value.
 *
 *  \param[in] paramater index of the parameter to set (cf. BoincConfigurationIndex)
 *
 *  \returns the value as a floating point 
 */
double boincConfigurationGetNumber(BoincConfiguration* configuration, int parameter);




/*** Utility functions ***/

/* If fileInfo is NULL, the path of the workunit will be returned. */
char* boincWorkUnitGetPath(BoincWorkUnit* workunit, BoincFileInfo * fileInfo, char * str, unsigned int len);


void boincPasswordHash(const char* email, const char* password, char hash[33]);



/*** BoincError ***/
  
enum _boincErrorType {
	kBoincNone = 0,
	kBoincDebug = 1,
	kBoincDelay = 2,
	kBoincInfo = 3,
	kBoincError = 4,
	kBoincFatal = 5,
};

enum _boincErrorCode {
	kBoincUndefined = 0,
	kBoincInternal = 1,
	kBoincSystem = 2,
	kBoincFileSystem = 3,
	kBoincNetwork = 4,
	kBoincServer = 5,
	kBoincAuthentication = 6,
};


/** \brief Create a BoincError object.
 *  \param[in] code identifier of the error
 *  \param[in] message a string explaining the error
 */
BoincError boincErrorCreate(int type, int code, const char* message);


/** \brief Create a BoincError object.
 *  \param[in] code identifier of the error
 *  \param[in] message a formated string explaining the error
 */
BoincError boincErrorCreatef(int type, int code, const char* format, ...);

/** \brief Destroy a BoincError object.
 */
void boincErrorDestroy(BoincError error);


/** \brief returns the error type.
 */
int boincErrorType(BoincError error);


/** \brief returns the error code.
 */
int boincErrorCode(BoincError error);


/** \brief returns the error message.
 */
char* boincErrorMessage(BoincError error);




/*** BoincLogger ***/

/** \brief definition of the function called write logs
 *  \param[in] type log level (cf BoincErrorType)
 *  \param[in] code number identifying the error
 *  \param[in] message string explaining the error
 */ 
typedef void (*BoincLogWrite)(BoincLogger * logger, int type, int code, const char* message);


struct _BoincLogger
{
	BoincLogWrite write;
	void* userData;
}; 

/** \brief default log function (cf. BoincLogWrite)
 *
 *  logger->userData defines from what log type the message is printed
 */
void boincDefaultLogFunction(BoincLogger* logger, int type, int code, const char* message);

/** \brief define a logger
 *  Example :
 *    BoincLogger log = { logFunction, (void*) 9 };
 *    boincLogInit(&log);
 *
 */
BoincError boincLogInit(BoincLogger * logger);

/** \brief log an error
 *
 */
void boincLogError(BoincError err);

/** \brief log a message
 *
 */
void boincLog(int type, int code, const char* message);

/** \brief log a message
 *
 */
void boincLogf(int type, int code, const char* format, ...);

//#if __DEBUG__
#define boincDebug(_message) boincLog(kBoincDebug, 0, _message)
#define boincDebugf(_message, args...) boincLogf(kBoincDebug, 0, _message, ## args)
/*#else
#define boincDebug(_message) 
#endif*/

#ifdef __cplusplus
}
#endif

#endif // __BoincLite_H__
