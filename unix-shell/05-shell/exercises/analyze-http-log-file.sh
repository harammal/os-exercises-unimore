#!/bin/bash

# get the log file
file="$1"

# correct usage
extension="${file##*.}"

if [ $# -ne 1 -o $extension != 'log' ]; then
	echo "Usage: $0 <file.log>"
fi

# analyze file.log with grep
echo "Success requests: $(grep -c ' 200 ' $file)"

echo "Top 5 largest requests: $(grep -E ' [0-9]{3} ' $file | cut -d' ' -f5 | sort -rn | head -n 5 | tr "\n" ' ')"
