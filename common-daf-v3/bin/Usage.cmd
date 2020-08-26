@echo off
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

java com.pb.common.rpc.DafNode