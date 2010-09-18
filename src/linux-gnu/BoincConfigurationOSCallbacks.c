#include "BoincConfigurationOSCallbacks.h"
#include "BoincError.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define __USE_POSIX2
#include <stdio.h>


unsigned int boincGetStringFromExec(char * res, unsigned int max_res, const char * format_cmd, ...) 
{
  char cmd [255];
  va_list arglist;

  va_start(arglist,format_cmd);
  vsnprintf(cmd, 255, format_cmd, arglist);
  va_end(arglist);
  strcat(cmd, " | tr -d '\\n' ");

  boincDebugf("pipe exec cmd: %s", cmd);

  FILE * file = popen(cmd, "r");
  if (file == NULL) {
    boincDebugf("pipe '%s' could not be opened", cmd);
    return 0;
  }

  size_t read = fread(res, sizeof(char), max_res -1, file);
  res[read] = 0;
  boincDebugf("exec result: %s (%d)", res, read);
  fclose(file);
  return read;
}

unsigned int boincConfigurationGetOSName(char * str, unsigned int max_size, BoincConfiguration * conf) 
{
  return boincGetStringFromExec(str, max_size, "uname -s"); //strlen(strcpy(str, "GNU/Linux"));
}
  
unsigned int boincConfigurationGetNCpu(char * str, unsigned int max_size, BoincConfiguration * conf) 
{
#ifdef __APPLE__
  return 1;
#else
  return boincGetStringFromExec(str, max_size, "grep processor /proc/cpuinfo | wc -l");
#endif
}

unsigned int boincConfigurationGetPlatformName(char* str, unsigned int max_size, BoincConfiguration * conf) 
{
#if defined(__APPLE__)
  #if defined(__i386__)
    return strlen(strcpy(str, "i686-apple-darwin"));
  #else
    #error Unsupported platform
  #endif
#elif defined(__linux__)
  #if defined(__i386__)
    return strlen(strcpy(str, "i686-pc-linux-gnu"));
  #elif defined(__powerpc__)
    return strlen(strcpy(str, "powerpc-linux-gnu"));
  #else
    #error Unsupported platform
  #endif
#else
    #error Unsupported platform
#endif
}
  
unsigned int boincConfigurationGetNetworkUpBandwidth(char * str, unsigned int max_size, BoincConfiguration * conf) 
{
  return strlen(strcpy(str, "0.0000"));
}

unsigned int boincConfigurationGetNetworkUpAverage(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return strlen(strcpy(str, "0.0000"));
}

unsigned int boincConfigurationGetNetworkUpAvgTime(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return strlen(strcpy(str, "0.0000"));
}

unsigned int boincConfigurationGetNetworkDownBandwidth(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return strlen(strcpy(str, "0.0000"));
}

unsigned int boincConfigurationGetNetworkDownAverage(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return strlen(strcpy(str, "0.0000"));
}

unsigned int boincConfigurationGetNetworkDownAvgTime(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return strlen(strcpy(str, "0.0000"));
}

unsigned int boincConfigurationGetHostTimezone(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return strlen(strcpy(str, "3600"));
}

unsigned int boincConfigurationGetHostDomainName(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__linux__)
  char ip[20];
  return boincGetStringFromExec(str, max_size, 
				"host %s | awk '{print $5}' | sed 's/^[^\\.]*\\.//' | sed 's/\\.$//'", 
				boincConfigurationGetString(conf, kBoincHostIpAddress, ip, 20));
#else
    return strlen(strcpy(str, "unknown"));
#endif
}

unsigned int boincConfigurationGetHostIpAddress(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return boincGetStringFromExec(str, max_size, "/sbin/ifconfig | grep 'inet' | head -n 1 | cut -d':' -f2 | cut -d' ' -f1");
}

unsigned int boincConfigurationGetHostCpuVendor(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__APPLE__)
  #if defined(__i386__)
    return strlen(strcpy(str, "Intel"));
  #endif
#elif defined(__linux__)
  return boincGetStringFromExec(str, max_size, "grep vendor_id /proc/cpuinfo | head -n 1 | sed 's/.* *: //' ");  
#else
    #error Unsupported platform
#endif
}

