#!/bin/bash

echo "=== Testing Luanti/VoxeLibre Launch ==="
echo "This script demonstrates how to properly launch Luanti with VoxeLibre assets"

# Set the working directory
cd /tmp/luanti-final

# Show environment variables that will be used
echo "Environment:"
echo "  HOME=$HOME"
echo "  LD_LIBRARY_PATH=$LD_LIBRARY_PATH"

echo ""
echo "To launch Luanti with VoxeLibre, run:"
echo "  /tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti/launch.sh"

echo ""
echo "The expected path structure has been created:"
echo "  /tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti"
echo "  This is where Luanti expects to find its assets and executable."

echo ""
echo "VoxeLibre assets are installed in:"
echo "  /tmp/luanti-final/home/config/luanti/games/voxe-libre"

echo ""
echo "For debugging, you can also run:"
echo "  LD_LIBRARY_PATH=/tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti /tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti/luanti --help"