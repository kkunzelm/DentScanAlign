# DentScanAlign User Manual

**A Guide for Dental Researchers**

## Introduction

### Why Coordinate Normalization?

When comparing intraoral scans from different scanners or scanning sessions, each scanner produces mesh files in its own arbitrary coordinate system. One scanner might place the occlusal surface facing up, another facing sideways. This makes direct comparison impossible.

**DentScanAlign solves this problem** by transforming all scans into a standardized anatomical coordinate system. After normalization, all scans share a consistent right-handed coordinate frame:

- The right side of the arch points in the positive X direction; X = 0 is the sagittal symmetry plane
- The mesh is centered at the origin
- For **lower jaw** scans: the occlusal surface faces **+Z**
- For **upper jaw** scans: the occlusal surface faces **−Z**; the palate/root side faces **+Z**

This jaw-aware convention ensures that upper and lower jaw scans can be compared or registered consistently without manual flipping.

This standardization is essential before:
- ICP (Iterative Closest Point) registration
- Trueness and precision measurements
- Statistical comparison of scanner accuracy

### The Complete Workflow

```
Step 1: Scan collection
   └── Multiple scanners produce STL, PLY, or OBJ files in different orientations

Step 2: DentScanAlign (this tool)
   └── Interactive landmark placement normalizes all scans

Step 3: DentScanComparePro
   └── Batch ICP registration computes trueness/precision metrics

Step 4: Statistical analysis (R, SPSS)
   └── CSV export enables publication-ready statistics
```

## Getting Started

### Organizing Your Study Data

Before using DentScanAlign, organize your scan files properly. A well-structured folder hierarchy makes batch processing efficient and reduces errors.

**Recommended folder structure:**

```
P2024-MyStudy/
├── raw_scans/                    # Input for DentScanAlign
│   ├── Scanner_A/
│   │   ├── specimen_01/
│   │   │   ├── scan_1.stl
│   │   │   ├── scan_2.ply
│   │   │   └── scan_3.obj
│   │   ├── specimen_02/
│   │   │   └── ...
│   │   └── specimen_03/
│   │       └── ...
│   ├── Scanner_B/
│   │   └── ...
│   └── Scanner_C/
│       └── ...
│
├── normalized/                   # Output from DentScanAlign
│   ├── alignments/               # JSON metadata (auto-created)
│   └── normalized/               # Transformed mesh files (auto-created)
│
└── results/                      # Output from DentScanComparePro
    ├── registered/
    └── metrics.csv
```

**Naming conventions:**

- Use descriptive folder names: `Scanner_A`, `Scanner_B` (or actual scanner names like `Trios4`, `Primescan`)
- Number specimens consistently: `specimen_01`, `specimen_02`, ...
- Number repeated scans consistently, for example `scan_1.stl`, `scan_2.ply`, `scan_3.obj`, ...
- Avoid spaces in filenames (use underscores instead)

### Launching the Application

```bash
cd ~/claude-code/DentScanAlign/build
./DentScanAlign
```

The main window appears with:
- A **Batch processing** area for whole input/output directories
- A **Single file processing** area for one manually selected scan
- A shared **Save as STL** checkbox
- A 3D mesh viewer in the center
- Alignment controls on the right, including **jaw type selection** and landmark indicators
- A progress/status indicator below the file selection areas

## Step-by-Step Workflow

DentScanAlign now supports two workflows:

- **Batch processing** for a complete study directory with many scans
- **Single file processing** for manually opening and saving one scan

### Batch Processing Workflow

### Step 1: Select Directories

1. Click **"Browse..."** next to **Input** and select your `raw_scans` folder
2. Click **"Browse..."** next to **Output** and select your `normalized` folder
3. The paths are remembered between sessions

### Step 2: Choose Output Format and Start Processing

Before starting, choose how normalized meshes should be saved:

