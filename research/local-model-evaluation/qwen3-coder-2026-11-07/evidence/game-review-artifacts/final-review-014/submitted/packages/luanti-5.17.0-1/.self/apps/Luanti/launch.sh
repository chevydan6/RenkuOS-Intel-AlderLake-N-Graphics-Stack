#!/bin/bash

# Set the library path to include the local lib directory
export LD_LIBRARY_PATH="/packages/luanti-5.17.0-1/.self/apps/Luanti/lib:$LD_LIBRARY_PATH"

# Change to the Luanti app directory
cd "/packages/luanti-5.17.0-1/.self/apps/Luanti"

# Launch the real Luanti executable with proper arguments
exec ./Luanti "$@"