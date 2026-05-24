#!/bin/bash

if [[ $# -ne 1 || "$1" == .* ]]; then
	echo "Usage: "$0" <extension>"
	exit 1
fi

EXTENSION=".$1"
PREFIX="backup-"

ls *"${EXTENSION}" | xargs -I {} bash -c '
	FILE="{}"
	echo "Working on ${FILE}"
	PREFIX="backup-"
	if [[ "${FILE}" != "${PREFIX}"* ]]; then
		mv "${FILE}" "${PREFIX}${FILE}"
		echo "Renamed: ${FILE} --> ${PREFIX}${FILE}"
	else
		echo "Skipping ${FILE} (already has prefix)"
	fi
'
