#!/bin/bash

#This script uses tmux to create the working dashboard

MESOS_MAIN_DIR="/home/m4rvin/mesos_drfh/"
MESOS_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/bin"
MESOS_FRAMEWORK_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/src"
CLUSTER_NODES_PIDS_FILENAME="clusterNodesPids"
CLUSTER_NODES_PIDS_FILEPATH="$MESOS_MAIN_DIR/ClusterHandling/$CLUSTER_NODES_PIDS_FILENAME"
FRAMEWORK_STATS_FOLDER="$MESOS_MAIN_DIR/ClusterHandling"


###########################
echo "LAUNCHING FRAMEWORKS"

cd $MESOS_FRAMEWORK_EXECUTABLES_PATH

./test-framework-drfh --master=127.0.0.1:5050 --framework_type=low --task_cpus_demand=0.1 --task_memory_demand=128MB --offers_limit=200 --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-1.txt " &
pid=$!
echo $pid
#echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"
