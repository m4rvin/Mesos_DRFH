#!/bin/bash

#This script uses tmux to create the working dashboard

MESOS_MAIN_DIR="/home/m4rvin/mesos_drfh/"
MESOS_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/bin"
CLUSTER_NODES_PIDS_FILENAME="clusterNodesPids"
CLUSTER_NODES_PIDS_FILEPATH="$MESOS_MAIN_DIR/$CLUSTER_NODES_PIDS_FILENAME"

#############################
echo "LAUNCHING SMALL AGENTS"

cd $MESOS_EXECUTABLES_PATH

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent1 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:16;mem:65536" --advertise_port=5051 --port=5051 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent2 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent2 --resources="cpus:16;mem:65536" --advertise_port=5052 --port=5052 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent3 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent3 --resources="cpus:16;mem:65536" --advertise_port=5053 --port=5053 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"
