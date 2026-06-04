# DentScanAlign - Implementation Plan

## Overview

**DentScanAlign** is a standalone Qt/VTK tool for interactive pre-alignment of dental intraoral STL scans. It normalizes arbitrary scanner coordinate systems to a standard anatomical orientation before batch ICP registration.

## Problem Statement

Intraoral STL scans from multiple vendors (iTero, Trios, Primescan, etc.) have:
- Permuted axes
- Inverted directions
- Arbitrary rotational offsets

A configuration-driven profile approach doesn't fit reality because variations exist even within the same scanner model. Instead, we use an **interactive-first workflow** where the user aligns each scan once, saves the transformation, and reuses it.

## Target Coordinate Standard

- **X-Axis (Transversal)**: Left (-) to Right (+)
- **Y-Axis (Sagittal)**: Posterior (-) to Anterior (+), aligned with midline
- **Z-Axis (Vertical)**: Apical (-) to Occlusal (+)
- **Origin**: Centroid of entire mesh

## Directory Structure

```
input_dir/                          # User specifies (original STLs)
├── Scanner_A/
│   ├── SKD30/
│   │   ├── scan1.stl
│   │   └── scan2.stl
│   └── SKD28/
│       └── scan3.stl
└── Scanner_B/
    └── ...

output_dir/                         # User specifies
├── alignments/                     # JSON per scan (transformation records)
│   ├── Scanner_A__SKD30__scan1.json
│   ├── Scanner_A__SKD30__scan2.json
│   └── ...
└── normalized/                     # Mirrors input structure
    ├── Scanner_A/
    │   ├── SKD30/
    │   │   ├── scan1.stl
    │   │   └── scan2.stl
    │   └── SKD28/
    │       └── scan3.stl
    └── Scanner_B/
        └── ...
```

## JSON Alignment File Structure (per scan)

```json
{
  "source_file": "Scanner_A/SKD30/scan1.stl",
  "landmarks": {
    "midline": {"seed": [x, y, z], "vertex_count": 123},
    "right": {"seed": [x, y, z], "vertex_count": 89},
    "left": {"seed": [x, y, z], "vertex_count": 95}
  },
  "fitted_plane_normal": [nx, ny, nz],
  "computed_axes": {
    "x": [xx, xy, xz],
    "y": [yx, yy, yz],
    "z": [zx, zy, zz]
  },
  "mesh_centroid": [cx, cy, cz],
  "transform_4x4": [16 values, row-major],
  "timestamp": "2026-06-04T..."
}
```

## UI Design

```
┌─────────────────────────────────────────────────────────┐
│  DentScanAlign                                          │
├─────────────────────────────────────────────────────────┤
│  [Input Dir: /path/to/stls    ] [Browse]                │
│  [Output Dir: /path/to/output ] [Browse]                │
│  [Start Processing]  [Review Processed]                 │
├─────────────────────────────────────────────────────────┤
│                                                         │
│          ┌─────────────────────────────┐                │
│          │                             │                │
│          │      3D Mesh View           │                │
│          │      (VTK Widget)           │                │
│          │                             │                │
│          │   Click to pick landmarks   │                │
│          │                             │                │
│          └─────────────────────────────┘                │
│                                                         │
│  Progress: scan1.stl (3 of 47)                          │
│  Status: Pick landmark 1 (midline)                      │
│                                                         │
│  Landmarks:  ● Midline  ○ Right  ○ Left                 │
│                                                         │
│  [Undo Last] [Clear All] [Compute Transform]            │
│                                                         │
│  ── Preview ──────────────────────────────────          │
│  [Toggle: Original ⟷ Normalized]                        │
│                                                         │
│  [Skip] [Save & Next]                                   │
└─────────────────────────────────────────────────────────┘
```

## Workflow Steps

1. **Startup**: User selects input/output directories
2. **Scan Discovery**: Tool finds all STLs, checks which have alignment JSONs already
3. **Auto-load First Unprocessed**: Shows mesh, prompts for landmark 1
4. **Point Picking**:
   - User clicks on mesh → seed point recorded
   - Dijkstra grows region around seed (using ToothSegmentation)
   - Region highlighted visually
   - Repeat for landmarks 2 and 3
5. **Compute Transform**: Button becomes active after 3 landmarks
6. **Preview**: Toggle shows mesh in normalized orientation
7. **Save & Next**: Writes JSON + normalized STL, auto-loads next unprocessed scan
8. **Review Mode**: Can browse already-processed scans for two-observer verification

## Coordinate System Computation (Math)

Given 3 landmark regions (midline, right, left):

1. **Collect vertices** from all 3 Dijkstra-grown regions
2. **Fit plane** (least-squares) through combined vertices → normal defines Z direction
3. **Y-axis**: Direction from midpoint of (right + left region centroids) toward midline centroid
4. **X-axis**: Z × Y (right-handed, perpendicular to Y, lying in fitted plane)
5. **Z-axis**: Plane normal, oriented so right-hand rule holds
6. **Origin**: Centroid of entire mesh (all vertices)
7. **Build 4x4 transform**: Rotation R = [X | Y | Z]^T, translation t = -R × origin

## Code Structure

```
~/claude-code/DentScanAlign/
├── CMakeLists.txt
├── CLAUDE.md
├── src/
│   ├── main.cpp
│   ├── core/                    # Copied from DentScanComparePro
│   │   ├── STLReader.{h,cpp}
│   │   ├── STLWriter.{h,cpp}    # New: write normalized STLs
│   │   ├── Mesh.h
│   │   └── ToothSegmentation.{h,cpp}
│   ├── alignment/               # New code
│   │   ├── CoordinateNormalizer.{h,cpp}
│   │   ├── LandmarkPlaneFitter.{h,cpp}
│   │   └── AlignmentSession.{h,cpp}
│   └── gui/
│       ├── MainWindow.{h,cpp}
│       ├── AlignmentWidget.{h,cpp}
│       └── MeshViewWidget.{h,cpp}
└── docs/
    ├── implementation-plan.md
    └── INITIAL-PROMPT.md
```

## Dependencies

- **Qt6**: Widgets
- **VTK 9.3**: Custom build from ~/VTK-install-linux
- **CGAL 6.0.1**: Surface mesh, property maps
- **Eigen 3.4**: Matrix math
- **nanoflann**: KD-tree for Dijkstra (header-only)

## Code Reuse

From `~/claude-code/DentScanComparePro/src/`:
- `core/STLReader.{h,cpp}` - Binary STL parser with winding correction
- `core/Mesh.h` - ScanData struct, SurfaceMesh type aliases
- `core/ToothSegmentation.{h,cpp}` - Dijkstra region growing from seeds

Adapt/simplify from DentScanComparePro:
- `visualization/VTKMeshWidget.{h,cpp}` - Base for MeshViewWidget

## Integration Path (Future)

Once DentScanAlign is working:
1. DentScanComparePro batch pipeline reads from `normalized/` directory
2. Or: merge alignment module back into DentScanComparePro as preprocessing step
3. JSON alignment files serve as audit trail and enable re-processing

## Notes

- **Lower jaw scans**: Same workflow applies. The "midline/right/left" terminology is just for consistent ordering, not anatomically strict.
- **Re-processing**: Delete output directory and re-run to redo alignments.
- **Two-observer control**: Review mode allows verification of existing alignments.