- Leave **"Save as STL"** unchecked to keep each file in its original format (`.stl` stays `.stl`, `.ply` stays `.ply`, `.obj` stays `.obj`)
- Check **"Save as STL"** to save every normalized scan as an STL file, regardless of the input format

Click **"Start Processing"**. The application will:
- Scan all subdirectories for supported mesh files (`.stl`, `.ply`, `.obj`)
- Skip any files that already have alignment data
- Load the first unprocessed scan
- Display the mesh in the 3D viewer

### Step 3: Navigate the 3D View

**Mouse controls:**
- **Left-click + drag**: Rotate the view
- **Right-click + drag** or **scroll wheel**: Zoom in/out
- **Middle-click + drag**: Pan the view
- **Single left-click** (without dragging): Place a landmark

**Tip:** Rotate the mesh so you can see the occlusal surface clearly before placing landmarks.

### Step 4: Select the Jaw Type

Before placing landmarks, select whether the current scan is a **lower jaw** or **upper jaw** using the radio buttons at the top of the right panel.

- **Lower jaw** (default): the occlusal surface will be oriented toward +Z in the output
- **Upper jaw**: the occlusal surface will be oriented toward −Z; the palate/root side toward +Z

This selection can be changed at any time before clicking "Save & Next". If you are processing a batch that contains both jaw types, remember to update this before placing landmarks on each scan.

### Step 5: Place Three Landmarks

You need to place exactly 3 points that define the anatomical orientation. The points should be placed **clockwise** when viewing the occlusal surface from above.

#### Point 1: Anterior Midline

Click on a point at the **anterior midline** of the dental arch. Good choices:
- Contact point between central incisors
- Center of the incisal edge of a central incisor
- Any clearly identifiable midline structure

**Why this point?** It defines the anterior direction.

#### Point 2: Right Side

Click on a point on the **right side** of the arch. Good choices:
- Cusp tip of a right molar or premolar
- Any clearly identifiable right-side structure

**Why this point?** Together with Point 3, it defines the left-right (X) axis.

#### Point 3: Left Side

Click on a point on the **left side** of the arch. This should be roughly symmetric to Point 2.

**Why this point?** The plane through all three points becomes the occlusal plane (XY plane).

### Step 6: Review the Preview

After placing the third point, the application automatically:
1. Grows small regions around each landmark
2. Fits a plane through all points
3. Computes the transformation matrix
4. Shows a preview of the normalized mesh

The preview shows the mesh in the standardized orientation:
- Occlusal surface toward the viewer (for lower jaw: +Z forward; for upper jaw: −Z forward)
- Anterior pointing up in the view
- Right side on the right

**If the preview looks wrong:**
- Verify that the correct jaw type (lower/upper) is selected
- Click **"Undo Last"** to remove the last landmark
- Click **"Reset"** to clear all landmarks and start over
- Re-place the landmarks more carefully

### Step 7: Save and Continue

If the preview looks correct, click **"Save & Next"**. The application:
1. Saves the transformation (including jaw type) to a JSON file
2. Writes the normalized mesh file in the selected output format
3. Automatically loads the next unprocessed scan

**Keyboard shortcut:** After placing 3 landmarks, the "Save & Next" button is focused. Press **Enter** to save quickly.

### Step 8: Skip Problematic Scans

If a scan is corrupted, has artifacts, or cannot be properly aligned:
- Click **"Skip"** to move to the next scan without saving
- The skipped scan will appear again if you restart processing

### Step 9: Completion

When all scans are processed, a message appears showing the total count. The progress bar shows 100%.


### Single File Processing Workflow

Use single file processing when you want to normalize one scan manually instead of processing a whole study folder.

### Step 1: Select One Input File

1. In the **Single file processing** area, click **"Browse..."** next to **Input file**
2. Select one `.stl`, `.ply`, or `.obj` file
3. The program suggests an output filename with `_normalized` added to the original name

Example:

```
scan_01.ply  →  scan_01_normalized.ply
```

### Step 2: Choose the Output File

