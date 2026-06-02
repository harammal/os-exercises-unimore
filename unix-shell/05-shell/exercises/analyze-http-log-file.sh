#!/bin/bash

# get the log file
file="$1"

# correct usage
extension="${file##*.}"

if [ $# -ne 1 -o $extension != 'log' ]; then
	echo "Usage: $0 <file.log>"
	exit 1
fi

# analyze file.log with grep
echo "Success requests: $(grep -c ' 200 ' $file)"

echo "Top 5 largest requests: $(grep -Eo ' [0-9]{3} [0-9]+ ' $file | cut -c2- | cut -d' ' -f2 | sort -rn | head -n 5 | tr '\n' ' ')"
