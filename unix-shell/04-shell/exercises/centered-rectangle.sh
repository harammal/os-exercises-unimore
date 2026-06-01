#!/bin/bash

draw_rect() {
	# read the variables
	local x1=$1 y1=$2 x2=$3 y2=$4
	# calculate length and height
	local l=$((x2 - x1))
	local h=$((y2 - y1))

	# coordinates and data for debug
	#echo "x1=$x1, y1=$y1, x2=$x2, y2=$y2"
	#echo "l=$l, h=$h"
	#echo "cols=$(tput cols), lines=$(tput lines)"

	# print upper base
	tput cup $y1 $x1
	for (( i=0; i<l; i++ )); do
		printf "+"
	done
	echo ""

	#print both heights with a custom background color
	for (( i=0; i<h-2; i++ )); do
		tput cuf $x1
		printf "+\e[46m%*s\e[0m+\n" $((l-2))
	done

	# print lower  base
	tput cup $((y2-1)) $x1
	for (( i=0; i<l; i++ )); do
		printf "+"
	done
	echo ""

	# Hide the cursor and move it out of the way
	tput civis
	tput cup 0 0
}

rect() {
	# calculate length and height
	local l=$(echo "$(tput cols) / 2" | bc)
	local h=$(echo "$(tput lines) / 2" | bc)

	# calculate the coordinates
	local x1=$((l / 2))
	local y1=$((h / 2))
	local x2=$((l + l / 2))
	local y2=$((h + h / 2))

	# call the function passing the coordinates
	draw_rect $x1 $y1 $x2 $y2
}

# SIGWINCH handler
trap 'clear; rect' SIGWINCH

# SIGINT handler
trap 'tput cnorm; clear; exit 0' SIGINT SIGTERM

# start
clear
rect

# main loop
while true; do
	sleep 1
done
