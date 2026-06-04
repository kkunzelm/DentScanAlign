// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "STLWriter.h"

#include <Eigen/Geometry>

#include <cstring>
#include <filesystem>
#include <fstream>

namespace STLWriter {

namespace {

// Transform a CGAL Point3 using an Eigen 4x4 matrix
Eigen::Vector3d transformPoint(const Point3& p, const Eigen::Matrix4d& T)
{
    Eigen::Vector4d v(CGAL::to_double(p.x()),
                      CGAL::to_double(p.y()),
                      CGAL::to_double(p.z()),
                      1.0);
    Eigen::Vector4d vt = T * v;
    return Eigen::Vector3d(vt[0], vt[1], vt[2]);
}

// Compute face normal from 3 transformed vertices
Eigen::Vector3f computeNormal(const Eigen::Vector3d& v0,
                               const Eigen::Vector3d& v1,
                               const Eigen::Vector3d& v2)
{
    Eigen::Vector3d e1 = v1 - v0;
    Eigen::Vector3d e2 = v2 - v0;
    Eigen::Vector3d n = e1.cross(e2);
    double len = n.norm();
    if (len > 1e-12)
        n /= len;
    return n.cast<float>();
}

} // anonymous namespace

bool write(const ScanData& scan,
           const std::string& outputPath,
           std::string& errorMsg)
{
    return writeTransformed(scan, Eigen::Matrix4d::Identity(), outputPath, errorMsg);
}

bool writeTransformed(const ScanData& scan,
                      const Eigen::Matrix4d& transform,
                      const std::string& outputPath,
                      std::string& errorMsg)
{
    const SurfaceMesh& mesh = scan.mesh;

    if (mesh.is_empty()) {
        errorMsg = "Cannot write empty mesh";
        return false;
    }

    // Ensure parent directory exists
    std::filesystem::path outPath(outputPath);
    if (outPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(outPath.parent_path(), ec);
        if (ec) {
            errorMsg = "Cannot create output directory: " + ec.message();
            return false;
        }
    }

    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        errorMsg = "Cannot open output file: " + outputPath;
        return false;
    }

    // Write 80-byte header
    char header[80] = {};
    std::string headerStr = "DentScanAlign normalized STL";
    std::memcpy(header, headerStr.c_str(), std::min(headerStr.size(), size_t(79)));
    file.write(header, 80);

    // Write triangle count
    uint32_t nTriangles = static_cast<uint32_t>(mesh.number_of_faces());
    file.write(reinterpret_cast<const char*>(&nTriangles), 4);

    // Write triangles
    for (auto f : mesh.faces()) {
        // Get face vertices
        auto h = mesh.halfedge(f);
        std::array<VertexDesc, 3> vs;
        int i = 0;
        for (auto vd : mesh.vertices_around_face(h))
            vs[i++] = vd;

        // Transform vertices
        Eigen::Vector3d v0 = transformPoint(mesh.point(vs[0]), transform);
        Eigen::Vector3d v1 = transformPoint(mesh.point(vs[1]), transform);
        Eigen::Vector3d v2 = transformPoint(mesh.point(vs[2]), transform);

        // Compute normal for transformed face
        Eigen::Vector3f normal = computeNormal(v0, v1, v2);

        // Write triangle data: normal (3 floats) + 3 vertices (9 floats) + attr (uint16)
        float buf[12];
        buf[0] = normal.x();
        buf[1] = normal.y();
        buf[2] = normal.z();
        buf[3] = static_cast<float>(v0.x());
        buf[4] = static_cast<float>(v0.y());
        buf[5] = static_cast<float>(v0.z());
        buf[6] = static_cast<float>(v1.x());
        buf[7] = static_cast<float>(v1.y());
        buf[8] = static_cast<float>(v1.z());
        buf[9] = static_cast<float>(v2.x());
        buf[10] = static_cast<float>(v2.y());
        buf[11] = static_cast<float>(v2.z());

        file.write(reinterpret_cast<const char*>(buf), 48);

        uint16_t attr = 0;
        file.write(reinterpret_cast<const char*>(&attr), 2);
    }

    if (!file) {
        errorMsg = "Error writing to file";
        return false;
    }

    return true;
}

} // namespace STLWriter
