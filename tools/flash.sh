#!/bin/bash
# Flash script for Motion Music Studio
# Automatically flashes .out file to LaunchPad

CCS_PATH="/opt/ti/ccs/ccs_base"
DSS="$CCS_PATH/DebugServer/bin/DSS"
PROJECT_NAME="Motion_Music_Studio"
OUT_FILE="Debug/$PROJECT_NAME.out"

if [ ! -f "$OUT_FILE" ]; then
    echo "Error: $OUT_FILE not found!"
    echo "Run ./build.sh first"
    exit 1
fi

echo "Flashing $OUT_FILE to MSPM0G3507..."
echo ""

$DSS -s flash.js

echo ""
echo "Flash complete!"