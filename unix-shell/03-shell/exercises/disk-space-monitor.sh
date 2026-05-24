#!/bin/bash

# Set variables
TRESHOLD=10
PARTITION="/home"

# Check the available disk space on the partition
HOME_DISK_SPACE=$(( 100 - $(df "${PARTITION}" | tail -n 1 | tr -s ' ' | cut -d' ' -f5 | tr -d '%') ))

if [ ${HOME_DISK_SPACE} -lt ${TRESHOLD} ]; then
	echo "Warning: the available disk space on ${PARTITION} falls below ${TRESHOLD}%" >&2
else
	echo "SUCCESS: Sufficient disk space on ${PARTITION}"
	exit 0
fi
