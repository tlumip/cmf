#! /bin/bash

#
# --- Starts a DAF node ---
#
# usage: java  DafNode  <node name>  <groovy configuration file>
#
# Environment variable REPOSITORY_DIR must be set
#

REPOSITORY_DIR=/mnt/svn
APP_DIR=projects/tlumip
MODULE_HANDLERS=config/ts/tsHandlers.groovy


CLASSPATH=.:$REPOSITORY_DIR/cmf/common-base/build/classes
CLASSPATH=$CLASSPATH:$REPOSITORY_DIR/cmf/common-daf-v3/build/classes
CLASSPATH=$CLASSPATH:$REPOSITORY_DIR/projects/tlumip/build/classes
CLASSPATH=$CLASSPATH:$REPOSITORY_DIR/projects/tlumip/config
CLASSPATH=$CLASSPATH:$REPOSITORY_DIR/projects/tlumip/config/ts
CLASSPATH=$CLASSPATH:$REPOSITORY_DIR/cmf/common-daf-v3/lib/commons-codec-1.3.jar
CLASSPATH=$CLASSPATH:$REPOSITORY_DIR/cmf/common-daf-v3/lib/xmlrpc-2.0.jar
CLASSPATH=$CLASSPATH:$REPOSITORY_DIR/cmf/common-daf-v3/lib/groovy-all-1.0-jsr-03.jar
CLASSPATH=$CLASSPATH:$REPOSITORY_DIR/third-party/logging-log4j-1.2.9/log4j-1.2.9.jar

export REPOSITORY_DIR APP_DIR CLASSPATH

java -Xmx1500m -Dlog4j.configuration=ts_log4j.xml com.pb.common.rpc.DafNode isis $REPOSITORY_DIR/$APP_DIR/$MODULE_HANDLERS
