/*
  BoincLite, a light-weight library for BOINC clients.

  Copyright (C) 2008 Sony Computer Science Laboratory Paris 
  Authors: Peter Hanappe, Anthony Beurive, Tangui Morlier

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/ 
#include "BoincLite.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define PROJECT_DIR "/tmp/boincLiteProject"
#define UPLOAD_MSG "Cette application me retourne des informations de qualité !! @àéù\n"

int compute(BoincScheduler * scheduler, BoincWorkUnit * wu, void* userData) 
{
        sleep(1);

        BoincFileInfo * fi;
        for (fi = wu->result->fileInfo ; fi ; fi = fi->next) {
                char uploadFile[255];
                boincWorkUnitGetPath(wu, fi, uploadFile, 255);
    
                FILE * file = fopen(uploadFile, "w+");
                fprintf(file, UPLOAD_MSG);
                fclose(file);
        }

        boincErrorDestroy(boincSchedulerChangeWorkUnitStatus(scheduler, wu, kBoincWorkUnitFinished));

        return 1;
}

int main(int argc, char *argv[])
{
        BoincLogger log = { boincDefaultLogFunction, (void*) kBoincInfo};
        if (argc > 1) {
                log.userData = (void*)atoi(argv[1]);
        }

        boincLogInit(&log);

        BoincError err ;
        mkdir(PROJECT_DIR, S_IRWXU);

        BoincConfiguration * conf = boincConfigurationCreate(PROJECT_DIR);
        err = boincConfigurationSetString(conf, kBoincProjectURL, "http://matisse.cslparis.lan/FamousAtCSL/");
        if (!err)
                err = boincConfigurationSetString(conf, kBoincUserEmail, "tanguim@gmail.com");
        if (!err)
                err = boincConfigurationSetString(conf, kBoincUserPassword, "b781076bb15ca436151e8f612e8f9003");
        if (!err)
                err = boincConfigurationLoad(conf);

        if (err) {
                boincLogError(err);
                boincConfigurationDestroy(conf);
                boincErrorDestroy(err);
                exit(1);
        }

        BoincScheduler * scheduler = boincSchedulerCreate(conf, compute, NULL);
  
        while(1) {

                if (boincSchedulerHasEvents(scheduler)) {
                        err = boincSchedulerHandleEvents(scheduler);
                        if (err) {
                                boincLogError(err);
                                boincErrorDestroy(err);
                        }
                } else {
                        sleep(1);
                }
        }
  
        boincSchedulerDestroy(scheduler);

        return 0;
}
