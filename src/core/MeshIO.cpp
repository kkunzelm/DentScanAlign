// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MeshIO.h"
#include "STLReader.h"
#include "STLWriter.h"

#include <CGAL/IO/polygon_mesh_io.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace MeshIO {

namespace {

std::string lowerExtension(const std::string& filePath)
{
    std::string ext = std::filesystem::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::string extractScannerName(const std::string& filePath)
{
    std::filesystem::path p(filePath);
    std::string stem = p.stem().string();
    std::istringstream ss(stem);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token, '_'))
        tokens.push_back(token);
    return tokens.size() >= 2 ? tokens[1] : stem;
}

void updateStats(ScanData& scan)
{
    scan.triangleCount = scan.mesh.number_of_faces();

    bool first = true;
    for (auto v : scan.mesh.vertices()) {
        const Point3& p = scan.mesh.point(v);
        const double x = CGAL::to_double(p.x());
        const double y = CGAL::to_double(p.y());
        const double z = CGAL::to_double(p.z());
        if (first) {
            scan.boundsMin = {x, y, z};
            scan.boundsMax = {x, y, z};
            first = false;
        } else {
            scan.boundsMin[0] = std::min(scan.boundsMin[0], x);
            scan.boundsMin[1] = std::min(scan.boundsMin[1], y);
            scan.boundsMin[2] = std::min(scan.boundsMin[2], z);
            scan.boundsMax[0] = std::max(scan.boundsMax[0], x);
            scan.boundsMax[1] = std::max(scan.boundsMax[1], y);
            scan.boundsMax[2] = std::max(scan.boundsMax[2], z);
        }
    }
}

Eigen::Vector3d transformPoint(double x, double y, double z, const Eigen::Matrix4d& T)
{
    Eigen::Vector4d p(x, y, z, 1.0);
    Eigen::Vector4d out = T * p;
    return {out.x(), out.y(), out.z()};
}

Eigen::Vector3d transformNormal(double x, double y, double z, const Eigen::Matrix4d& T)
{
    Eigen::Matrix3d normalMatrix = T.block<3, 3>(0, 0).inverse().transpose();
    Eigen::Vector3d n = normalMatrix * Eigen::Vector3d(x, y, z);
    const double len = n.norm();
    if (len > 1e-12)
        n /= len;
    return n;
}

bool writeParentDirectories(const std::string& outputPath, std::string& errorMsg)
{
    std::filesystem::path out(outputPath);
    if (!out.has_parent_path())
        return true;

    std::error_code ec;
    std::filesystem::create_directories(out.parent_path(), ec);
    if (ec) {
        errorMsg = "Cannot create output directory: " + ec.message();
        return false;
    }
    return true;
}

bool transformObjPreservingRecords(const std::string& inputPath,
                                   const Eigen::Matrix4d& transform,
                                   const std::string& outputPath,
                                   std::string& errorMsg)
{
    if (!writeParentDirectories(outputPath, errorMsg))
        return false;

    std::ifstream in(inputPath);
    if (!in) {
        errorMsg = "Cannot open OBJ file: " + inputPath;
        return false;
    }

    std::ofstream out(outputPath);
    if (!out) {
        errorMsg = "Cannot write OBJ file: " + outputPath;
        return false;
    }

    out << std::setprecision(17);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;

        if (tag == "v") {
            double x = 0, y = 0, z = 0;
            if (ls >> x >> y >> z) {
                Eigen::Vector3d p = transformPoint(x, y, z, transform);
                std::string rest;
                std::getline(ls, rest);
                out << "v " << p.x() << ' ' << p.y() << ' ' << p.z() << rest << '\n';
                continue;
            }
        } else if (tag == "vn") {
            double x = 0, y = 0, z = 0;
            if (ls >> x >> y >> z) {
                Eigen::Vector3d n = transformNormal(x, y, z, transform);
                std::string rest;
                std::getline(ls, rest);
                out << "vn " << n.x() << ' ' << n.y() << ' ' << n.z() << rest << '\n';
                continue;
            }
        }

        out << line << '\n';
    }

    if (!out) {
        errorMsg = "Error writing OBJ file: " + outputPath;
        return false;
    }
    return true;
}

