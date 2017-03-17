#!/bin/bash

if [ "$#" -lt 3 ]; then
	echo ">>> WARNING: You must specify the duration (in secs) for the frameworks."
	echo ">>> WARNING: You must specify the workload distribution (LogNorm,Exp) for the frameworks'workloads."
	echo ">>> WARNING: You must specify the desired cluster Utilization (cpu bound), e.g. 75,85,...,100."
	exit
fi

fwDuration=$1
wloadDistr=$2
clusterU=$3

echo "framework duration = $fwDuration secs"
echo "workload distribution = $wloadDistr"
echo "cluster utilization = $clusterU"

MESOS_MAIN_DIR="/home/m4rvin/mesos_drfh/"
MESOS_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/bin"
MESOS_FRAMEWORK_EXECUTABLES_PATH="$MESOS_MAIN_DIR/build/src"
CLUSTER_NODES_PIDS_FILENAME="clusterNodesPids"
CLUSTER_NODES_PIDS_FILEPATH="$MESOS_MAIN_DIR/ClusterHandling/$CLUSTER_NODES_PIDS_FILENAME"
FRAMEWORK_STATS_FOLDER="$MESOS_MAIN_DIR/ClusterHandling/OUTPUT"


###########################
echo "LAUNCHING FRAMEWORKS"

cd $MESOS_FRAMEWORK_EXECUTABLES_PATH

if [ "$2" == "LogNorm" ]; then #->> start LogNorm section

	if [ "$3" -eq "85" ]; then
		######### LogNorm U=85%

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=0.1 --task_memory_demand=128MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-1.dat " --generators_seed="framework-low" --interarrivals_distribution="LogNormal,-4.82001088998788,0.5" &
		pid=$!
		echo $pid

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=1 --task_memory_demand=1024MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-2.dat " --generators_seed="framework-common" --interarrivals_distribution="LogNormal,-2.5137627892351,0.5" &
		pid=$!
		echo $pid

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=1 --task_memory_demand=4096MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-3.dat " --generators_seed="framework-memint" --interarrivals_distribution="LogNormal,-0.0196394843421737,0.5" &
		pid=$!
		echo $pid

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=4 --task_memory_demand=256MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-4.dat " --generators_seed="framework-cpuint" --interarrivals_distribution="LogNormal,-0.866937344729377,0.5" &
		pid=$!
		echo $pid
	elif [ "$3" -eq "75" ]; then
		########## LogNorm U=75%

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=0.1 --task_memory_demand=128MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-1.dat " --generators_seed="framework-low" --interarrivals_distribution="LogNormal,-4.69557874121847,0.5" &
		pid=$!
		echo $pid

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=1 --task_memory_demand=1024MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-2.dat " --generators_seed="framework-common" --interarrivals_distribution="LogNormal,-2.386763098,0.5" &
		pid=$!
		echo $pid

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=1 --task_memory_demand=4096MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-3.dat " --generators_seed="framework-memint" --interarrivals_distribution="LogNormal,0.09814355131,0.5" &
		pid=$!
		echo $pid

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=4 --task_memory_demand=256MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-4.dat " --generators_seed="framework-cpuint" --interarrivals_distribution="LogNormal,-0.7127866649,0.5" &
		pid=$!
		echo $pid
	else
		echo "Not recognized utilization value."
		exit
	fi
#<-- end LogNorm section

elif [ "$2" == "Exp" ]; then #--> start Exp section

	if [ "$3" -eq "85" ]; then
		########## EXP U=85%

		echo "TODO 85% utilization with exp distribution."
		exit
	elif [ "$3" -eq "75" ]; then
		########## EXP U=75%

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=0.1 --task_memory_demand=128MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-1.dat " --generators_seed="framework-low" --interarrivals_distribution="Exp,96.6" &
		pid=$!
		echo $pid

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=1 --task_memory_demand=1024MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-2.dat " --generators_seed="framework-common" --interarrivals_distribution="Exp,9.6" &
		pid=$!
		echo $pid

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=1 --task_memory_demand=4096MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-3.dat " --generators_seed="framework-memint" --interarrivals_distribution="Exp,0.8" &
		pid=$!
		echo $pid

		time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=10 --task_cpus_demand=4 --task_memory_demand=256MB --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-4.dat " --generators_seed="framework-cpuint" --interarrivals_distribution="Exp,1.8" &
		pid=$!
		echo $pid

	else
		echo "Not recognized utilization value."
		exit
	fi
#<-- end LogNorm section

else #--> start custom section
	echo "TODO custom distribution."
	exit
fi
#<-- end custom section
