# Developer Handoff - DentScanAlign

## Current Status

**Date:** 2026-06-05
**Build Status:** MeshIO and single-file mode implemented; needs rebuild/test on target workstation
**Phase:** Mesh import/export expansion and manual single-file workflow added after Phase 5 UX polish

## Recent Changes (Latest Session)


### Session 2026-06-05 (Manual Single-File Workflow)

- **Single file processing UI**: Added a separate group for selecting one input mesh file and one explicit output mesh file
- **Manual input selection**: User can choose one `.stl`, `.ply`, or `.obj` file without preparing a batch directory
- **Manual output selection**: User can choose the exact transformed output path; parent directories are created when needed
- **Save as STL integration**: The existing checkbox also applies to single-file mode. Checked forces `.stl`; unchecked keeps the input file format
- **Extension enforcement**: Output extension is adjusted automatically to match the active export mode
- **Overwrite protection**: Single-file mode refuses to save when input and output resolve to the same file path
- **Single-file JSON metadata**: Alignment metadata is saved next to the selected output mesh with the same basename and `.json` extension
- **Shared alignment workflow**: Single-file mode reuses the same landmark picking, transform preview, `AlignmentRecord`, and `MeshIO::writeTransformed()` save path
- **Batch mode preserved**: Existing directory-based batch workflow remains unchanged
- **QtConcurrent warning fix**: Batch save now stores the returned `QFuture` in a `[[maybe_unused]]` local to silence Qt 6 `nodiscard` warnings

### Session 2026-06-05 (Mesh Format Import/Export Expansion)

- **Multi-format mesh discovery**: Session scanning now includes `.stl`, `.ply`, and `.obj` files instead of STL-only input
- **New MeshIO layer**: Added `src/core/MeshIO.h/cpp` as the format-aware reader/writer path used by session saving
- **STL / PLY / OBJ readers**: Added format detection by extension and readers for supported mesh formats
- **Format-preserving export**: When the new UI option is unchecked, normalized files are written back in the same format as the input file
- **Save as STL option**: Added a user-facing checkbox to force all normalized outputs to `.stl` after transformation
- **Metadata preservation**: PLY/OBJ export paths are designed to keep non-geometric records where possible, including colors, comments, material references, groups, texture coordinates, and custom PLY properties
- **Normal handling**: Normals are transformed or recomputed where needed after applying the landmark-based transform
- **CMake update**: Added `src/core/MeshIO.cpp` to `CORE_SOURCES` so the new implementation is compiled

### Session 2026-06-04 (UX Polish)

- **Progress bar fix**: Changed from `skipCurrent()` to new `markCurrentAsProcessed()` method that increments both `m_currentIndex` and `m_processedCount`, so progress bar advances correctly after each save
- **Focus Save button**: After 3rd landmark click and transform computation, focus moves to "Save & Next" button for quick keyboard/mouse access
- **Window title update**: Added "Prof. Kunzelmann" to title: `"DentScanAlign - Coordinate Normalization Tool (Prof. Kunzelmann)"`
- **About dialog**: Added Help menu with "About..." option showing:
  - Prof. Dr. Karl-Heinz Kunzelmann
  - Clickable link to www.kunzelmann.de
  - Version 1.0
- **Last scan handling**: Fixed error when processing final scan - the progress bar fix also ensures `remainingCount()` returns 0 correctly, showing completion message instead of error

### Previous Session

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
- **Camera reset**: Camera resets when Reset button clicked or transform computed
- **Standard view after transform**: Camera resets to axes-aligned view (Z toward viewer, Y up) showing occlusal surface
- **Background save**: File I/O runs in background via QtConcurrent, allowing immediate continuation to next scan

## What Was Implemented

### Phase 1: Project Setup & Core Infrastructure ✓

- **CMakeLists.txt** - Build configuration for Qt6, VTK 9.3, CGAL 6.0.1, Eigen3
- **Directory structure** created: `src/core/`, `src/alignment/`, `src/gui/`
- **Core files** copied and adapted from DentScanComparePro:
  - `Mesh.h` - ScanData struct, CGAL SurfaceMesh type aliases
  - `STLReader.h/cpp` - Binary STL parser with per-face winding correction
  - `ToothSegmentation.h/cpp` - Dijkstra-based region growing from seed points