unsigned int boincConfigurationGetHostCpuModel(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__APPLE__)
  #if defined(__i386__)
    const char* value = "Intel";
    strcat(str, value);
    return strlen(value);
  #endif
#elif defined(__linux__)
  #if defined(__i386__)
    return boincGetStringFromExec(str, max_size, "grep 'model name' /proc/cpuinfo | head -n 1 | sed 's/.* *: //' ");  
  #elif defined(__powerpc__)
    return boincGetStringFromExec(str, max_size, "grep cpu /proc/cpuinfo | head -n 1 | sed 's/.* *: //' ");  
  #endif
#else
    #error Unsupported platform
#endif
}

unsigned int boincConfigurationGetHostCpuFeatures(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__APPLE__)
  #if defined(__i386__)
    const char* value = "Intel";
    strcat(str, value);
    return strlen(value);
  #endif
#elif defined(__linux__)
  #if defined(__i386__)
    return boincGetStringFromExec(str, max_size, "grep flags /proc/cpuinfo | head -n 1 | sed 's/.* *: //' ");  
  #elif defined(__powerpc__)
    return strlen(strcpy(str, "not specified"));
  #endif
#endif
}

unsigned int boincConfigurationGetHostCpuFpops(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return strlen(strcpy(str, "10000000"));
}

unsigned int boincConfigurationGetHostCpuIops(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  //  return strlen(strcpy(str, "10000000"));
  return 0;
}

unsigned int boincConfigurationGetHostCpuMemBW(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  //  return strlen(strcpy(str, "10000000"));
  return 0;
}

unsigned int boincConfigurationGetHostCpuCalculated(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return 0;
}

unsigned int boincConfigurationGetHostMemTotal(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__linux__)
  return boincGetStringFromExec(str, max_size, "grep MemTotal: /proc/meminfo | sed 's/.*: *//' | sed 's/ kB/000/'");
#else
  return strlen(strcpy(str, "1073741824"));
#endif
}

unsigned int boincConfigurationGetHostMemBytes(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__linux__)
  return boincGetStringFromExec(str, max_size, "grep MemFree: /proc/meminfo | sed 's/.*: *//' | sed 's/ kB/000/'");
#else
  return strlen(strcpy(str, "1073741824"));
#endif
}

unsigned int boincConfigurationGetHostMemCache(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__linux__)
  return boincGetStringFromExec(str, max_size, "grep Cached: /proc/meminfo | head -n 1 | sed 's/.*: *//' | sed 's/ kB/000/'");
#else
  return strlen(strcpy(str, "1048576"));
#endif
}

unsigned int boincConfigurationGetHostMemSwap(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__linux__)
  return boincGetStringFromExec(str, max_size, "grep SwapTotal: /proc/meminfo | sed 's/.*: *//' | sed 's/ kB/000/'");
#else
  return strlen(strcpy(str, "1073741824"));
#endif
}

unsigned int boincConfigurationGetHostDiskTotal(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__linux__)
  return boincGetStringFromExec(str, max_size, "df -B 1 . | tail -n 1 | awk '{print $2}'");
#else
  return strlen(strcpy(str, "85899345920"));
#endif
}

unsigned int boincConfigurationGetHostDiskFree(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__linux__)
  return boincGetStringFromExec(str, max_size, "df -B 1 . | tail -n 1 | awk '{print $4}'");
#else
  return strlen(strcpy(str, "85899345920"));
#endif
}

unsigned int boincConfigurationGetHostOSVersion(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return boincGetStringFromExec(str, max_size, "uname -r");
}

unsigned int boincConfigurationGetHostAccelerators(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return 0;
}

unsigned int boincConfigurationGetTotalDiskUsage(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
  return boincConfigurationGetProjectDiskUsage(str, max_size, conf);
}

unsigned int boincConfigurationGetProjectDiskUsage(char * str, unsigned int max_size, BoincConfiguration * conf) 
{ 
#if defined(__linux__)
  char dir[255];
  return boincGetStringFromExec(str, max_size, "du -b -s %s 2> /dev/null | sed 's/[^0-9].*//'", 
				boincConfigurationGetString(conf, kBoincProjectDirectory, dir, 255));
#else
  return strlen(strcpy(str, "0"));
#endif
}


BoincError boincConfigurationOSCreateDefault(BoincConfiguration* conf) 
{
  return NULL;
}


