#!/bin/bash

# Deployment variables
read -p "Enter the base directory (BASE): " BASE
read -p "Enter the project name (PROJECT): " PROJECT
read -p "Enter the version (VERSION): " VERSION
APP_DIR="${BASE}/${PROJECT}/${VERSION}"

# Create the directory layout including src, build, and docs
# The -p flag ensures parent directories are created if they don't exist
mkdir -p "${APP_DIR}/src"
mkdir -p "${APP_DIR}/build"
mkdir -p "${APP_DIR}/docs"