- **STLWriter.h/cpp** - Writer for normalized STL output
- **MeshIO.h/cpp** - Format-aware STL/PLY/OBJ import/export wrapper with optional forced-STL output

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
  - Recursive mesh file discovery for `.stl`, `.ply`, and `.obj` inputs
  - JSON alignment file I/O (custom parser, no external JSON library)
  - Writes both alignment JSON and normalized mesh output on save
  - Honors the Save as STL option when deciding the output extension/format
  - Flat filename scheme for JSON: `Scanner_A__SKD30__scan1.json`
  - `markCurrentAsProcessed()` method for accurate progress tracking with background saves

### Phase 4: GUI ✓

- **MeshViewWidget.h/cpp**
  - VTK-based 3D mesh rendering using `QVTKOpenGLNativeWidget`
  - Point picking mode (click detection vs drag)
  - Picked point visualization (yellow spheres)
  - Region highlighting (green point cloud)
  - Transform preview (renders mesh with applied 4x4 transform)

- **AlignmentWidget.h/cpp**
  - Landmark status indicators (○ unpicked, ● picked)
  - Action buttons: Undo Last, Reset
  - Preview checkbox (toggle normalized view)
  - Skip and Save & Next buttons
  - Status label for user guidance
  - Transform auto-computed when 3rd point clicked (no manual button)
  - `focusSaveButton()` method for keyboard/mouse accessibility after transform

- **MainWindow.h/cpp**
  - Batch directory selection UI (input/output directories)
  - Single file processing UI (one input mesh file and one explicit output mesh file)
  - Save as STL checkbox for forced STL export versus preserving the input format
  - Progress bar and scan counter / single-file status label
  - Help menu with About dialog (Prof. Dr. Karl-Heinz Kunzelmann, www.kunzelmann.de)
  - Orchestrates the full workflow:
    1. Initialize batch session or open manually selected single file
    2. Load next unprocessed scan, or load the selected single file
    3. Enable point picking
    4. Grow regions around picked landmarks
    5. Compute transform when 3 landmarks picked
    6. Focus Save button for quick access
    7. Preview and save

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
│   │   ├── MeshIO.h
│   │   ├── MeshIO.cpp
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

`CMakeLists.txt` must include `src/core/MeshIO.cpp` in `CORE_SOURCES`. No additional external dependency was added for the mesh format work.

## What Needs Testing (Phase 5)

### Functional Testing

1. **Load mesh files** - Test STL, PLY, and OBJ scans from representative study folders and via single-file mode
2. **Point picking** - Verify clicks on mesh surface return correct 3D coordinates
3. **Region growing** - Check that Dijkstra expansion from seed creates reasonable regions
4. **Plane fitting** - Confirm plane normal and axes are computed correctly
5. **Transform preview** - Toggle should show mesh in normalized orientation
6. **JSON output** - Verify alignment JSON is written with correct structure
7. **Normalized mesh output** - Confirm output vertices are transformed correctly for STL, PLY, and OBJ
8. **Single-file workflow** - Select one input file, choose output path, align, save, and verify both mesh and sidecar JSON are created
9. **Single-file overwrite protection** - Confirm the app rejects identical input/output paths
10. **Save as STL in single-file mode** - Confirm output extension changes to `.stl` when checked and follows input format when unchecked

### Edge Cases to Test

- Empty directories
- Unsupported files in input directory should be ignored
- Mixed STL/PLY/OBJ input directories
- Single-file mode with output directory that does not yet exist
- Single-file mode with manually typed output extension inconsistent with Save as STL setting
- Uppercase extensions such as `.STL`, `.PLY`, `.OBJ`
- PLY files with vertex colors/custom properties
- OBJ files with material libraries, groups, texture coordinates, normals, and comments
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
- **Standard camera view**: After transform, camera resets to standard orientation with Z toward viewer (occlusal view), Y pointing up (anterior), X pointing right. This ensures consistent view regardless of how user rotated mesh during landmark picking.
- **Background file I/O**: JSON and mesh writing happens in a separate thread via `QtConcurrent::run()`, allowing the user to continue with the next scan immediately
- **Save button focus**: After transform computed, focus automatically moves to "Save & Next" button for efficient keyboard/mouse workflow
- **Progress tracking**: Uses `markCurrentAsProcessed()` instead of `skipCurrent()` when saving, ensuring progress bar updates correctly even with background saves
- **Format preservation default**: The Save as STL checkbox controls output format. Unchecked preserves the input extension/format; checked forces `.stl` output.
- **Two workflow modes**: Batch processing remains the primary high-throughput workflow. Single-file mode is intentionally a thin path through the same landmark/normalization/save code, with manually selected input/output paths.
- **Single-file sidecar JSON**: Unlike batch mode, which stores JSON under `output/alignments/`, single-file mode writes the JSON next to the chosen output mesh so the mesh and transform metadata travel together.
- **No original overwrite in single-file mode**: The GUI rejects equal input and output paths to protect source scans.
- **Metadata preservation goal**: MeshIO should avoid reducing PLY/OBJ files to only vertices/faces. Keep colors, comments, texture coordinates, material references, groups, and custom PLY properties where the parser/writer supports them.
- **Normals after transform**: Transform normals with the rotation part of the transform when available. Recompute normals when the input does not provide usable normals or when the output format requires them.

