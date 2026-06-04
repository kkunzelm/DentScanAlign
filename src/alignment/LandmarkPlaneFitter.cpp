// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LandmarkPlaneFitter.h"

#include <Eigen/Eigenvalues>

namespace LandmarkPlaneFitter {

namespace {

Eigen::Vector3d toEigen(const Point3& p)
{
    return Eigen::Vector3d(CGAL::to_double(p.x()),
                           CGAL::to_double(p.y()),
                           CGAL::to_double(p.z()));
}

Eigen::Vector3d computeCentroid(const SurfaceMesh& mesh,
                                const std::vector<VertexDesc>& vertices)
{
    Eigen::Vector3d sum = Eigen::Vector3d::Zero();
    for (auto v : vertices) {
        sum += toEigen(mesh.point(v));
    }
    return vertices.empty() ? sum : sum / static_cast<double>(vertices.size());
}

} // anonymous namespace

PlaneResult fitPlane(
    const SurfaceMesh& mesh,
    const std::vector<VertexDesc>& midlineVertices,
    const std::vector<VertexDesc>& rightVertices,
    const std::vector<VertexDesc>& leftVertices)
{
    PlaneResult result;

    // Combine all vertices from 3 regions
    std::vector<Eigen::Vector3d> allPoints;
    allPoints.reserve(midlineVertices.size() + rightVertices.size() + leftVertices.size());

    for (auto v : midlineVertices)
        allPoints.push_back(toEigen(mesh.point(v)));
    for (auto v : rightVertices)
        allPoints.push_back(toEigen(mesh.point(v)));
    for (auto v : leftVertices)
        allPoints.push_back(toEigen(mesh.point(v)));

    if (allPoints.size() < 3) {
        result.valid = false;
        return result;
    }

    // Compute centroid of all points
    Eigen::Vector3d centroid = Eigen::Vector3d::Zero();
    for (const auto& p : allPoints)
        centroid += p;
    centroid /= static_cast<double>(allPoints.size());
    result.centroid = centroid;

    // Build covariance matrix for PCA
    Eigen::Matrix3d cov = Eigen::Matrix3d::Zero();
    for (const auto& p : allPoints) {
        Eigen::Vector3d d = p - centroid;
        cov += d * d.transpose();
    }

    // Eigendecomposition - smallest eigenvalue corresponds to plane normal
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(cov);
    Eigen::Vector3d normal = solver.eigenvectors().col(0); // smallest eigenvalue

    // Compute region centroids
    // User clicks: Point1 (midline/anterior), Point2, Point3 in CLOCKWISE order
    // when viewing from occlusal (top) direction
    Eigen::Vector3d p1Centroid = computeCentroid(mesh, midlineVertices);  // midline/anterior
    Eigen::Vector3d p2Centroid = computeCentroid(mesh, rightVertices);    // clockwise from midline
    Eigen::Vector3d p3Centroid = computeCentroid(mesh, leftVertices);     // clockwise from p2

    // Z-axis orientation from clockwise convention:
    // If points are clicked clockwise when viewed from above (occlusal),
    // then (P2-P1) × (P3-P1) points DOWN (into mouth).
    // Align fitted normal with clockwise cross product direction.
    Eigen::Vector3d v1 = p2Centroid - p1Centroid;
    Eigen::Vector3d v2 = p3Centroid - p1Centroid;
    Eigen::Vector3d clockwiseNormal = v1.cross(v2);

    // Align normal with clockwiseNormal direction
    if (clockwiseNormal.dot(normal) < 0) {
        normal = -normal;
    }

    // X-axis: direction from P2 to P3, projected onto fitted plane
    // This makes X parallel to the line between points 2 and 3
    // Convention: X points left(-) to right(+), so from P2 to P3
    Eigen::Vector3d xDirection = p3Centroid - p2Centroid;
    xDirection = xDirection - normal * (xDirection.dot(normal));
    double xLen = xDirection.norm();
    if (xLen < 1e-9) {
        result.valid = false;
        return result;
    }
    Eigen::Vector3d xAxis = xDirection / xLen;

    // Y-axis: perpendicular to X in the plane, pointing toward P1 (anterior)
    // Y = Z × X (right-hand rule: if Z up and X right, Y is forward)
    Eigen::Vector3d yAxis = normal.cross(xAxis);

    // Ensure Y points toward P1 (anterior), not away
    Eigen::Vector3d backMidpoint = (p2Centroid + p3Centroid) * 0.5;
    Eigen::Vector3d towardP1 = p1Centroid - backMidpoint;
    if (yAxis.dot(towardP1) < 0) {
        yAxis = -yAxis;
        xAxis = -xAxis;  // flip X too to maintain right-hand rule
    }

    result.normal = normal;
    result.xAxis = xAxis;
    result.yAxis = yAxis;
    result.zAxis = normal;
    result.valid = true;

    return result;
}

} // namespace LandmarkPlaneFitter