template <typename T>
T readScalar(std::istream& in, bool bigEndian)
{
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (bigEndian) {
        auto* p = reinterpret_cast<unsigned char*>(&value);
        std::reverse(p, p + sizeof(T));
    }
    return value;
}

template <typename T>
void writeScalar(std::ostream& out, T value, bool bigEndian)
{
    if (bigEndian) {
        auto* p = reinterpret_cast<unsigned char*>(&value);
        std::reverse(p, p + sizeof(T));
    }
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

struct PlyProperty {
    std::string type;
    std::string name;
    bool isList = false;
    std::string countType;
    std::string itemType;
};

struct PlyElement {
    std::string name;
    std::size_t count = 0;
    std::vector<PlyProperty> properties;
};

std::size_t scalarSize(const std::string& type)
{
    if (type == "char" || type == "uchar" || type == "int8" || type == "uint8") return 1;
    if (type == "short" || type == "ushort" || type == "int16" || type == "uint16") return 2;
    if (type == "int" || type == "uint" || type == "float" || type == "int32" || type == "uint32" || type == "float32") return 4;
    if (type == "double" || type == "float64") return 8;
    throw std::runtime_error("Unsupported PLY property type: " + type);
}

double readPlyNumber(std::istream& in, const std::string& type, bool bigEndian)
{
    if (type == "char" || type == "int8") return readScalar<int8_t>(in, bigEndian);
    if (type == "uchar" || type == "uint8") return readScalar<uint8_t>(in, bigEndian);
    if (type == "short" || type == "int16") return readScalar<int16_t>(in, bigEndian);
    if (type == "ushort" || type == "uint16") return readScalar<uint16_t>(in, bigEndian);
    if (type == "int" || type == "int32") return readScalar<int32_t>(in, bigEndian);
    if (type == "uint" || type == "uint32") return readScalar<uint32_t>(in, bigEndian);
    if (type == "float" || type == "float32") return readScalar<float>(in, bigEndian);
    if (type == "double" || type == "float64") return readScalar<double>(in, bigEndian);
    throw std::runtime_error("Unsupported PLY property type: " + type);
}

std::uint32_t readPlyCount(std::istream& in, const std::string& type, bool bigEndian)
{
    return static_cast<std::uint32_t>(readPlyNumber(in, type, bigEndian));
}

void writePlyNumber(std::ostream& out, const std::string& type, double value, bool bigEndian)
{
    if (type == "char" || type == "int8") return writeScalar<int8_t>(out, static_cast<int8_t>(value), bigEndian);
    if (type == "uchar" || type == "uint8") return writeScalar<uint8_t>(out, static_cast<uint8_t>(value), bigEndian);
    if (type == "short" || type == "int16") return writeScalar<int16_t>(out, static_cast<int16_t>(value), bigEndian);
    if (type == "ushort" || type == "uint16") return writeScalar<uint16_t>(out, static_cast<uint16_t>(value), bigEndian);
    if (type == "int" || type == "int32") return writeScalar<int32_t>(out, static_cast<int32_t>(value), bigEndian);
    if (type == "uint" || type == "uint32") return writeScalar<uint32_t>(out, static_cast<uint32_t>(value), bigEndian);
    if (type == "float" || type == "float32") return writeScalar<float>(out, static_cast<float>(value), bigEndian);
    if (type == "double" || type == "float64") return writeScalar<double>(out, value, bigEndian);
    throw std::runtime_error("Unsupported PLY property type: " + type);
}

bool transformAsciiPly(const std::vector<std::string>& headerLines,
                       const std::vector<PlyElement>& elements,
                       std::istream& in,
                       std::ostream& out,
                       const Eigen::Matrix4d& transform,
                       std::string& errorMsg)
{
    out << std::setprecision(17);
    for (const auto& h : headerLines)
        out << h << '\n';

    for (const PlyElement& el : elements) {
        for (std::size_t i = 0; i < el.count; ++i) {
            std::string line;
            if (!std::getline(in, line)) {
                errorMsg = "Unexpected end of ASCII PLY data";
                return false;
            }

            if (el.name != "vertex") {
                out << line << '\n';
                continue;
            }

            std::istringstream ls(line);
            std::vector<std::string> values;
            std::string value;
            while (ls >> value)
                values.push_back(value);

            int ix = -1, iy = -1, iz = -1, inx = -1, iny = -1, inz = -1;
            for (std::size_t p = 0; p < el.properties.size(); ++p) {
                const auto& name = el.properties[p].name;
                if (name == "x") ix = static_cast<int>(p);
                else if (name == "y") iy = static_cast<int>(p);
                else if (name == "z") iz = static_cast<int>(p);
                else if (name == "nx") inx = static_cast<int>(p);
                else if (name == "ny") iny = static_cast<int>(p);
                else if (name == "nz") inz = static_cast<int>(p);
            }

            if (ix >= 0 && iy >= 0 && iz >= 0 &&
                values.size() >= el.properties.size()) {
                Eigen::Vector3d p = transformPoint(std::stod(values[ix]), std::stod(values[iy]), std::stod(values[iz]), transform);
                values[ix] = std::to_string(p.x());
                values[iy] = std::to_string(p.y());
                values[iz] = std::to_string(p.z());
            }
            if (inx >= 0 && iny >= 0 && inz >= 0 &&
                values.size() >= el.properties.size()) {
                Eigen::Vector3d n = transformNormal(std::stod(values[inx]), std::stod(values[iny]), std::stod(values[inz]), transform);
                values[inx] = std::to_string(n.x());
                values[iny] = std::to_string(n.y());
                values[inz] = std::to_string(n.z());
            }

            for (std::size_t p = 0; p < values.size(); ++p) {
                if (p > 0) out << ' ';
                out << values[p];
            }
            out << '\n';
        }
    }
    return true;
}

bool transformBinaryPly(const std::vector<std::string>& headerLines,
                        const std::vector<PlyElement>& elements,
                        std::istream& in,
                        std::ostream& out,
                        const Eigen::Matrix4d& transform,
                        bool bigEndian,
                        std::string& errorMsg)
{
    for (const auto& h : headerLines)
        out << h << '\n';

    for (const PlyElement& el : elements) {
        int ix = -1, iy = -1, iz = -1, inx = -1, iny = -1, inz = -1;
        if (el.name == "vertex") {
            for (std::size_t p = 0; p < el.properties.size(); ++p) {
                const auto& name = el.properties[p].name;
                if (name == "x") ix = static_cast<int>(p);
                else if (name == "y") iy = static_cast<int>(p);
                else if (name == "z") iz = static_cast<int>(p);
                else if (name == "nx") inx = static_cast<int>(p);
                else if (name == "ny") iny = static_cast<int>(p);
                else if (name == "nz") inz = static_cast<int>(p);
            }
        }

        for (std::size_t i = 0; i < el.count; ++i) {
            std::vector<double> scalars(el.properties.size(), 0.0);
            std::vector<std::vector<char>> listPayloads(el.properties.size());
            std::vector<std::uint32_t> listCounts(el.properties.size(), 0);

            for (std::size_t p = 0; p < el.properties.size(); ++p) {
                const PlyProperty& prop = el.properties[p];
                if (!prop.isList) {
                    scalars[p] = readPlyNumber(in, prop.type, bigEndian);
                } else {
                    const std::uint32_t count = readPlyCount(in, prop.countType, bigEndian);
                    listCounts[p] = count;
                    const std::size_t bytes = scalarSize(prop.itemType) * count;
                    listPayloads[p].resize(bytes);
                    in.read(listPayloads[p].data(), static_cast<std::streamsize>(bytes));
                }
            }

            if (!in) {
                errorMsg = "Unexpected end of binary PLY data";
                return false;
            }

            if (el.name == "vertex" && ix >= 0 && iy >= 0 && iz >= 0) {
                Eigen::Vector3d p = transformPoint(scalars[ix], scalars[iy], scalars[iz], transform);
                scalars[ix] = p.x();
                scalars[iy] = p.y();
                scalars[iz] = p.z();
            }
            if (el.name == "vertex" && inx >= 0 && iny >= 0 && inz >= 0) {
                Eigen::Vector3d n = transformNormal(scalars[inx], scalars[iny], scalars[inz], transform);
                scalars[inx] = n.x();
                scalars[iny] = n.y();
                scalars[inz] = n.z();
            }

            for (std::size_t p = 0; p < el.properties.size(); ++p) {
                const PlyProperty& prop = el.properties[p];
                if (!prop.isList) {
                    writePlyNumber(out, prop.type, scalars[p], bigEndian);
                } else {
                    writePlyNumber(out, prop.countType, listCounts[p], bigEndian);
                    out.write(listPayloads[p].data(), static_cast<std::streamsize>(listPayloads[p].size()));
                }
            }
        }
    }
    return true;
}

bool transformPlyPreservingRecords(const std::string& inputPath,
                                   const Eigen::Matrix4d& transform,
                                   const std::string& outputPath,
                                   std::string& errorMsg)
{
    if (!writeParentDirectories(outputPath, errorMsg))
        return false;

    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        errorMsg = "Cannot open PLY file: " + inputPath;
        return false;
    }

    std::vector<std::string> headerLines;
    std::vector<PlyElement> elements;
    std::string format;
    std::string line;
    PlyElement* currentElement = nullptr;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        headerLines.push_back(line);

        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "format") {
            ls >> format;
        } else if (tag == "element") {
            PlyElement el;
            ls >> el.name >> el.count;
            elements.push_back(el);
            currentElement = &elements.back();
        } else if (tag == "property" && currentElement) {
            PlyProperty prop;
            std::string first;
            ls >> first;
            if (first == "list") {
                prop.isList = true;
                ls >> prop.countType >> prop.itemType >> prop.name;
            } else {
                prop.type = first;
                ls >> prop.name;
            }
            currentElement->properties.push_back(prop);
        } else if (tag == "end_header") {
            break;
        }
    }

    if (headerLines.empty() || headerLines.front() != "ply") {
        errorMsg = "Not a PLY file: " + inputPath;
        return false;
    }

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        errorMsg = "Cannot write PLY file: " + outputPath;
        return false;
    }

    try {
        if (format == "ascii") {
            return transformAsciiPly(headerLines, elements, in, out, transform, errorMsg);
        }
        if (format == "binary_little_endian" || format == "binary_big_endian") {
            return transformBinaryPly(headerLines, elements, in, out, transform,
                                      format == "binary_big_endian", errorMsg);
        }
    } catch (const std::exception& e) {
        errorMsg = std::string("PLY transform failed: ") + e.what();
        return false;
    }

    errorMsg = "Unsupported PLY format: " + format;
    return false;
}

