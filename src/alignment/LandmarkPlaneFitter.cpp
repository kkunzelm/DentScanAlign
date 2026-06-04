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
    Eigen::Vector3d midlineCentroid = computeCentroid(mesh, midlineVertices);
    Eigen::Vector3d rightCentroid = computeCentroid(mesh, rightVertices);
    Eigen::Vector3d leftCentroid = computeCentroid(mesh, leftVertices);

    // Y-axis: direction from midpoint of (right, left) toward midline
    Eigen::Vector3d sidesMidpoint = (rightCentroid + leftCentroid) * 0.5;
    Eigen::Vector3d yDirection = midlineCentroid - sidesMidpoint;

    // Project Y onto the fitted plane (remove component along normal)
    yDirection = yDirection - normal * (yDirection.dot(normal));
    double yLen = yDirection.norm();
    if (yLen < 1e-9) {
        result.valid = false;
        return result;
    }
    Eigen::Vector3d yAxis = yDirection / yLen;

    // X-axis: perpendicular to both Y and Z, in the plane
    // X = Z × Y gives left-to-right direction if Z is up and Y is forward
    Eigen::Vector3d xAxis = normal.cross(yAxis);
    xAxis.normalize();

    // Ensure Z points in the "occlusal" direction (positive Z should be away from mouth)
    // Convention: if right is to the left of left (negative X direction), flip Z
    Eigen::Vector3d rightDir = rightCentroid - leftCentroid;
    if (rightDir.dot(xAxis) < 0) {
        normal = -normal;
        xAxis = -xAxis;
    }

    result.normal = normal;
    result.xAxis = xAxis;
    result.yAxis = yAxis;
    result.zAxis = normal;
    result.valid = true;

    return result;
}

} // namespace LandmarkPlaneFitter
