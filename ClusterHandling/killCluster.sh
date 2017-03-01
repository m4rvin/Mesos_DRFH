#!/bin/bash

CLUSTER_NODES_PIDS_FILENAME="clusterNodesPids"


while IFS='' read -r line; do
	echo "Text read from file: $line"
done < "$CLUSTER_NODES_PIDS_FILENAME"

while IFS='' read -r line; do
	echo "Killing process with pid $line"
	kill -9  $line
	if [ $? -eq 0 ]
	then
		echo "SUCCESS: process with pid $line killed."
	else
		echo "ERROR: process with pid $line NOT killed."
	fi
done < "$CLUSTER_NODES_PIDS_FILENAME"

echo "Removing pids file."
rm $CLUSTER_NODES_PIDS_FILENAME
if [ $? -eq 0 ]
	then
		echo "SUCCESS."
	else
		echo "ERROR."
fi
