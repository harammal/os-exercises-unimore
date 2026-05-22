#!/bin/bash

DIR_PATH="$1"

# Create a timestamp for the archive name
TIMESTAMP=$(date +%Y-%m-%d_%H%M%S)
ARCHIVE_NAME="$(basename "${DIR_PATH}")_${TIMESTAMP}.tar.gz"

# Compress the directory
tar -czf "${ARCHIVE_NAME}" -C "$(dirname "${DIR_PATH}")" "$(basename "${DIR_PATH}")"

# Check the exit status of the tar command
TAR_STATUS=$?

# Store the archive size and print
ARCHIVE_SIZE=$(du -h "${ARCHIVE_NAME}" | cut -f1)
echo "Backup returned ${TAR_STATUS}"
echo "Size of the archive ${ARCHIVE_NAME}: ${ARCHIVE_SIZE}"
