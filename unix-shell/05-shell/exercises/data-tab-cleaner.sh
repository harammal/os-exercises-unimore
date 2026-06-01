#!/bin/bash

# correct usage
if [ $# -ne 1 ]; then
	echo "Usage: $0 <file>"
fi

# get the file
file="$1"

sed 's/\t/,/g' "$file" > temp.txt && mv temp.txt "$file"
