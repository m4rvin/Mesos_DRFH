#!/bin/bash

#This script uses tmux to create the working dashboard

MESOS_EXECUTABLES_PATH="/home/m4rvin/mesos_drfh/build/bin"
MESOS_FRAMEWORK_EXECUTABLES_PATH="/home/m4rvin/mesos_drfh/build/src"

cd $MESOS_EXECUTABLES_PATH

#create tmux windows and submit commands inside the window's panes (NB: double Enter is for entering into the docker-container's session)
tmux new-window -P -n MASTER

tmux send-keys -t MASTER 'GLOG_v=1 ./mesos-master.sh --work_dir=/tmp/mesos --ip=127.0.0.1 --advertise_ip=127.0.0.1 --quorum=1 --log_dir=/tmp/mesosLog ' Enter

tmux new-window -P -n SMALL_AGENTS
tmux select-window -t SMALL_AGENTS
tmux split-window -t SMALL_AGENTS
tmux split-window -t SMALL_AGENTS
tmux select-layout even-vertical

tmux send-keys -t 0 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent1 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:16;mem:65536" --advertise_port=5051 --port=5051' Enter

tmux send-keys -t 1 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent2 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:16;mem:65536" --advertise_port=5052 --port=5052' Enter

tmux send-keys -t 2 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent3 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:16;mem:65536" --advertise_port=5053 --port=5053' Enter

tmux new-window -P -n MEDIUM_AGENTS
tmux select-window -t MEDIUM_AGENTS
tmux split-window -t MEDIUM_AGENTS
tmux split-window -t MEDIUM_AGENTS
tmux split-window -t MEDIUM_AGENTS
tmux split-window -t MEDIUM_AGENTS
#tmux split-window -t MEDIUM_AGENTS
#tmux send-keys -t 4 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent8 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:64;mem:262144" --advertise_port=5058 --port=5058' Enter

tmux select-layout even-vertical

tmux send-keys -t 0 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent4 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:64;mem:262144" --advertise_port=5054 --port=5054' Enter

tmux send-keys -t 1 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent5 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:64;mem:262144" --advertise_port=5055 --port=5055' Enter

tmux send-keys -t 2 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent6 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:64;mem:262144" --advertise_port=5056 --port=5056' Enter

tmux send-keys -t 3 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent7 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:64;mem:262144" --advertise_port=5057 --port=5057' Enter


tmux new-window -P -n BIG_AGENTS
tmux select-window -t BIG_AGENTS
tmux split-window -t BIG_AGENTS
tmux select-layout even-vertical

tmux send-keys -t 0 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent9 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:64;mem:999424" --advertise_port=5059 --port=5059' Enter

tmux send-keys -t 1 './mesos-agent.sh --master=127.0.0.1:5050 --work_dir=/tmp/mesos/agent10 --no-systemd_enable_support --log_dir=/tmp/mesosLog/agent1 --resources="cpus:64;mem:999424" --advertise_port=5060 --port=5060' Enter


cd $MESOS_FRAMEWORK_EXECUTABLES_PATH

tmux new-window -P -n FRAMEWORKS
tmux select-window -t FRAMEWORKS
#tmux split-window -t FRAMEWORKS
#tmux select-layout even-vertical

tmux send-keys -t 0 './test-framework-drfh --master=127.0.0.1:5050 --task_cpus_demand=1.1 --task_memory_demand=64MB --offers_limit=200'

#tmux send-keys -t 1 'docker start ' $memosSlave_ContainerName Enter
