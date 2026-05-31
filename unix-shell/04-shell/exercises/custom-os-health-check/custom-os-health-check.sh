#!/bin/bash

source os-health-lib.sh

while true; do
	get_process_load
	get_cpu_load
	get_memory_usage
	echo ""
	sleep 1
done
