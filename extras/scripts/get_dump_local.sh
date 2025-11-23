#!/bin/bash

# Get the latest dump file from local dumps directory
DUMPS_DIR="/vita-dev/soulcalibur_vita/dumps"

# Find the most recent psp2core dump file
LATEST_DUMP=$(ls -t "$DUMPS_DIR"/psp2core* 2>/dev/null | head -1)

if [ -z "$LATEST_DUMP" ]; then
    echo "Error: No dump files found in $DUMPS_DIR"
    exit 1
fi

echo "Found latest dump: $(basename "$LATEST_DUMP")"
cp "$LATEST_DUMP" coredump

if [ $? -eq 0 ]; then
    echo "Successfully copied to: coredump"
    ls -lh coredump
else
    echo "Error: Failed to copy dump file"
    exit 1
fi
