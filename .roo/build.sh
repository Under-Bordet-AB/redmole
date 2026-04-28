#!/usr/bin/env bash

# Go to project root, assuming this script is in ./scripts/
cd "$(dirname "$0")/.."

# Load ESP-IDF environment
. "$HOME/esp-idf/export.sh"

# Build project
idf.py build