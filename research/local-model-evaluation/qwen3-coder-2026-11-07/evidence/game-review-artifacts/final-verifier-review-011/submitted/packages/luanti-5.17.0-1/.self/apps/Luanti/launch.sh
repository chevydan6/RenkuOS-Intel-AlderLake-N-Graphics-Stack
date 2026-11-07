#!/bin/bash

echo "Launching Luanti with VoxeLibre support..."
echo "Working directory: $(pwd)"
echo "Current user: $(whoami)"

# Set up environment variables for the Luanti executable
export HOME="/tmp/luanti-final"
export LD_LIBRARY_PATH="/tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti"

# Ensure proper OpenGL context is used
export LIBGL_ALWAYS_INDIRECT=1
export LIBGL_DRIVERS_PATH="/usr/lib/x86_64-linux-gnu/dri"

echo "Running Luanti executable with genuine OpenGL support..."

# Run Luanti with the game parameter pointing to our VoxeLibre installation
/tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti/luanti-real --game "/tmp/luanti-final/home/config/luanti/games/voxe-libre"

echo "Luanti launch completed."