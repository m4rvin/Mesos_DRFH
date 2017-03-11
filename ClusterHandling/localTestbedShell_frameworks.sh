#!/bin/bash

#This script uses tmux to create the working dashboard

MESOS_MAIN_DIR="/home/m4rvin/mesos_drfh/"
MESOS_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/bin"
MESOS_FRAMEWORK_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/src"
CLUSTER_NODES_PIDS_FILENAME="clusterNodesPids"
CLUSTER_NODES_PIDS_FILEPATH="$MESOS_MAIN_DIR/ClusterHandling/$CLUSTER_NODES_PIDS_FILENAME"
FRAMEWORK_STATS_FOLDER="$MESOS_MAIN_DIR/ClusterHandling/OUTPUT"


###########################
echo "LAUNCHING FRAMEWORKS"

cd $MESOS_FRAMEWORK_EXECUTABLES_PATH

time ./test-framework-drfh --master=127.0.0.1:5050 --framework_type=low --task_cpus_demand=1 --task_memory_demand=128MB --duration=60 --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-1.dat " --generators_seed="framework1" --interarrivals_distribution="E,1.8" &
pid=$!
echo $pid

time ./test-framework-drfh --master=127.0.0.1:5050 --framework_type=low --task_cpus_demand=4 --task_memory_demand=128MB --duration=60 --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-2.dat " --generators_seed="framework2" --interarrivals_distribution="E,0.4" &
pid=$!
echo $pid


#echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"
