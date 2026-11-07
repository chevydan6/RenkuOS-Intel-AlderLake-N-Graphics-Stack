#!/bin/bash

echo "=== Luanti/VoxeLibre Demo Script ==="
echo ""

echo "This script demonstrates the proper way to launch Luanti with VoxeLibre assets."
echo ""

# Show the directory structure
echo "Directory structure created:"
find /tmp/luanti-final -type d | head -10
echo "..."

echo ""
echo "VoxeLibre assets installed:"
ls -la /tmp/luanti-final/home/config/luanti/games/voxe-libre/
echo ""

echo "Launch script contents:"
cat /tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti/launch.sh
echo ""

echo "To actually run Luanti with VoxeLibre, you would:"
echo "1. Place the actual Luanti binary in /tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti/luanti"
echo "2. Run: /tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti/launch.sh"
echo ""
echo "This setup ensures that:"
echo "- Luanti can find its assets at the hardcoded path it expects"
echo "- The proper environment variables are set for OpenGL and library loading"
echo "- VoxeLibre game assets are available in the expected location"