bool transformBinaryStlPreservingAttributes(const std::string& inputPath,
                                            const Eigen::Matrix4d& transform,
                                            const std::string& outputPath,
                                            std::string& errorMsg)
{
    if (!writeParentDirectories(outputPath, errorMsg))
        return false;

    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        errorMsg = "Cannot open STL file: " + inputPath;
        return false;
    }

    char header[80] = {};
    in.read(header, 80);
    std::uint32_t count = 0;
    in.read(reinterpret_cast<char*>(&count), 4);
    if (!in || count == 0) {
        // ASCII STL or invalid binary STL: use mesh writer fallback.
        auto scan = STLReader::read(inputPath, errorMsg);
        if (!scan)
            return false;
        return STLWriter::writeTransformed(*scan, transform, outputPath, errorMsg);
    }

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        errorMsg = "Cannot write STL file: " + outputPath;
        return false;
    }

    out.write(header, 80);
    out.write(reinterpret_cast<const char*>(&count), 4);

    for (std::uint32_t i = 0; i < count; ++i) {
        float buf[12];
        std::uint16_t attr = 0;
        in.read(reinterpret_cast<char*>(buf), 48);
        in.read(reinterpret_cast<char*>(&attr), 2);
        if (!in) {
            errorMsg = "Unexpected end of STL file";
            return false;
        }

        Eigen::Vector3d v0 = transformPoint(buf[3],  buf[4],  buf[5],  transform);
        Eigen::Vector3d v1 = transformPoint(buf[6],  buf[7],  buf[8],  transform);
        Eigen::Vector3d v2 = transformPoint(buf[9],  buf[10], buf[11], transform);
        Eigen::Vector3d n = (v1 - v0).cross(v2 - v0);
        if (n.norm() > 1e-12) n.normalize();

        float outBuf[12] = {
            static_cast<float>(n.x()), static_cast<float>(n.y()), static_cast<float>(n.z()),
            static_cast<float>(v0.x()), static_cast<float>(v0.y()), static_cast<float>(v0.z()),
            static_cast<float>(v1.x()), static_cast<float>(v1.y()), static_cast<float>(v1.z()),
            static_cast<float>(v2.x()), static_cast<float>(v2.y()), static_cast<float>(v2.z())
        };
        out.write(reinterpret_cast<const char*>(outBuf), 48);
        out.write(reinterpret_cast<const char*>(&attr), 2);
    }

    return static_cast<bool>(out);
}

} // anonymous namespace

