// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Mesh.h"
#include <string>

namespace STLWriter {

// Writes a binary STL file from the mesh data.
// If a transform matrix is provided (not identity), vertices are transformed before writing.
// Returns true on success, false on error (errorMsg is set accordingly).
bool write(const ScanData& scan,
           const std::string& outputPath,
           std::string& errorMsg);

// Writes a binary STL with an explicit 4x4 transformation applied to vertices.
bool writeTransformed(const ScanData& scan,
                      const Eigen::Matrix4d& transform,
                      const std::string& outputPath,
                      std::string& errorMsg);

} // namespace STLWriter
