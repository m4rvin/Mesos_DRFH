#!/bin/bash

#This script uses tmux to create the working dashboard

MESOS_MAIN_DIR="/home/m4rvin/mesos_drfh/"
MESOS_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/bin"
CLUSTER_NODES_PIDS_FILENAME="clusterNodesPids"
CLUSTER_NODES_PIDS_FILEPATH="$MESOS_MAIN_DIR/$CLUSTER_NODES_PIDS_FILENAME"

###########################
echo "LAUNCHING BIG AGENTS"

cd $MESOS_EXECUTABLES_PATH

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent9 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent9 --resources="cpus:64;mem:999424" --advertise_port=5059 --port=5059 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent10 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent10 --resources="cpus:64;mem:999424" --advertise_port=5060 --port=5060 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"
