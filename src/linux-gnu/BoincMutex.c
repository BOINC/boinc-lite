/*

  BoincMutex, a small abstraction library for mutexes.

  Copyright (C) 2008 Sony Computer Science Laboratory Paris 
  Authors: Peter Hanappe
 
*/
#include "BoincMutex.h"
#include "BoincError.h"
#include "BoincMem.h"
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define TRACE_MUTEX_CALLS 0
#if TRACE_MUTEX_CALLS
#include <stdio.h>
#include <execinfo.h>
#endif


typedef struct _BoincMutexImpl {
	char name[16];
	pthread_mutex_t impl;
} BoincMutexImpl;


BoincMutex boincMutexCreate(char* name)
{
	int err;
	pthread_mutexattr_t attr;
	BoincMutexImpl* mutex;

	mutex = boincNew(BoincMutexImpl);
	if (mutex == NULL) {
		boincLogf(kBoincError, -1, "boincMutexCreate: Out of memory");
		return NULL;
	}

	memset(mutex->name, 0, sizeof(mutex->name));
	if (name) {
		strncpy(mutex->name, name, sizeof(mutex->name) - 1);
	}

	pthread_mutexattr_init(&attr);
	err = pthread_mutex_init(&mutex->impl, &attr);
	if (err) {
		boincLogf(kBoincError, -1, "boincMutexCreate: pthread_mutex_init returned error %d", err);
		boincFree(mutex);
		return NULL;
	}

	return mutex;
}

void boincMutexDestroy(BoincMutex mutex)
{
	if (mutex) {
		pthread_mutex_destroy(&mutex->impl);
		boincFree(mutex);
	}
}

int boincMutexLock(BoincMutex mutex)
{
#if TRACE_MUTEX_CALLS
	void *array[5];
	size_t size;
	char **strings;
	size_t i;
	
	size = backtrace(array, 5);
	strings = backtrace_symbols(array, size);
	
	printf("Mutex %s: lock\n", mutex->name);
	for (i = 0; i < size; i++) {
		//printf ("%s\n", strings[i]);
	}
	
	free (strings);
#endif

	return pthread_mutex_lock(&mutex->impl);
}

int boincMutexUnlock(BoincMutex mutex)
{
#if TRACE_MUTEX_CALLS
	void *array[5];
	size_t size;
	char **strings;
	size_t i;
	
	size = backtrace(array, 5);
	strings = backtrace_symbols(array, size);
	
	printf("Mutex %s: unlock\n", mutex->name);
	for (i = 0; i < size; i++) {
		//printf ("%s\n", strings[i]);
	}
	
	free (strings);
#endif

	return pthread_mutex_unlock(&mutex->impl);
}

void boincCatchSigTerm(boincTermHandler th)
{
  signal(SIGINT, th);
  signal(SIGTERM, th);
  signal(SIGQUIT, th);
}
