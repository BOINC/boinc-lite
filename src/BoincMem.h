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
#ifndef __BoincMem_H__
#define __BoincMem_H__

#include "config.h"

#if !WITH_GC
#include <stdlib.h>
#define boincNew(_type)          (_type*) calloc(1, sizeof(_type))
#define boincArray(_type,_len)   (_type*) calloc(_len, sizeof(_type))
#define boincFree(_ptr)          free((void*)_ptr)

// The following functions are only really needed for applications
// that want to use the memory leak detector / garbage collector. 
// See below.
#define boincMemInit()           
#define boincMemLeaks()
#define boincMemCollect()
#define boincMemCleanup()        


#else
// Special memory management functions used for debugging in
// conjunction with SGC (Sony CSL Paris' Garbage Collector):
// http://ikoru.svn.sourceforge.net/viewvc/ikoru/libcsl/sgc/
#include <sgc.h>
#define boincNew(_type)          (_type*) sgc_new_object(sizeof(_type), SGC_ZERO, 0)
#define boincArray(_type,_len)   (_type*) sgc_new_object(_len * sizeof(_type), SGC_ZERO, 0)
#define boincFree(_ptr)          sgc_free_object((void*)_ptr)

#define boincMemInit()           sgc_init(0, &argc, 0, 0)
#define boincMemLeaks()          sgc_search_memory_leaks(NULL)
#define boincMemCollect()        sgc_run()
#define boincMemCleanup()        sgc_cleanup()
#endif

#endif /* __BoincMem_H__ */
