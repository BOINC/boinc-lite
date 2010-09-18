#ifndef __BoincTestLinux_H__
#define __BoincTestLinux_H__

#define TMPFILE "/tmp/boinclite.tmp"
#define TMPDIR "/tmp/"
#define TESTDIR "/home/tangui/"
#define PROJECT_SCHEDULER_TEST_DIR "/tmp/boincTester"

int grep(char * file, char * motif);

int zgrep(char * file, char * motif);

int rmrf(const char * dir);

#endif // __BoincTestLinux_H__
