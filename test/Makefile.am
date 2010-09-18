noinst_PROGRAMS = BoincLiteTests
BoincLiteTests_SOURCES = BoincLite.h BoincLiteTests.c ../src/@TARGET@/BoincHttp.c BoincTest.h BoincTest.c BoincHttpTest.c BoincHttpTest.h BoincConfigurationTest.c BoincConfigurationTest.h ../src/BoincConfiguration.c ../src/@TARGET@/BoincConfigurationOSCallbacks.c BoincHttp.h BoincConfiguration.h ../src/BoincError.c BoincError.h ../src/BoincProxy.c BoincProxy.h BoincProxyTest.h BoincProxyTest.c @TARGET@/BoincTestOSCalls.c @TARGET@/BoincTestOSCalls.h ../src/@TARGET@/BoincXMLParser.c BoincXMLParser.h BoincHash.h BoincHashTest.h BoincHashTest.c ../src/@TARGET@/BoincHash.c ../src/BoincScheduler.c BoincScheduler.h BoincSchedulerTest.h BoincSchedulerTest.c BoincUtil.h ../src/BoincUtil.c ../src/@TARGET@/BoincMutex.c BoincMutex.h ../src/BoincWorkUnit.c

AM_CPPFLAGS = -I../src -I../include -I@TARGET@
AM_CFLAGS = -std=c99

test: BoincLiteTests
	./BoincLiteTests

testfirst : BoincLiteTests
	./BoincLiteTests 999
