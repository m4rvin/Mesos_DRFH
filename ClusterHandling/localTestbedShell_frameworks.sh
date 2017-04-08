#!/bin/bash

if [ "$#" -lt 4 ]; then
	echo ">>> WARNING: You must specify the duration (in secs) for the frameworks."
	echo ">>> WARNING: You must specify the workload distribution (LogNorm,Exp) for the frameworks'workloads."
	echo ">>> WARNING: You must specify the desired cluster Utilization (cpu bound), e.g. 75,85,...,100."
	echo ">>> WARNING: You must specify the desired configuration of the workload: confA, confB (i.e. DRFbypass, or GoogleTrace-like)."
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



TASK_DURATION=10

FWLOW_TASK_CPU_DEMAND=0.05
FWLOW_TASK_MEM_DEMAND="128MB"

FWCOMMON_TASK_CPU_DEMAND=1
FWCOMMON_TASK_MEM_DEMAND="1024MB"

FWMEMINT_TASK_CPU_DEMAND=1.5
FWMEMINT_TASK_MEM_DEMAND="4096MB"

FWCPUINT_TASK_CPU_DEMAND=4
FWCPUINT_TASK_MEM_DEMAND="256MB"


echo "LAUNCHING FRAMEWORKS"

cd $MESOS_FRAMEWORK_EXECUTABLES_PATH

if [ "$2" == "LogNorm" ]; then #->> start LogNorm section

	if [ "$3" -eq "100" ]; then
	######### LogNorm U=100%
		if [ "$4" == "ConfA" ]; then
			## Conf__DRF-bypass
			fwLow_interarrival_distribution="LogNormal,-5.33993575760899,0.5"
			fwCommon_interarrival_distribution="LogNormal,-2.34420348405499,0.5"
			fwMemInt_interarrival_distribution="LogNormal,-1.93328877117927,0.5"
			fwCpuInt_interarrival_distribution="LogNormal,-0.957909122935104,0.5"
		elif [ "$4" == "ConfB" ]; then
			## Conf_GoogleTrace-like V1
			fwLow_interarrival_distribution="LogNormal,-4.42364502573483,0.5"
			fwCommon_interarrival_distribution="LogNormal,-2.92836038090654,0.5"
			fwMemInt_interarrival_distribution="LogNormal,-1.82974809223843,0.5"
			fwCpuInt_interarrival_distribution="LogNormal,-0.818147180559945,0.5"
		else
			echo "Not recognized Configuration value."
			exit
		fi
	elif [ "$3" -eq "95" ]; then
	######### LogNorm U=95%
		if [ "$4" == "ConfA" ]; then
			## Conf_DRF-bypass
			fwLow_interarrival_distribution="LogNormal,-5.28864246322144,0.5"
			fwCommon_interarrival_distribution="LogNormal,-2.28832302566054,0.5"
			fwMemInt_interarrival_distribution="LogNormal,-1.88285791755237,0.5"
			fwCpuInt_interarrival_distribution="LogNormal,-0.866937344729377,0.5"
		elif [ "$4" == "ConfB" ]; then
			## Conf_GoogleTrace-like V1
			fwLow_interarrival_distribution="LogNormal,-4.37206564923976,0.5"
			fwCommon_interarrival_distribution="LogNormal,-2.87866071235426,0.5"
			fwMemInt_interarrival_distribution="LogNormal,-1.77365862558738,0.5"
			fwCpuInt_interarrival_distribution="LogNormal,-0.766853886172395,0.5"
		else
			echo "Not recognized Configuration value."
			exit
		fi
	elif [ "$3" -eq "85" ]; then
	######### LogNorm U=85%
		if [ "$4" == "ConfA" ]; then
			## Conf_DRF-bypass
			fwLow_interarrival_distribution="LogNormal,-5.17741682811121,0.5"
			fwCommon_interarrival_distribution="LogNormal,-2.17912373369555,0.5"
			fwMemInt_interarrival_distribution="LogNormal,-1.77365862558738,0.5"
			fwCpuInt_interarrival_distribution="LogNormal,-0.766853886172395,0.5"
		else
			echo "Not recognized Configuration value."
			exit
		fi
	else
		echo "Not recognized utilization value."
		exit
	fi
else #--> start custom section
	echo "TODO custom distribution."
	exit
fi
#<-- end custom section

time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=$TASK_DURATION --task_cpus_demand=$FWLOW_TASK_CPU_DEMAND --task_memory_demand=$FWLOW_TASK_MEM_DEMAND --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-1.dat " --generators_seed="framework-low" --interarrivals_distribution=$fwLow_interarrival_distribution &
pid=$!
echo $pid

time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=$TASK_DURATION --task_cpus_demand=$FWCOMMON_TASK_CPU_DEMAND --task_memory_demand=$FWCOMMON_TASK_MEM_DEMAND --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-2.dat " --generators_seed="framework-common" --interarrivals_distribution=$fwCommon_interarrival_distribution &
pid=$!
echo $pid

time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=$TASK_DURATION --task_cpus_demand=$FWMEMINT_TASK_CPU_DEMAND --task_memory_demand=$FWMEMINT_TASK_MEM_DEMAND --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-3.dat " --generators_seed="framework-memint" --interarrivals_distribution=$fwMemInt_interarrival_distribution &
pid=$!
echo $pid

time ./test-framework-drfh --master=127.0.0.1:5050 --task_duration=$TASK_DURATION --task_cpus_demand=$FWCPUINT_TASK_CPU_DEMAND --task_memory_demand=$FWCPUINT_TASK_MEM_DEMAND --duration=$fwDuration --offers_stats_file="$FRAMEWORK_STATS_FOLDER/framework-4.dat " --generators_seed="framework-cpuint" --interarrivals_distribution=$fwCpuInt_interarrival_distribution &
pid=$!
echo $pid

exit
