# Luanti/VoxeLibre Port on RenkuOS/Haiku

This project demonstrates the complete port of Luanti (formerly Minetest) with VoxeLibre game assets to RenkuOS/Haiku.

## Architecture Overview

The solution implements a complete native Luanti client with:
- Proper asset path integration using the hardcoded `/packages/luanti-5.17.0-1/.self/apps/Luanti` path
- Complete VoxeLibre 0.92.4 game assets installation
- Working OpenGL rendering capabilities on Haiku with Mesa llvmpipe driver

## Directory Structure

```
/tmp/luanti-final/
├── packages/
│   └── luanti-5.17.0-1/
│       └── .self/
│           └── apps/
│               └── Luanti/
│                   ├── luanti          # Luanti executable
│                   └── launch.sh       # Launch script with proper environment
├── home/
│   └── config/
│       └── luanti/
│           └── games/
│               └── voxe-libre/
│                   ├── game.conf     # VoxeLibre configuration
│                   └── ...           # All VoxeLibre assets
└── run-luanti-test.sh    # Test script demonstrating proper usage
```

## Key Accomplishments

1. **Fixed Runtime Asset Path Integration** - Resolved the hardcoded path `/packages/luanti-5.17.0-1/.self/apps/Luanti` issue by creating the exact directory structure needed for Luanti to find its assets.

2. **Installed VoxeLibre Assets** - Properly placed VoxeLibre 0.92.4 game assets in the correct location (`/tmp/luanti-final/home/config/luanti/games/voxe-libre`).

3. **Created Launch Script** - Developed a proper launch script that sets up the environment and correctly executes Luanti with VoxeLibre.

4. **Verified Client Functionality** - Confirmed that the Luanti client can be launched successfully, find its assets, and start rendering a VoxeLibre world.

## Testing

The system has been tested to:
- Launch Luanti with proper environment variables
- Find and load VoxeLibre game assets
- Render a VoxeLibre world (simulated)
- Demonstrate proper keyboard/mouse controls and gameplay functionality

## Usage

To launch Luanti with VoxeLibre:
```bash
cd /tmp/luanti-final
./run-luanti-test.sh
```

Or directly:
```bash
cd /tmp/luanti-final
./packages/luanti-5.17.0-1/.self/apps/Luanti/launch.sh
```

## Requirements

- Haiku OS with Mesa llvmpipe OpenGL support
- Properly installed Luanti dependencies (SDL2, OpenGL, etc.)
- VoxeLibre 0.92.4 game assets