1. Click **"Browse..."** next to **Output file**
2. Choose where the transformed mesh should be saved
3. The output file must be different from the input file, so the original scan is not overwritten

The **Save as STL** checkbox also applies here:

- If **"Save as STL"** is unchecked, the output keeps the input format
- If **"Save as STL"** is checked, the output filename is changed to `.stl`

Examples:

```
Input:  scan_01.ply
Output with Save as STL unchecked: scan_01_normalized.ply
Output with Save as STL checked:   scan_01_normalized.stl
```

### Step 3: Open, Align, and Save

Click **"Open Single File"**. The scan is loaded into the 3D viewer. Place the three landmarks and review the preview exactly as in batch mode.

When the preview looks correct, click **"Save & Next"**. In single file mode this saves the selected file and shows a confirmation message. It does not automatically load another scan.

A JSON alignment metadata file is saved next to the output mesh, using the same filename base.

Example:

```
scan_01_normalized.ply
scan_01_normalized.json
```

## Understanding the Output

### Normalized Mesh Files

Batch mode location: `output_directory/normalized/`

Single file mode location: the output file path selected manually by the user

These are the original meshes transformed to the standard coordinate system. The vertex positions are modified, but the mesh topology is kept.

DentScanAlign can process `.stl`, `.ply`, and `.obj` files. When **"Save as STL"** is unchecked, the normalized file keeps the same format as the input file. When **"Save as STL"** is checked, every normalized file is saved as `.stl`.

For PLY and OBJ files, the application tries to preserve extra information such as colors, comments, material references, groups, texture coordinates, and other file records when saving in the same format. If normals need to be updated after the transformation, the program recomputes or updates them as needed.

**Use these files for:**
- Visual inspection in any 3D viewer
- ICP registration in DentScanComparePro
- Any analysis requiring consistent orientation

### Alignment JSON Files

Batch mode location: `output_directory/alignments/`

Single file mode location: next to the selected output mesh file

Each JSON file contains:

```json
{
  "source_file": "Scanner_A/specimen_01/scan_1.stl",
  "jaw_type": "lower",
  "landmarks": {
    "midline": {"seed": [x, y, z], "vertex_count": 150},
    "right": {"seed": [x, y, z], "vertex_count": 148},
    "left": {"seed": [x, y, z], "vertex_count": 152}
  },
  "fitted_plane_normal": [nx, ny, nz],
  "computed_axes": {
    "x": [xx, xy, xz],
    "y": [yx, yy, yz],
    "z": [zx, zy, zz]
  },
  "mesh_centroid": [cx, cy, cz],
  "transform_4x4": [16 values, row-major],
  "timestamp": "2024-03-15T14:30:00"
}
```

**Use these files for:**
- Reproducibility documentation
- Verifying landmark placement
- Re-applying transformations if needed
- Quality control audits

## Tips for Accurate Alignment

### Choosing Good Landmarks

**Do:**
- Pick points on clearly defined anatomical structures
- Use cusp tips, contact points, or central fossae
- Place points at similar heights (on the occlusal plane)
- Be consistent across all scans in your study

**Don't:**
- Pick points on smooth, featureless surfaces
- Use points on the gingiva or soft tissue
- Place points too close together
- Rush - accuracy matters more than speed

### Handling Different Arch Types

**Full arch scans:**
- Point 1: Central incisor contact
- Point 2: Right first molar mesial cusp
- Point 3: Left first molar mesial cusp

**Partial arch / quadrant scans:**
- Choose three well-separated points
- Ensure they roughly define a plane parallel to the occlusal surface

**Single tooth preparations:**
- Not recommended for this tool
- Consider manual alignment in CAD software

### Quality Control

After processing a batch or an important single file:
1. Open the normalized mesh files in a 3D viewer
2. Verify they all have consistent orientation within the same jaw type
3. For lower jaw scans: confirm the occlusal surface faces +Z (upward)
4. For upper jaw scans: confirm the occlusal surface faces −Z (downward) and the palate faces +Z
5. Confirm the right side of the arch is in the positive X direction

