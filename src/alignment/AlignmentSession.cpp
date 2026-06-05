// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AlignmentSession.h"
#include "MeshIO.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace AlignmentSession {

namespace {

// Simple JSON writing helpers (no external dependency)
std::string escapeJson(const std::string& s)
{
    std::string result;
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:   result += c;      break;
        }
    }
    return result;
}

std::string toJsonArray3(const std::array<double, 3>& arr)
{
    std::ostringstream ss;
    ss << std::setprecision(9) << "[" << arr[0] << ", " << arr[1] << ", " << arr[2] << "]";
    return ss.str();
}

std::string toJsonArray16(const std::array<double, 16>& arr)
{
    std::ostringstream ss;
    ss << std::setprecision(9) << "[";
    for (int i = 0; i < 16; ++i) {
        if (i > 0) ss << ", ";
        ss << arr[i];
    }
    ss << "]";
    return ss.str();
}

std::string currentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

// Simple JSON parsing helpers
std::string extractJsonString(const std::string& json, const std::string& key)
{
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";

    pos = json.find('"', pos);
    if (pos == std::string::npos) return "";

    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";

    return json.substr(pos + 1, end - pos - 1);
}

std::array<double, 3> extractJsonArray3(const std::string& json, const std::string& key)
{
    std::array<double, 3> result{0, 0, 0};
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return result;

    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;

    auto end = json.find(']', pos);
    if (end == std::string::npos) return result;

    std::string arr = json.substr(pos + 1, end - pos - 1);
    std::istringstream ss(arr);
    char comma;
    ss >> result[0] >> comma >> result[1] >> comma >> result[2];
    return result;
}

std::array<double, 16> extractJsonArray16(const std::string& json, const std::string& key)
{
    std::array<double, 16> result{};
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return result;

    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;

    auto end = json.find(']', pos);
    if (end == std::string::npos) return result;

    std::string arr = json.substr(pos + 1, end - pos - 1);
    std::istringstream ss(arr);
    for (int i = 0; i < 16; ++i) {
        ss >> result[i];
        if (i < 15) {
            char comma;
            ss >> comma;
        }
    }
    return result;
}

std::size_t extractJsonSizeT(const std::string& json, const std::string& key)
{
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return 0;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return 0;

    // Skip whitespace
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
        ++pos;

    std::size_t value = 0;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
        value = value * 10 + (json[pos] - '0');
        ++pos;
    }
    return value;
}

} // anonymous namespace

bool Session::initialize(const std::string& inputDir, const std::string& outputDir,
                         std::string& errorMsg)
{
    if (!std::filesystem::exists(inputDir)) {
        errorMsg = "Input directory does not exist: " + inputDir;
        return false;
    }

    if (!std::filesystem::is_directory(inputDir)) {
        errorMsg = "Input path is not a directory: " + inputDir;
        return false;
    }

    m_inputDir = inputDir;
    m_outputDir = outputDir;

    // Create output subdirectories
    std::error_code ec;
    std::filesystem::create_directories(m_outputDir + "/alignments", ec);
    std::filesystem::create_directories(m_outputDir + "/normalized", ec);
    if (ec) {
        errorMsg = "Cannot create output directories: " + ec.message();
        return false;
    }

    scanDirectory();
    updateProcessedCount();
    m_currentIndex = 0;

    return true;
}

void Session::scanDirectory()
{
    m_allScans.clear();

    for (auto& entry : std::filesystem::recursive_directory_iterator(m_inputDir)) {
        if (!entry.is_regular_file())
            continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (!MeshIO::isSupportedMeshFile(entry.path().string()))
            continue;

        // Store relative path
        auto relPath = std::filesystem::relative(entry.path(), m_inputDir).string();
        m_allScans.push_back(relPath);
    }

    // Sort for consistent ordering
    std::sort(m_allScans.begin(), m_allScans.end());
}

void Session::updateProcessedCount()
{
    m_processedCount = 0;
    for (const auto& rel : m_allScans) {
        if (hasAlignment(rel))
            ++m_processedCount;
    }
}

bool Session::hasAlignment(const std::string& relPath) const
{
    return std::filesystem::exists(alignmentJsonPath(relPath));
}

std::string Session::alignmentJsonPath(const std::string& relPath) const
{
    // Convert path separators to double underscores for flat filename
    std::string flat = relPath;
    for (char& c : flat) {
        if (c == '/' || c == '\\')
            c = '_';
    }
    // Remove mesh extension
    std::filesystem::path flatPath(flat);
    flatPath.replace_extension();
    flat = flatPath.string();

    return m_outputDir + "/alignments/" + flat + ".json";
}

std::string Session::normalizedOutputPath(const std::string& relPath, bool saveAsStl) const
{
    std::filesystem::path rel(relPath);
    if (saveAsStl)
        rel.replace_extension(".stl");
    return (std::filesystem::path(m_outputDir) / "normalized" / rel).string();
}

std::shared_ptr<ScanData> Session::loadNextUnprocessed(std::string& errorMsg)
{
    // Find next unprocessed scan
    while (m_currentIndex < m_allScans.size()) {
        const std::string& relPath = m_allScans[m_currentIndex];
        if (!hasAlignment(relPath)) {
            m_currentRelPath = relPath;
            m_currentAbsPath = m_inputDir + "/" + relPath;
            return MeshIO::read(m_currentAbsPath, errorMsg);
        }
        ++m_currentIndex;
    }

    // All done
    m_currentRelPath.clear();
    m_currentAbsPath.clear();
    return nullptr;
}

