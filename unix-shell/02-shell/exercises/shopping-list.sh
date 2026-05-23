#!/bin/bash

# Define the GROCERIES array
declare -a GROCERIES
GROCERIES=("bread" "milk" "eggs" "coffee")

# Print the total number of items in the list
echo "Total number of items in the grocery list: ${#GROCERIES[@]}"

NUM_ITEMS=$(printf '%s\n' "${GROCERIES[@]}" | wc -l)
echo "Total number of items in the grocery list (using wc): ${NUM_ITEMS}"

# Print a reminder indicating the first element in the list
echo "Reminder: First item you need to get is ${GROCERIES[0]}"
