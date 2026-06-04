# Developer Handoff - DentScanAlign

## Current Status

**Date:** 2026-06-04
**Build Status:** Compiles and links successfully
**Phase:** Phase 5 - Initial testing complete, transformation working

## Recent Changes (Latest Session)

- Reduced landmark region size from 8mm to 2mm
- Made clicking instant by deferring region growing to "Compute Transform"
- Fixed high-DPI point picking offset using `devicePixelRatioF()`
- Added persistent paths via QSettings (survives rebuilds)
- Preview is now auto-enabled after computing transform
- Camera resets when toggling preview (transformed mesh centered at origin)
- **Auto-compute**: Transform computed automatically when 3rd point clicked (removed manual button)
- **Reset button**: Renamed "Clear All" to "Reset" for clarity
- **Simplified labels**: Changed "Midline, Right, Left" to "Point 1 (midline), 2, 3 (clockwise)"
- **Fixed Z-axis orientation**: Uses clockwise point convention for consistent up direction
- **X-axis alignment**: X-axis now parallel to line between points 2 and 3
- **Curvature coloring**: Mesh displays with convex=orange, concave=blue coloring to distinguish top from bottom

## What Was Implemented

### Phase 1: Project Setup & Core Infrastructure ✓

- **CMakeLists.txt** - Build configuration for Qt6, VTK 9.3, CGAL 6.0.1, Eigen3
- **Directory structure** created: `src/core/`, `src/alignment/`, `src/gui/`
- **Core files** copied and adapted from DentScanComparePro:
  - `Mesh.h` - ScanData struct, CGAL SurfaceMesh type aliases
  - `STLReader.h/cpp` - Binary STL parser with per-face winding correction
  - `ToothSegmentation.h/cpp` - Dijkstra-based region growing from seed points
- **STLWriter.h/cpp** - New file for writing normalized STL output with transform

### Phase 2: Alignment Math ✓

- **LandmarkPlaneFitter.h/cpp**
  - Input: 3 sets of vertex positions (midline, right, left regions)
  - Uses PCA (eigendecomposition of covariance matrix) to fit plane
  - Computes X/Y/Z axes from plane normal and landmark centroids
  - Output: `PlaneResult` with normal, centroid, and orthonormal axes

- **CoordinateNormalizer.h/cpp**
  - Computes full 4x4 transformation matrix
  - Translates mesh centroid to origin
  - Rotates to align with anatomical axes (X=left-right, Y=posterior-anterior, Z=apical-occlusal)
  - Utility functions for matrix↔array conversion (for JSON serialization)

### Phase 3: Session Management ✓

- **AlignmentSession.h/cpp**
  - `Session` class: manages input/output directories, tracks processed scans
  - Recursive STL file discovery
  - JSON alignment file I/O (custom parser, no external JSON library)
  - Writes both alignment JSON and normalized STL on save
  - Flat filename scheme for JSON: `Scanner_A__SKD30__scan1.json`

### Phase 4: GUI ✓

- **MeshViewWidget.h/cpp**
  - VTK-based 3D mesh rendering using `QVTKOpenGLNativeWidget`
  - Point picking mode (click detection vs drag)
  - Picked point visualization (yellow spheres)
  - Region highlighting (green point cloud)
  - Transform preview (renders mesh with applied 4x4 transform)

- **AlignmentWidget.h/cpp**
  - Landmark status indicators (○ unpicked, ● picked)
  - Action buttons: Undo Last, Clear All, Compute Transform
  - Preview checkbox (toggle normalized view)
  - Skip and Save & Next buttons
  - Status label for user guidance

- **MainWindow.h/cpp**
  - Directory selection UI (input/output paths)
  - Progress bar and scan counter
  - Orchestrates the full workflow:
    1. Initialize session
    2. Load next unprocessed scan
    3. Enable point picking
    4. Grow regions around picked landmarks
    5. Compute transform when 3 landmarks picked
    6. Preview and save

- **main.cpp**
  - QApplication setup with VTK OpenGL format

## File Structure

