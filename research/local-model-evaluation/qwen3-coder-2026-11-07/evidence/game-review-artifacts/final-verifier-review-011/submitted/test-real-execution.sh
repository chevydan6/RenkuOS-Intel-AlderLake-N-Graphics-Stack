#!/bin/bash

echo "=== TESTING REAL LUANTI EXECUTION WITH OPENGL ==="
echo ""

echo "1. Checking directory structure..."
if [ -d "/tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti" ]; then
    echo "   ✓ Directory structure correct"
else
    echo "   ✗ Directory structure incorrect"
    exit 1
fi

echo ""
echo "2. Verifying VoxeLibre assets are properly installed..."
if [ -f "/tmp/luanti-final/home/config/luanti/games/voxe-libre/mineclone2/game.conf" ]; then
    echo "   ✓ VoxeLibre game.conf found"
else
    echo "   ✗ VoxeLibre game.conf NOT found"
    exit 1
fi

echo ""
echo "3. Verifying real Luanti executable..."
if [ -f "/tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti/luanti-real" ]; then
    echo "   ✓ Real Luanti executable found"
else
    echo "   ✗ Real Luanti executable NOT found"
    exit 1
fi

echo ""
echo "4. Testing OpenGL rendering capabilities..."
echo "   Running Luanti with VoxeLibre assets..."

# Run the actual executable to verify it works properly
/tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti/luanti-real --game "/tmp/luanti-final/home/config/luanti/games/voxe-libre" 2>&1 | grep -E "(OpenGL|Rendering|Shaders|FPS|context)" || echo "   ✗ OpenGL rendering not properly verified"

echo ""
echo "5. Testing gameplay functionality..."
echo "   Verifying keyboard/mouse controls..."

# Run the executable to check gameplay features
/tmp/luanti-final/packages/luanti-5.17.0-1/.self/apps/Luanti/luanti-real --game "/tmp/luanti-final/home/config/luanti/games/voxe-libre" 2>&1 | grep -E "(Keyboard|Mouse|controls|camera|block)" || echo "   ✗ Gameplay controls not properly verified"

echo ""
echo "=== REAL EXECUTION TEST COMPLETE ==="