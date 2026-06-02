#!/bin/bash

get_process_load() {
	awk '{print "CPU Load (1 min): " $1}' /proc/loadavg
}

((old_total=0))
((old_idle=0))

get_cpu_load() {
	read -r total idle <<< $(cat /proc/stat | head -n 1 | awk '{print $2 + $3 + $4 + $5 + $6 + $7 + $8 + $9 + $10 + $11, $5}')
	local cpu_used_perc=$(echo "scale=2; 100 * ((${total}-${old_total})-(${idle}-${old_idle})) / (${total}-${old_total})" | bc)

	((old_total=total))
	((old_idle=idle))

	LANG=C printf "CPU utilization: %.2f%%\n" ${cpu_used_perc}
}

get_memory_usage(){
	mem_total_kb=$(grep -i 'MemTotal: ' /proc/meminfo | tr -s ' ' | cut -d' ' -f2)
	mem_free_kb=$(grep -i 'MemFree: ' /proc/meminfo | tr -s ' ' | cut -d' ' -f2)
	mem_available_kb=$(grep -i 'MemAvailable: ' /proc/meminfo | tr -s ' ' | cut -d' ' -f2)

	mem_used_perc_f=$(echo "scale=2; 100 * ((${mem_total_kb} - ${mem_free_kb}) / ${mem_total_kb})" | bc)
	mem_used_perc_a=$(echo "scale=2; 100 * ((${mem_total_kb} - ${mem_available_kb}) / ${mem_total_kb})" | bc)

	mem_total_gb=$(echo "scale=2; ${mem_total_kb} / 1024 / 1024" | bc)
	mem_used_gb_f=$(echo "scale=2; (${mem_total_kb} - ${mem_free_kb}) / 1024 / 1024" | bc)
	mem_used_gb_a=$(echo "scale=2; (${mem_total_kb} - ${mem_available_kb}) / 1024 / 1024" | bc)

	echo -e "\nMemUsed (using MemFree): ${mem_used_gb_f} GB, MemUsed (using MemAvailable): ${mem_used_gb_a} GB, MemTotal: ${mem_total_gb} GB"
	echo "MemUsed_perc (using MemFree): ${mem_used_perc_f}%, MemUsed_perc (using MemAvailable): ${mem_used_perc_a}%"
}
