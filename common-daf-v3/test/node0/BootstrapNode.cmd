@echo off
rem
rem --- Starts a Bootstrap node ---
rem
rem Environment variable REPOSITORY_DIR must be set
rem

set REPOSITORY_DIR=C:\myfiles\subversion

set classpath=.;%REPOSITORY_DIR%\cmf\common-base\build\classes
set classpath=%classpath%;%REPOSITORY_DIR%\cmf\common-daf-v3\build\classes
set classpath=%classpath%;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\commons-codec-1.3.jar
set classpath=%classpath%;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\xmlrpc-2.0.jar
set classpath=%classpath%;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\groovy-all-1.0-jsr-03.jar
set classpath=%classpath%;%REPOSITORY_DIR%\third-party\logging-log4j-1.2.9\log4j-1.2.9.jar
set classpath=%classpath%;%REPOSITORY_DIR%\cmf\common-daf-v3\test

java com.pb.common.rpc.DafNode -bootstrap -command startnode0.txt