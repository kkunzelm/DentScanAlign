// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Mesh.h"
#include <Eigen/Dense>
#include <array>
#include <vector>

namespace LandmarkPlaneFitter {

struct PlaneResult {
    Eigen::Vector3d normal;     // Fitted plane normal (Z-axis direction)
    Eigen::Vector3d centroid;   // Centroid of all region vertices
    Eigen::Vector3d xAxis;      // Computed X-axis (left-right)
    Eigen::Vector3d yAxis;      // Computed Y-axis (posterior-anterior)
    Eigen::Vector3d zAxis;      // Computed Z-axis (apical-occlusal) = normal
    bool valid = false;
};

// Fits a plane through vertices from 3 landmark regions.
// Input: 3 sets of vertices (midline, right, left regions grown from seeds)
// The axes are computed as:
//   - Z: least-squares plane normal
//   - Y: direction from midpoint(right,left) toward midline centroid
//   - X: Z × Y (perpendicular in plane)
PlaneResult fitPlane(
    const SurfaceMesh& mesh,
    const std::vector<VertexDesc>& midlineVertices,
    const std::vector<VertexDesc>& rightVertices,
    const std::vector<VertexDesc>& leftVertices);

} // namespace LandmarkPlaneFitter
