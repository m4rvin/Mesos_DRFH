#!/bin/bash

#This script uses tmux to create the working dashboard

MESOS_MAIN_DIR="/home/m4rvin/mesos_drfh/"
MESOS_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/bin"
CLUSTER_NODES_PIDS_FILENAME="clusterNodesPids"
CLUSTER_NODES_PIDS_FILEPATH="$MESOS_MAIN_DIR/ClusterHandling/$CLUSTER_NODES_PIDS_FILENAME"


#######################
echo "LAUNCHING MASTER"

cd $MESOS_EXECUTABLES_PATH

GLOG_v=1 ./mesos-master.sh --work_dir=/tmp/mesos --ip=127.0.0.1 --advertise_ip=127.0.0.1 --quorum=1 --log_dir=/tmp/mesosLog &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"
