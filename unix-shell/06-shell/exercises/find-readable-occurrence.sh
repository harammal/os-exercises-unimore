#!/bin/bash

# correct usage
if [ $# -lt 2 ]; then
	echo "Usage: $0 <directory> <c1> <c2> <c3> ..."
	echo "Where <c1> <c2> <c3> ... are characters"
	exit 1
fi

# get the directory
dir="$1"

if [ ! -d "$dir" ]; then
	echo "Error: $dir is not a valid directory."
	exit 1
fi

# move the parameters: $@ contains all the characters
shift

# validate parameters
pattern=""
for param in "$@"; do
	if [ ${#param} -ne 1 ]; then
		echo "Error: $param is not a singol character."
		exit 1
	else
		# create the pattern string for grep
		pattern+=$param
	fi
done

find "$dir" -type d | while read -r current_dir; do
	if [ $(find "$current_dir" -maxdepth 1 -readable -type f -exec grep -El "[$pattern]" {} \; | wc -l) -gt 0 ]; then
		echo "+ $current_dir"
	else
		echo "- $current_dir"
	fi
done

# count the files that satisfy the condition
echo "Total files: $(find "$dir" -readable -type f -exec grep -El "[$pattern]" {} \; | wc -l)"