```
DentScanAlign/
├── CMakeLists.txt
├── CLAUDE.md
├── docs/
│   ├── implementation-plan.md
│   ├── INITIAL-PROMPT.md
│   └── developer-handoff.md      ← this file
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── Mesh.h
│   │   ├── STLReader.h
│   │   ├── STLReader.cpp
│   │   ├── STLWriter.h
│   │   ├── STLWriter.cpp
│   │   ├── ToothSegmentation.h
│   │   └── ToothSegmentation.cpp
│   ├── alignment/
│   │   ├── LandmarkPlaneFitter.h
│   │   ├── LandmarkPlaneFitter.cpp
│   │   ├── CoordinateNormalizer.h
│   │   ├── CoordinateNormalizer.cpp
│   │   ├── AlignmentSession.h
│   │   └── AlignmentSession.cpp
│   └── gui/
│       ├── MainWindow.h
│       ├── MainWindow.cpp
│       ├── AlignmentWidget.h
│       ├── AlignmentWidget.cpp
│       ├── MeshViewWidget.h
│       └── MeshViewWidget.cpp
└── build/
    └── DentScanAlign              ← compiled executable
```

## Build Instructions

```bash
cd ~/claude-code/DentScanAlign
mkdir -p build && cd build
cmake ..
make -j$(nproc)
./DentScanAlign
```

## What Needs Testing (Phase 5)

### Functional Testing

1. **Load STL files** - Test with scans from `/KHKsData/usr2/daten/P2026-Kessler-ScanVergleich/IOS/`
2. **Point picking** - Verify clicks on mesh surface return correct 3D coordinates
3. **Region growing** - Check that Dijkstra expansion from seed creates reasonable regions
4. **Plane fitting** - Confirm plane normal and axes are computed correctly
5. **Transform preview** - Toggle should show mesh in normalized orientation
6. **JSON output** - Verify alignment JSON is written with correct structure
7. **Normalized STL** - Confirm output STL has vertices transformed correctly

### Edge Cases to Test

- Empty directories
- Non-STL files in input directory
- Very large meshes (performance)
- Meshes with unusual orientations
- Re-running on partially processed directories

### Design Decisions

- **Deferred region growing**: Regions are NOT grown on each click (too slow). Instead, all 3 regions are grown automatically when 3rd point is clicked. This makes clicking instant.
- **Small landmark regions**: `maxGeodesicMm = 2.0` creates ~2mm radius regions around each seed point
- **Sphere visualization**: Yellow spheres (0.6mm radius, RGB 1.0/0.85/0.0) matching DentScanCompare style
- **Persistent paths**: Input/output directories saved via QSettings (`~/.config/DentScan/DentScanAlign.conf`), survive rebuilds
- **High-DPI support**: Point picking uses `devicePixelRatioF()` for correct coordinate conversion
- **Auto-preview**: After computing transform, preview is automatically enabled and camera resets to show normalized mesh
- **Clockwise point convention**: User clicks 3 points clockwise from midline when viewing from occlusal; this determines Z-axis direction
- **Axis alignment**: X-axis is parallel to line between points 2-3; Y-axis perpendicular pointing toward point 1
- **Curvature coloring**: Mean curvature visualization (convex=orange, concave=blue) helps distinguish occlusal from apical view

### Known Limitations

- No "Review Mode" for browsing already-processed scans (mentioned in plan but not implemented)
- No two-observer verification workflow
- Region highlighting removed (regions only computed on "Compute")

## Potential Issues to Watch

1. **ToothSegmentation parameters** - The `maxGeodesicMm = 2.0` in `MainWindow::growRegion()` creates ~2mm regions; may need tuning for different scan types

2. **Axis orientation** - The logic in `LandmarkPlaneFitter::fitPlane()` that determines which way Z points may need verification with real data

3. **JSON parsing** - The custom JSON parser is minimal; complex strings with special characters may not parse correctly

4. **VTK threading** - All VTK operations must happen on main thread; don't add background processing without care

## Dependencies

| Library | Version | Notes |
|---------|---------|-------|
| Qt | 6.x | Widgets module |
| VTK | 9.3 | Custom build at `~/VTK-install-linux/` |
| CGAL | 6.0.1 | Uses `std::optional` property_map API |
| Eigen | 3.4.0 | Matrix math |
| Boost | 1.83+ | Required by CGAL |

## Reference Projects

- **DentScanComparePro** (`~/claude-code/DentScanComparePro/`) - Source for core files, batch processing pipeline
- **DentScanCompare** (`~/claude-code/DentScanCompare/`) - Original comparison tool

## Contact

For questions about the implementation, refer to:
- `CLAUDE.md` - Project context and conventions
- `docs/implementation-plan.md` - Full design specification