MeshFormat formatFromPath(const std::string& filePath)
{
    const std::string ext = lowerExtension(filePath);
    if (ext == ".stl") return MeshFormat::STL;
    if (ext == ".ply") return MeshFormat::PLY;
    if (ext == ".obj") return MeshFormat::OBJ;
    return MeshFormat::Unknown;
}

std::string extensionForFormat(MeshFormat format)
{
    switch (format) {
        case MeshFormat::STL: return ".stl";
        case MeshFormat::PLY: return ".ply";
        case MeshFormat::OBJ: return ".obj";
        default: return "";
    }
}

bool isSupportedMeshFile(const std::string& filePath)
{
    return formatFromPath(filePath) != MeshFormat::Unknown;
}

std::shared_ptr<ScanData> read(const std::string& filePath, std::string& errorMsg)
{
    if (formatFromPath(filePath) == MeshFormat::STL)
        return STLReader::read(filePath, errorMsg);

    if (formatFromPath(filePath) == MeshFormat::Unknown) {
        errorMsg = "Unsupported mesh format: " + filePath;
        return nullptr;
    }

    auto scan = std::make_shared<ScanData>();
    if (!CGAL::IO::read_polygon_mesh(filePath, scan->mesh)) {
        errorMsg = "Cannot read mesh file: " + filePath;
        return nullptr;
    }

    if (scan->mesh.is_empty()) {
        errorMsg = "Mesh is empty after reading: " + filePath;
        return nullptr;
    }

    namespace PMP = CGAL::Polygon_mesh_processing;
    PMP::triangulate_faces(scan->mesh);

    scan->filePath = filePath;
    scan->scannerName = extractScannerName(filePath);
    updateStats(*scan);
    return scan;
}

bool writeTransformed(const ScanData& scan,
                      const Eigen::Matrix4d& transform,
                      const std::string& outputPath,
                      bool saveAsStl,
                      std::string& errorMsg)
{
    if (saveAsStl)
        return STLWriter::writeTransformed(scan, transform, outputPath, errorMsg);

    switch (formatFromPath(scan.filePath)) {
        case MeshFormat::OBJ:
            return transformObjPreservingRecords(scan.filePath, transform, outputPath, errorMsg);
        case MeshFormat::PLY:
            return transformPlyPreservingRecords(scan.filePath, transform, outputPath, errorMsg);
        case MeshFormat::STL:
            return transformBinaryStlPreservingAttributes(scan.filePath, transform, outputPath, errorMsg);
        default:
            errorMsg = "Unsupported mesh format for writing: " + scan.filePath;
            return false;
    }
}

} // namespace MeshIO
