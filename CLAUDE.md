# CLAUDE.md - DentScanAlign Project Context

## Project Overview

**DentScanAlign** is a standalone Qt/VTK tool for interactive pre-alignment of dental intraoral STL scans. It normalizes arbitrary scanner coordinate systems to a standard anatomical orientation before batch ICP registration in DentScanComparePro.

## Quick Start

```bash
cd ~/claude-code/DentScanAlign
mkdir build && cd build
cmake ..
make -j$(nproc)
./DentScanAlign
```

## Key Documentation

- **Implementation Plan**: `docs/implementation-plan.md` - Full design specification
- **Initial Prompt**: `docs/INITIAL-PROMPT.md` - Phased implementation guide

## Build Environment

- **Qt 6** (Widgets)
- **VTK 9.3** custom build: `~/VTK-install-linux/lib/cmake/vtk-9.3`
- **CGAL 6.0.1** (property_map uses `std::optional`, not `std::pair`)
- **Eigen 3.4.0**
- **nanoflann 1.7** (header-only KD-tree)
- **Platform**: Debian 13 / Linux
- **C++ Standard**: C++17

## CMake Critical Setup

```cmake
project(DentScanAlign VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Custom VTK installation
set(VTK_DIR "$ENV{HOME}/VTK-install-linux/lib/cmake/vtk-9.3")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(VTK 9.3 REQUIRED COMPONENTS
    CommonCore CommonDataModel
    FiltersSources FiltersGeneral
    InteractionStyle
    RenderingCore RenderingOpenGL2
    GUISupportQt
)
find_package(CGAL REQUIRED)
find_package(Eigen3 REQUIRED)
```

## Code Reuse

Core files copied from `~/claude-code/DentScanComparePro/src/core/`:
- `Mesh.h` - ScanData struct, SurfaceMesh type aliases
- `STLReader.{h,cpp}` - Binary STL parser with winding auto-correction
- `ToothSegmentation.{h,cpp}` - Dijkstra-based region growing

## Target Coordinate Standard

- **X-Axis (Transversal)**: Left (-) to Right (+)
- **Y-Axis (Sagittal)**: Posterior (-) to Anterior (+), midline direction
- **Z-Axis (Vertical)**: Apical (-) to Occlusal (+)
- **Origin**: Centroid of entire mesh

## Workflow Summary

1. User picks 3 seed points: midline, right-side, left-side
2. Dijkstra grows regions around each seed
3. Plane fitted through all region vertices → XY plane
4. Y-axis = direction toward midline, X-axis = perpendicular in plane
5. 4x4 transform computed and saved to JSON
6. Normalized STL written to output directory

## Directory Structure

```
src/
├── main.cpp
├── core/           # Mesh handling, STL I/O, segmentation
├── alignment/      # Transform computation, session management
└── gui/            # Qt widgets, VTK rendering
```

## Related Projects

- **DentScanComparePro**: `~/claude-code/DentScanComparePro/` - Batch processing pipeline (reads normalized output from this tool)
- **DentScanCompare**: `~/claude-code/DentScanCompare/` - Original comparison tool (reference implementation)

## Common Pitfalls

1. **CGAL 6.0 property_map**: `mesh.property_map<T>("name")` returns `std::optional`, not `std::pair`
2. **VTK threading**: VTK objects are NOT thread-safe; update from main thread only
3. **VTK point picking**: Use `vtkCellPicker` or `vtkPointPicker` with proper interactor setup
4. **STL winding**: STLReader auto-corrects; no manual handling needed

## Test Data

Dental scan STL files: `/KHKsData/usr2/daten/P2026-Kessler-ScanVergleich/IOS/`
