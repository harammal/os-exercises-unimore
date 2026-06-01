#!/bin/bash

# get the log file
file="$1"

# correct usage
extension="${file##*.}"

if [ $# -ne 1 -o $extension != 'log' ]; then
	echo "Usage: $0 <file.log>"
fi

# analyze file.log with grep
echo "Success requests: $(awk '$9 == 200 {count++} END {print count+0}' $file)"

echo "Top 5 largest requests: $(awk '{print $10}' $file | sort -rn | head -n 5 | tr '\n' ' ')"
