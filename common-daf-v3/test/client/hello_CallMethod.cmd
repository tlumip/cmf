@echo off
rem
rem --- Calls a method on the SampleHandler in node0 ---
rem
rem Environment variable REPOSITORY_DIR must be set
rem

set cp=.;%REPOSITORY_DIR%\cmf\common-base\build\classes
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\build\classes
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\commons-codec-1.3.jar
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\xmlrpc-2.0.jar
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\lib\groovy-all-1.0-jsr-03.jar
set cp=%cp%;%REPOSITORY_DIR%\third-party\logging-log4j-1.2.9\log4j-1.2.9.jar
set cp=%cp%;%REPOSITORY_DIR%\cmf\common-daf-v3\test

set classpath=%cp%

rem RpcClient <URL> <Method>

java com.pb.common.rpc.RpcClient tcp://192.168.2.201:6001 test.sayHello