### Known Limitations

- Single-file mode currently saves synchronously and shows a confirmation dialog; this is acceptable for one file, while batch mode remains background-saved
- Single-file mode does not maintain a queue or history; it is intended for one manually selected file at a time
- The new MeshIO path has not yet been fully validated against a large corpus of real-world PLY/OBJ scanner outputs
- Format-specific metadata preservation depends on what the parser recognizes; test with target scanner exports before relying on every custom record
- OBJ material files (`.mtl`) may need path handling checks when output is written to the normalized directory
- No "Review Mode" for browsing already-processed scans (mentioned in plan but not implemented)
- No two-observer verification workflow
- Region highlighting removed (regions only computed on "Compute")

## Potential Issues to Watch

1. **ToothSegmentation parameters** - The `maxGeodesicMm = 2.0` in `MainWindow::growRegion()` creates ~2mm regions; may need tuning for different scan types

2. **Axis orientation** - The logic in `LandmarkPlaneFitter::fitPlane()` that determines which way Z points may need verification with real data

3. **JSON parsing** - The custom JSON parser is minimal; complex strings with special characters may not parse correctly

4. **VTK threading** - All VTK operations must happen on main thread; don't add background processing without care. Note: the background save is safe because it only writes files (no VTK calls)

5. **Background save race condition** - If user quits before background save completes, data may be lost. Consider adding graceful shutdown that waits for pending writes

6. **Single-file output extension enforcement** - `adjustedSingleOutputPath()` rewrites the extension based on Save as STL and input format. Verify this remains consistent if additional formats are added later

7. **Single-file synchronous save UX** - Large single files may temporarily block the UI during save. If this becomes noticeable, move single-file save to the same background pattern as batch mode but keep the completion/error dialog on the main thread

## Dependencies

| Library | Version | Notes |
|---------|---------|-------|
| Qt | 6.x | Widgets, Concurrent modules |
| VTK | 9.3 | Custom build at `~/VTK-install-linux/` |
| CGAL | 6.0.1 | Uses `std::optional` property_map API |
| Eigen | 3.4.0 | Matrix math |
| Boost | 1.83+ | Required by CGAL |

## Reference Projects

- **DentScanComparePro** (`~/claude-code/DentScanComparePro/`) - Source for core files, batch processing pipeline
- **DentScanCompare** (`~/claude-code/DentScanCompare/`) - Original comparison tool

## Next Phase: DentScanComparePro

DentScanAlign (this tool) is **complete and working**. The next phase continues in **DentScanComparePro** (`~/claude-code/DentScanComparePro/`).

### Tasks for DentScanComparePro

1. **Fix segmentation fault** - The program crashes on startup. This was introduced during recent axis alignment improvements in the segmentation module. Remove the problematic transformations.

2. **Add external reference registration** - Currently supports GPA mean or fixed scanner reference. Need to add:
   - `reference_strategy: "external_mesh: /path/to/reference.stl"` config option
   - Load external gold-standard mesh as reference
   - Register all scans directly to external reference via ICP (skip GPA)

3. **Enhance R-compatible export** - Existing CSV export is good, but consider adding:
   - Wide-format CSV (pivot table for multivariate analysis)
   - Per-vertex distance export for granular analysis
   - Metadata summary JSON (study-level processing record)

### Workflow Integration

1. **DentScanAlign** normalizes scans → outputs to `normalized/` directory
2. **DentScanComparePro** reads normalized scans → registers to reference → computes metrics → exports CSV for R

### What Already Works in DentScanComparePro

- ICP registration (point-to-plane)
- GPA mean reference computation
- Trueness metrics (RMS, MAD, Hausdorff, bias, coverage)
- Precision metrics (pairwise RMS, SD, CV)
- CSV export (long format, R-compatible)
- Batch processing with progress tracking

## Contact

For questions about the implementation, refer to:
- `CLAUDE.md` - Project context and conventions
- `docs/implementation-plan.md` - Full design specification
