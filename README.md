# Terrain Erosion Simulation

Real-time terrain rendering and tessellation simulation built with C++ and DirectX, developed as part of a Computer Vision & Graphics course.

## Overview

This project implements a GPU-accelerated terrain rendering pipeline using hardware tessellation. The terrain geometry is generated and refined dynamically on the GPU using Hull, Domain, and Geometry shaders, allowing for adaptive level-of-detail based on camera distance. Shadow mapping and dynamic lighting are also implemented.

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Language | C++ |
| Graphics API | DirectX (HLSL shaders) |
| Build System | CMake |
| Shader Stages | Vertex, Hull, Domain, Geometry, Pixel |

## Project Structure

```
terrain-erosion-simulation/
└── Proyecto/
    └── Tutorial08_Tessellation/
        ├── assets/                 # HLSL shader files
        │   ├── terrain.vsh         # Vertex shader
        │   ├── terrain.hsh         # Hull shader (tessellation control)
        │   ├── terrain.dsh         # Domain shader (tessellation evaluation)
        │   ├── terrain.gsh         # Geometry shader
        │   ├── terrain.psh         # Pixel shader
        │   ├── terrain_wire.psh    # Wireframe pixel shader
        │   ├── shadowmap.vsh       # Shadow map vertex shader
        │   ├── lighting.fxh        # Lighting helper functions
        │   └── structures.fxh      # Shared data structures
        ├── src/
        │   ├── Tutorial08_Tessellation.cpp
        │   └── Tutorial08_Tessellation.hpp
        └── CMakeLists.txt
```

## Key Concepts

- **Hardware Tessellation** — Dynamic mesh subdivision on the GPU using the Hull and Domain shader stages
- **Shadow Mapping** — Real-time shadow generation with a dedicated render pass
- **Lighting Model** — Per-pixel lighting computed in the pixel shader using shared helper functions
- **Wireframe Mode** — Alternate rendering pipeline to visualize tessellation topology

## Build Instructions

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

> Requires a DirectX 11-compatible GPU and the Windows SDK.

## Course

Computer Vision — Universidad de los Andes