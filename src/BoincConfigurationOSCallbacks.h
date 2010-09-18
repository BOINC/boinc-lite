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

#ifndef __BoincConfigurationOSCallbacks_H__
#define __BoincConfigurationOSCallbacks_H__

#include "BoincLite.h"

unsigned int boincConfigurationGetOSName(char * str, unsigned int max_size, BoincConfiguration * configuration);
  
unsigned int boincConfigurationGetNCpu(char * str, unsigned int max_size, BoincConfiguration * configuration);

unsigned int boincConfigurationGetPlatformName(char * str, unsigned int max_size, BoincConfiguration * configuration);

unsigned int boincConfigurationGetNetworkUpBandwidth(char * str, unsigned int max_size, BoincConfiguration * configuration);

unsigned int boincConfigurationGetNetworkUpAverage(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetNetworkUpAvgTime(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetNetworkDownBandwidth(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetNetworkDownAverage(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetNetworkDownAvgTime(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostTimezone(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostDomainName(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostIpAddress(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostCPID(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostNCpu(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostCpuVendor(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostCpuModel(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostCpuFeatures(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostCpuFpops(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostCpuIops(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostCpuMemBW(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostCpuCalculated(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostMemTotal(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostMemBytes(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostMemCache(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostMemSwap(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostDiskTotal(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostDiskFree(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostOSName(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostOSVersion(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetHostAccelerators(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetTotalDiskUsage(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

unsigned int boincConfigurationGetProjectDiskUsage(char * str, unsigned int max_size, BoincConfiguration * configuration) ;

BoincError boincConfigurationOSCreateDefault(BoincConfiguration* conf);

#endif // __BoincConfiguration_H__
