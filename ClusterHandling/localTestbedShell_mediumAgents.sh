#!/bin/bash

#This script uses tmux to create the working dashboard

MESOS_MAIN_DIR="/home/m4rvin/mesos_drfh/"
MESOS_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/bin"
CLUSTER_NODES_PIDS_FILENAME="clusterNodesPids"
CLUSTER_NODES_PIDS_FILEPATH="$MESOS_MAIN_DIR/ClusterHandling/$CLUSTER_NODES_PIDS_FILENAME"

##############################
echo "LAUNCHING MEDIUM AGENTS"

cd $MESOS_EXECUTABLES_PATH

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent4 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent4 --resources="cpus:64;mem:262144" --advertise_port=5054 --port=5054 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent5 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent5 --resources="cpus:64;mem:262144" --advertise_port=5055 --port=5055 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent6 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent6 --resources="cpus:64;mem:262144" --advertise_port=5056 --port=5056 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent7 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent7 --resources="cpus:64;mem:262144" --advertise_port=5057 --port=5057 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"

./mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent8 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent8 --resources="cpus:64;mem:262144" --advertise_port=5058 --port=5058 &
pid=$!
echo $pid
echo $pid >> "$CLUSTER_NODES_PIDS_FILEPATH"
