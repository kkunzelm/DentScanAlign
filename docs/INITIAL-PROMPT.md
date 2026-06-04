# Initial Prompt for Claude

Read this file and `implementation-plan.md` in this directory, then proceed with implementation.

## Task

Implement the **DentScanAlign** standalone tool as described in `implementation-plan.md`.

## Implementation Order

### Phase 1: Project Setup & Core Infrastructure
1. Create `CMakeLists.txt` with Qt6, VTK 9.3, CGAL 6.0.1, Eigen dependencies
2. Copy and adapt core files from `~/claude-code/DentScanComparePro/src/core/`:
   - `Mesh.h`
   - `STLReader.{h,cpp}`
   - `ToothSegmentation.{h,cpp}`
3. Create `STLWriter.{h,cpp}` for writing normalized STL files
4. Create basic `main.cpp` that compiles and links

### Phase 2: Alignment Math
1. Create `src/alignment/LandmarkPlaneFitter.{h,cpp}`:
   - Input: 3 sets of vertex positions (from Dijkstra regions)
   - Output: Fitted plane normal, axes (X, Y, Z), rotation matrix
2. Create `src/alignment/CoordinateNormalizer.{h,cpp}`:
   - Compute full 4x4 transformation matrix
   - Apply transform to mesh vertices
   - Handle mesh centroid calculation

### Phase 3: Session Management
1. Create `src/alignment/AlignmentSession.{h,cpp}`:
   - Scan input directory recursively for STL files
   - Track which scans have alignment JSONs already
   - Provide next-unprocessed-scan iteration
   - Save/load JSON alignment files

### Phase 4: GUI
1. Create `src/gui/MeshViewWidget.{h,cpp}`:
   - VTK-based 3D mesh rendering
   - Point picking (click → 3D coordinate)
   - Highlight picked regions
   - Toggle between original and transformed view
2. Create `src/gui/AlignmentWidget.{h,cpp}`:
   - Landmark status indicators
   - Undo/Clear/Compute buttons
   - Preview toggle
3. Create `src/gui/MainWindow.{h,cpp}`:
   - Directory selection
   - Progress display
   - Orchestrate workflow

### Phase 5: Integration & Testing
1. Wire up complete workflow
2. Test with sample STL files from `~/claude-code/match3d-plus/data/3d-data/stl/`
3. Verify JSON output and normalized STL output

## Key Technical Notes

- **VTK custom build**: Use `~/VTK-install-linux/lib/cmake/vtk-9.3`
- **CGAL 6.0.1 property maps**: Use `std::optional` API, not `std::pair`
- **Dijkstra segmentation**: ToothSegmentation uses geodesic distance with curvature-weighted edges
- **STL winding**: STLReader auto-corrects per-face; no config flag needed
- **Platform**: Debian 13 / Linux

## Source Files to Reference

When copying/adapting code, look at:
- `/home/kkunzelm/claude-code/DentScanComparePro/src/core/STLReader.cpp` (148 lines)
- `/home/kkunzelm/claude-code/DentScanComparePro/src/core/Mesh.h`
- `/home/kkunzelm/claude-code/DentScanComparePro/src/core/ToothSegmentation.{h,cpp}`
- `/home/kkunzelm/claude-code/DentScanComparePro/src/visualization/VTKMeshWidget.{h,cpp}` (for VTK patterns)
- `/home/kkunzelm/claude-code/DentScanComparePro/src/qc/LandmarkRegistration.cpp` (Kabsch algorithm reference)

## Build Commands

```bash
cd ~/claude-code/DentScanAlign
mkdir build && cd build
cmake ..
make -j$(nproc)
./DentScanAlign
```

## Test Data

Sample STL files are in:
- `/home/kkunzelm/claude-code/match3d-plus/data/3d-data/stl/`

Start with a small subset for testing.
