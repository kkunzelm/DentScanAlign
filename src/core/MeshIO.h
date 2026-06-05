// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Mesh.h"
#include <Eigen/Dense>
#include <memory>
#include <string>

namespace MeshIO {

enum class MeshFormat {
    Unknown,
    STL,
    PLY,
    OBJ
};

MeshFormat formatFromPath(const std::string& filePath);
std::string extensionForFormat(MeshFormat format);
bool isSupportedMeshFile(const std::string& filePath);

// Reads STL, PLY, or OBJ into ScanData. STL keeps the existing specialised
// reader; PLY/OBJ use CGAL polygon mesh I/O for display and landmark picking.
std::shared_ptr<ScanData> read(const std::string& filePath, std::string& errorMsg);

// Writes a transformed scan. When saveAsStl is true, outputPath is written as
// binary STL. Otherwise the input file format is preserved when possible:
// OBJ and PLY are transformed in-place at file-record level so material refs,
// vertex colors, texture coordinates, comments and custom properties are kept.
bool writeTransformed(const ScanData& scan,
                      const Eigen::Matrix4d& transform,
                      const std::string& outputPath,
                      bool saveAsStl,
                      std::string& errorMsg);

} // namespace MeshIO
