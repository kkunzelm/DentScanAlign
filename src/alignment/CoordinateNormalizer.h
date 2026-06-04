#pragma once

#include "Mesh.h"
#include "LandmarkPlaneFitter.h"
#include <Eigen/Dense>
#include <array>

namespace CoordinateNormalizer {

struct NormalizationResult {
    Eigen::Matrix4d transform;       // Full 4x4 transformation matrix (row-major)
    Eigen::Vector3d meshCentroid;    // Centroid of entire mesh (origin for normalized system)
    bool valid = false;
};

// Computes the full 4x4 transformation matrix to normalize mesh coordinates.
// The transform:
//   1. Translates mesh so its centroid is at origin
//   2. Rotates so X/Y/Z axes align with the anatomical directions
//
// Target coordinate system:
//   X: Left (-) to Right (+)
//   Y: Posterior (-) to Anterior (+)
//   Z: Apical (-) to Occlusal (+)
NormalizationResult computeTransform(
    const SurfaceMesh& mesh,
    const LandmarkPlaneFitter::PlaneResult& planeResult);

// Compute the centroid of the entire mesh
Eigen::Vector3d computeMeshCentroid(const SurfaceMesh& mesh);

// Apply a 4x4 transform to a point
std::array<double, 3> transformPoint(const std::array<double, 3>& pt,
                                     const Eigen::Matrix4d& transform);

// Convert 4x4 matrix to row-major array (for JSON serialization)
std::array<double, 16> matrixToArray(const Eigen::Matrix4d& mat);

// Convert row-major array to 4x4 matrix
Eigen::Matrix4d arrayToMatrix(const std::array<double, 16>& arr);

} // namespace CoordinateNormalizer
