#!/bin/bash

# Get terminal size
TOTAL_COLS=$(tput cols)
LAST_ROW=$(($(tput lines) - 1))

# Read and validate input
read -p "Inserisci un numero (0-100): " VALUE

if ! [[ "$VALUE" =~ ^[0-9]+$ ]] || [ "$VALUE" -lt 0 ] || [ "$VALUE" -gt 100 ]; then
    echo "Input non valido. Inserisci un numero compreso tra 0 e 100."
    exit 1
fi

# Calculate bar length proportional to the value
BAR_LENGTH=$((TOTAL_COLS * VALUE / 100))

# Move cursor to the last line, first column
tput cup "$LAST_ROW" 0

# Print the progress bar
BAR_CHAR="*"
printf "%${BAR_LENGTH}s" | tr ' ' "$BAR_CHAR"

# Reset cursor to next line
echo