bool Session::saveAlignment(const AlignmentRecord& record, bool saveAsStl, std::string& errorMsg)
{
    // Write JSON
    std::string jsonPath = alignmentJsonPath(m_currentRelPath);
    if (!writeJson(record, jsonPath, errorMsg))
        return false;

    // Write normalized STL
    auto scan = MeshIO::read(m_currentAbsPath, errorMsg);
    if (!scan)
        return false;

    Eigen::Matrix4d transform = CoordinateNormalizer::arrayToMatrix(record.transform4x4);
    std::string outPath = normalizedOutputPath(m_currentRelPath, saveAsStl);
    if (!MeshIO::writeTransformed(*scan, transform, outPath, saveAsStl, errorMsg))
        return false;

    // Move to next
    ++m_currentIndex;
    ++m_processedCount;
    return true;
}

void Session::skipCurrent()
{
    ++m_currentIndex;
}

void Session::markCurrentAsProcessed()
{
    ++m_currentIndex;
    ++m_processedCount;
}

bool Session::loadAlignment(const std::string& relPath, AlignmentRecord& record,
                            std::string& errorMsg)
{
    return readJson(alignmentJsonPath(relPath), record, errorMsg);
}

bool writeJson(const AlignmentRecord& record, const std::string& path,
               std::string& errorMsg)
{
    // Ensure parent directory exists
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ofstream file(path);
    if (!file) {
        errorMsg = "Cannot open file for writing: " + path;
        return false;
    }

    file << "{\n";
    file << "  \"source_file\": \"" << escapeJson(record.sourceFile) << "\",\n";
    file << "  \"landmarks\": {\n";
    file << "    \"midline\": {\"seed\": " << toJsonArray3(record.midline.seed)
         << ", \"vertex_count\": " << record.midline.vertexCount << "},\n";
    file << "    \"right\": {\"seed\": " << toJsonArray3(record.right.seed)
         << ", \"vertex_count\": " << record.right.vertexCount << "},\n";
    file << "    \"left\": {\"seed\": " << toJsonArray3(record.left.seed)
         << ", \"vertex_count\": " << record.left.vertexCount << "}\n";
    file << "  },\n";
    file << "  \"fitted_plane_normal\": " << toJsonArray3(record.fittedPlaneNormal) << ",\n";
    file << "  \"computed_axes\": {\n";
    file << "    \"x\": " << toJsonArray3(record.axisX) << ",\n";
    file << "    \"y\": " << toJsonArray3(record.axisY) << ",\n";
    file << "    \"z\": " << toJsonArray3(record.axisZ) << "\n";
    file << "  },\n";
    file << "  \"mesh_centroid\": " << toJsonArray3(record.meshCentroid) << ",\n";
    file << "  \"transform_4x4\": " << toJsonArray16(record.transform4x4) << ",\n";
    file << "  \"timestamp\": \"" << (record.timestamp.empty() ? currentTimestamp() : record.timestamp) << "\"\n";
    file << "}\n";

    if (!file) {
        errorMsg = "Error writing to file";
        return false;
    }

    return true;
}

bool readJson(const std::string& path, AlignmentRecord& record, std::string& errorMsg)
{
    std::ifstream file(path);
    if (!file) {
        errorMsg = "Cannot open file: " + path;
        return false;
    }

    std::string json((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

    record.sourceFile = extractJsonString(json, "source_file");
    record.timestamp = extractJsonString(json, "timestamp");

    // Find landmarks section
    auto landmarksPos = json.find("\"landmarks\"");
    if (landmarksPos != std::string::npos) {
        std::string landmarksSection = json.substr(landmarksPos);

        // Midline
        auto midlinePos = landmarksSection.find("\"midline\"");
        if (midlinePos != std::string::npos) {
            std::string midlineSection = landmarksSection.substr(midlinePos);
            record.midline.seed = extractJsonArray3(midlineSection, "seed");
            record.midline.vertexCount = extractJsonSizeT(midlineSection, "vertex_count");
        }

        // Right
        auto rightPos = landmarksSection.find("\"right\"");
        if (rightPos != std::string::npos) {
            std::string rightSection = landmarksSection.substr(rightPos);
            record.right.seed = extractJsonArray3(rightSection, "seed");
            record.right.vertexCount = extractJsonSizeT(rightSection, "vertex_count");
        }

        // Left
        auto leftPos = landmarksSection.find("\"left\"");
        if (leftPos != std::string::npos) {
            std::string leftSection = landmarksSection.substr(leftPos);
            record.left.seed = extractJsonArray3(leftSection, "seed");
            record.left.vertexCount = extractJsonSizeT(leftSection, "vertex_count");
        }
    }

    record.fittedPlaneNormal = extractJsonArray3(json, "fitted_plane_normal");
    record.meshCentroid = extractJsonArray3(json, "mesh_centroid");
    record.transform4x4 = extractJsonArray16(json, "transform_4x4");

    // Axes from computed_axes section
    auto axesPos = json.find("\"computed_axes\"");
    if (axesPos != std::string::npos) {
        std::string axesSection = json.substr(axesPos);
        record.axisX = extractJsonArray3(axesSection, "x");
        record.axisY = extractJsonArray3(axesSection, "y");
        record.axisZ = extractJsonArray3(axesSection, "z");
    }

    record.valid = true;
    return true;
}

} // namespace AlignmentSession
