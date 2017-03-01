#!/bin/bash

#This script uses tmux to create the working dashboard

MESOS_MAIN_DIR="/home/m4rvin/mesos_drfh/"
MESOS_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/bin"
MESOS_FRAMEWORK_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/src"
CLUSTER_NODES_PIDS_FILENAME="clusterNodesPids"
CLUSTER_NODES_PIDS_FILEPATH="$MESOS_MAIN_DIR/$CLUSTER_NODES_PIDS_FILENAME"


###########################
echo "LAUNCHING FRAMEWORKS"

cd $MESOS_FRAMEWORK_EXECUTABLES_PATH

./test-framework-drfh --master=127.0.0.1:5050 --task_cpus_demand=1.1 --task_memory_demand=64MB --offers_limit=200 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"
