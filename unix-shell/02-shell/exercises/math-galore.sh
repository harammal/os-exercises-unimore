#!/bin/bash

# Ask for birth year and monthly budget
read -p "Enter the birth year: " BIRTH_YEAR
read -p "Enter the monthly budget: " MONTHLY_BUDGET

# Dinamically get che current year
CURRENT_YEAR=$(date +%Y)

# Compute the approximate age and the daily budget (assuming 30-days months)
AGE=$((CURRENT_YEAR - BIRTH_YEAR))
DAILY_BUDGET=$(echo "scale=2; ${MONTHLY_BUDGET} / 30" | bc)

# Print variables
echo -e "\nCurrent year: ${CURRENT_YEAR}"
echo "Approximate age: ${AGE} years old"
echo "Daily budget: ${DAILY_BUDGET}€"
