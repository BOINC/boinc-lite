/*
  BoincLite, a light-weight library for BOINC clients.

  Copyright (C) 2008 Sony Computer Science Laboratory Paris 
  Authors: Peter Hanappe, Tangui Morlier, Anthony Beurive

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
#include "BoincError.h"
#include "BoincMem.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

typedef struct _BoincError
{
        int type;
        int code;
        char* message;
        int delay;
} BoincErrorStruct;

static BoincErrorStruct _outOfMemoryError = { kBoincFatal, -1, "Out of memory" };

void boincDefaultLogFunction(BoincLogger* logger, int type, int code, const char* message)
{
        int logLevel = (int) logger->userData;
        if (type >= logLevel) {
                switch (type) {
                case kBoincInfo: 
                        fprintf(stderr, "Info");
                        break;
                case kBoincError: 
                        fprintf(stderr, "Error");
                        break;
                case kBoincFatal: 
                        fprintf(stderr, "Fatal");
                        break;
                case kBoincDebug: 
                default:
                        fprintf(stderr, "Debug");
                        break;
                }
                fprintf(stderr, ": %s (code %d)\n", message, code);
        }
}


BoincError boincErrorCreateWithDelay(int code, const char* message, int delay) 
{
        BoincError err = boincErrorCreate(kBoincDelay, code, message);
        err->delay = delay;
        return err;
}

BoincError boincErrorCreate(int type, int code, const char* message) 
{
        BoincError err = boincNew(BoincErrorStruct);
        if (err == NULL) {
                return &_outOfMemoryError;
        }
        err->type = type;
        err->code = code;
        err->message = NULL;
        err->delay = 0;
        if (message != NULL) {
                err->message = boincArray(char, strlen(message) + 1);
                if (err->message == NULL) {
                        boincErrorDestroy(err);
                        return &_outOfMemoryError;
                }
                strcpy(err->message, message);
        }  
        return err;
}

BoincError boincErrorCreatef(int type, int code, const char* format, ...)
{
        char buff[255];
        va_list arglist;
        va_start(arglist,format);
        vsnprintf(buff, 255, format, arglist);
        va_end(arglist);
        return boincErrorCreate(type, code, buff);
}

void boincErrorDestroy(BoincError error) 
{
        if ((error == NULL) || (error == &_outOfMemoryError)) {
                return;
        }
        if (error->message != NULL) {
                boincFree(error->message);
                error->message = NULL;
        }
        boincFree(error);
        error = NULL;
}

int boincErrorOccured(BoincError error)
{
        int r = 0;
        if (error) {
                boincLogError(error);
                r = error->code? error->code : 1; // make sure we don't return false
                boincErrorDestroy(error);
        }
        return r;
}

int boincErrorIsMajor(BoincError error) 
{
        return (error->code < 0);
}

int boincErrorCode(BoincError error) 
{
        return (error != NULL)? error->code : 0;
}

int boincErrorType(BoincError error) 
{
        return (error != NULL)? error->type : 0;
}

char* boincErrorMessage(BoincError error)
{
        return (error != NULL)? error->message : NULL;
}

int boincErrorDelay(BoincError error)
{
        return (error != NULL)? error->delay : 0;
}


static BoincLogger * logger = NULL;

BoincError boincLogInit(BoincLogger * myLogger) 
{
        logger = myLogger;
        return NULL;
}

void boincLogError(BoincError err) 
{
        boincLog(err->type, err->code, err->message);
}

void boincLog(int type, int code, const char* message) 
{
        if ((logger != NULL) && (logger->write != NULL)) {
                return logger->write(logger, type, code, message);
        }
}

void boincLogf(int type, int code, const char* format, ...)
{
        char buff[255];
        va_list arglist;
        va_start(arglist,format);
        vsnprintf(buff, 255, format, arglist);
        va_end(arglist);
        buff[254] = 0;
        return boincLog(type, code, buff);
}

typedef struct _QueueEntry QueueEntry;

struct _QueueEntry
{
        unsigned int time;
        void* data;
        QueueEntry* next;
};

typedef struct _BoincQueue BoincQueue;

struct _BoincQueue
{
        QueueEntry * first;
};

typedef struct _BoincEvent {
        int eventType;
        int index;
        unsigned int nbtries;
} BoincEvent;

struct _BoincScheduler {
        BoincConfiguration * configuration;
        void * proxy;
        int nbworkunits;
        BoincWorkUnit ** workunits;
        void * callback;
        void * userData;
        BoincQueue * queue;
};

void boincLogSchedulerStates(BoincScheduler * sched) 
{
        if ((logger == NULL) || (logger->write == NULL))
                return ;

        boincDebug("================================================");
        int i ;
        if (sched && sched->workunits[0]) {
                for(i = 0 ; i < sched->nbworkunits ; i++ ) {
                        if (sched->workunits[i])
                                boincDebugf("\t%d\t%d", i, sched->workunits[i]->status);
                }
        }else{
                boincDebug("No workunit defined");
        }
        boincDebug("================================================");
        QueueEntry * qe;
        for (qe = sched->queue->first ; qe != NULL ; qe = qe->next) {
                BoincEvent * be = (BoincEvent*) qe->data;
                boincDebugf("%d, type %d, index %d", qe->time, be->eventType, be->index);
        }
        boincDebug("================================================");
        //  sleep(1);
}