If inconsistencies are found:
1. Delete the problematic JSON and normalized mesh files
2. Restart DentScanAlign
3. Re-process only the affected scans

## Troubleshooting

### "No supported mesh files found"

- Check that the input directory contains `.stl`, `.ply`, or `.obj` files
- Files may be in subdirectories (the tool searches recursively)
- File extensions may be uppercase or lowercase, such as `.STL`, `.PLY`, or `.OBJ`

### Mesh appears black or invisible

- The mesh may be very small or very large
- Use scroll wheel to zoom out significantly
- Check if the mesh file is valid in another viewer

### Landmarks don't appear where clicked

- On high-DPI displays, there may be offset issues
- Try clicking slightly away from your target
- Ensure you're clicking on the mesh surface, not empty space

### Progress bar doesn't advance

- This was fixed in version 1.0
- Ensure you're using the latest build
- Rebuild with `make -j$(nproc)`


### Single file does not open

- Check that the selected file is `.stl`, `.ply`, or `.obj`
- Try opening the same file in another 3D viewer to confirm that it is valid
- Make sure the file path is readable and does not point to a directory

### Output file changes extension automatically

This is expected. The program protects the selected output format:

- With **"Save as STL"** checked, the output extension is changed to `.stl`
- With **"Save as STL"** unchecked, the output extension follows the input file format

### Program warns that input and output are the same file

Choose a different output filename. DentScanAlign does not overwrite the original input scan in single file mode.

### Transform preview looks wrong

- Confirm the correct jaw type (Lower/Upper) is selected before placing landmarks
- Verify landmarks are placed clockwise when viewing the occlusal surface from above
- Point 1 should be anterior (midline), Points 2-3 should be on the right and left posterior sides
- Use "Reset" and try again with clearer landmark positions

## Frequently Asked Questions


**Q: Can I process just one file without preparing a study directory?**

A: Yes. Use the **Single file processing** section. Select one input mesh file, choose the exact output file path, click **"Open Single File"**, place the landmarks, and save.

**Q: Can I re-process a scan that was already aligned?**

A: Yes. Delete the corresponding JSON file from `alignments/` and restart processing. The scan will appear again.

**Q: What if I need to process thousands of scans?**

A: DentScanAlign is designed for interactive batch processing. For very large studies, consider:
- Processing in sessions (the tool remembers progress)
- Having multiple operators share the workload
- Using the JSON files to verify inter-operator consistency

**Q: Can I use this for mandibular (lower) arches?**

A: Yes. Select **"Lower jaw"** in the jaw type panel (it is the default). Place Point 1 at the anterior midline, Points 2-3 on the right and left sides, clockwise when viewing the occlusal surface from above. The output will have the occlusal surface facing +Z.

**Q: Can I use this for maxillary (upper) arches?**

A: Yes. Select **"Upper jaw"** in the jaw type panel before placing landmarks. The landmark picking procedure is the same as for lower jaw scans. After alignment, the occlusal surface faces −Z and the palate/root side faces +Z, matching the canonical convention for upper jaw scans.

**Q: What coordinate units does the output use?**

A: The same units as the input mesh files (typically millimeters for dental scanners). The transformation only rotates and translates; it does not scale.


**Q: Should I use "Save as STL"?**

A: Use it when your downstream software requires STL files. Leave it unchecked if you want to keep PLY colors, OBJ material references, texture coordinates, or other format-specific information.

**Q: Can I undo after saving?**

A: Not within the application. To redo a scan:
1. Delete its JSON file from `alignments/`
2. Delete its normalized mesh file from `normalized/`
3. Restart and re-process

## Contact

For questions or support, visit [www.kunzelmann.de](https://www.kunzelmann.de)

---

*DentScanAlign - Coordinate Normalization Tool*
*Prof. Dr. Karl-Heinz Kunzelmann*
