#!/bin/bash

#This script uses tmux to create the working dashboard

MESOS_EXECUTABLES_PATH="/home/m4rvin/mesos_drfh/build/bin"
MESOS_FRAMEWORK_EXECUTABLES_PATH="/home/m4rvin/mesos_drfh/build/src"

cd $MESOS_EXECUTABLES_PATH

#create tmux windows and submit commands inside the window's panes (NB: double Enter is for entering into the docker-container's session)
tmux select-window -t MASTER
tmux send-keys C-c Enter

tmux select-window -t SMALL_AGENTS
tmux send-keys -t 0 C-c Enter
tmux send-keys -t 1 C-c Enter
tmux send-keys -t 2 C-c Enter


tmux select-window -t MEDIUM_AGENTS
tmux send-keys -t 0 C-c Enter
tmux send-keys -t 1 C-c Enter
tmux send-keys -t 2 C-c Enter
tmux send-keys -t 3 C-c Enter
#tmux send-keys -t 4 C-c Enter


tmux select-window -t BIG_AGENTS
tmux send-keys -t 0 C-c Enter
tmux send-keys -t 1 C-c Enter


tmux select-window -t FRAMEWORKS
tmux send-keys -t 0 C-c Enter
