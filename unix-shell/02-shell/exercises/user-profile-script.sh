#!/bin/bash

# Prompt the user for their name
read -p "Write your name: " USERNAME

# Prompt the user for their favourite color
declare -A COLORS
COLORS=(["black"]="30" ["red"]="31" ["green"]="32" ["yellow"]="33"
["blue"]="34" ["magenta"]="35" ["cyan"]="36" ["white"]="37")

read -p "Write your favourite color: " FAV_COLOR

# Print a personalized greeting
echo -e "\e[${COLORS[${FAV_COLOR,,}]}mHello "${USERNAME}"\e[0m"
