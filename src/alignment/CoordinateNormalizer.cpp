// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CoordinateNormalizer.h"

namespace CoordinateNormalizer {

Eigen::Vector3d computeMeshCentroid(const SurfaceMesh& mesh)
{
    Eigen::Vector3d sum = Eigen::Vector3d::Zero();
    std::size_t count = 0;

    for (auto v : mesh.vertices()) {
        const Point3& p = mesh.point(v);
        sum += Eigen::Vector3d(CGAL::to_double(p.x()),
                               CGAL::to_double(p.y()),
                               CGAL::to_double(p.z()));
        ++count;
    }

    return count > 0 ? sum / static_cast<double>(count) : sum;
}

NormalizationResult computeTransform(
    const SurfaceMesh& mesh,
    const LandmarkPlaneFitter::PlaneResult& planeResult,
    JawType jawType)
{
    NormalizationResult result;

    if (!planeResult.valid) {
        result.valid = false;
        return result;
    }

    // Compute mesh centroid (this becomes the origin)
    Eigen::Vector3d centroid = computeMeshCentroid(mesh);
    result.meshCentroid = centroid;

    // Build rotation matrix from computed axes
    // R = [X | Y | Z]^T where X, Y, Z are column vectors in the original frame
    // This rotates original coordinates to the standard frame
    Eigen::Matrix3d R;
    R.row(0) = planeResult.xAxis.transpose();
    R.row(1) = planeResult.yAxis.transpose();
    R.row(2) = planeResult.zAxis.transpose();

    // Full transform: first translate to origin, then rotate
    // P' = R * (P - centroid) = R*P - R*centroid
    Eigen::Vector3d t = -R * centroid;

    // Build 4x4 matrix
    result.transform = Eigen::Matrix4d::Identity();
    result.transform.block<3,3>(0,0) = R;
    result.transform.block<3,1>(0,3) = t;

    // Apply jaw-type correction to reach the canonical frame (right-handed, det = +1).
    //
    // The base transform (from LandmarkPlaneFitter) delivers:
    //   Z = out of occlusal surface  (+Z for both jaws as scanned)
    //   Y = toward anterior (P1)
    //
    // Canonical target:
    //   Lower jaw  – Z = occlusal (+Z),  Y = posterior (+Y toward molars)
    //   Upper jaw  – Z = palate/root (+Z, occlusal at -Z),  Y = posterior
    //
    // Lower jaw: 180° around Z  → flips X and Y, keeps Z.
    //   Result: Y = posterior ✓, Z = occlusal ✓
    //
    // Upper jaw: 180° around X  → flips Y and Z.
    //   Result: Y = posterior ✓, Z = palate/root ✓ (occlusal at -Z)
    if (jawType == JawType::Lower) {
        Eigen::Matrix4d Rz180 = Eigen::Matrix4d::Identity();
        Rz180(0, 0) = -1.0;
        Rz180(1, 1) = -1.0;
        result.transform = Rz180 * result.transform;
    } else {
        Eigen::Matrix4d Rx180 = Eigen::Matrix4d::Identity();
        Rx180(1, 1) = -1.0;
        Rx180(2, 2) = -1.0;
        result.transform = Rx180 * result.transform;
    }

    result.valid = true;
    return result;
}

std::array<double, 3> transformPoint(const std::array<double, 3>& pt,
                                     const Eigen::Matrix4d& transform)
{
    Eigen::Vector4d p(pt[0], pt[1], pt[2], 1.0);
    Eigen::Vector4d tp = transform * p;
    return {tp[0], tp[1], tp[2]};
}

std::array<double, 16> matrixToArray(const Eigen::Matrix4d& mat)
{
    std::array<double, 16> arr;
    // Row-major order
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            arr[r * 4 + c] = mat(r, c);
        }
    }
    return arr;
}

Eigen::Matrix4d arrayToMatrix(const std::array<double, 16>& arr)
{
    Eigen::Matrix4d mat;
    // Row-major order
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            mat(r, c) = arr[r * 4 + c];
        }
    }
    return mat;
}

} // namespace CoordinateNormalizer
