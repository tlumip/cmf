@echo off
rem
rem --- Starts a DAF node ---
rem
rem Environment variable REPOSITORY_DIR must be set
rem

set cp=.;%REPOSITORY_DIR%\cmf\common-base\build\classes
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\build\classes
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\commons-codec-1.3.jar
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\xmlrpc-2.0.jar
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\groovy-all-1.0-jsr-03.jar
set cp=%cp%;%REPOSITORY_DIR%\third-party\logging-log4j-1.2.9\log4j-1.2.9.jar
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\config

set classpath=%cp%

rem --- Alternate way to start node
rem java -cp %REPOSITORY_DIR%\cmf\build\cmf-20060210.jar;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\xmlrpc-2.0.jar;%REPOSITORY_DIR%\cmf\common-daf-v3\config com.pb.common.rpc.DafNode -node node0 -config ..\config\testapp.groovy

java com.pb.common.rpc.DafNode -node node0 -config ../config/testapp.groovy