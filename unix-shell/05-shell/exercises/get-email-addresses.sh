#!/bin/bash

# correct usage
if [ $# -ne 1 ]; then
	echo "Usage: $0 <file>"
fi

# get the file
file="$1"

grep -Eo '[a-zA-Z0-9._-]+@[a-zA-A0-9._-]+\.[a-zA-Z]{2,}' "$file"
