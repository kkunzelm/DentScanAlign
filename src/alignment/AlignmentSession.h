// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Mesh.h"
#include "LandmarkPlaneFitter.h"
#include "CoordinateNormalizer.h"

#include <Eigen/Dense>
#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace AlignmentSession {

// Landmark data for one region
struct LandmarkInfo {
    std::array<double, 3> seed{0, 0, 0};    // Seed point position
    std::size_t vertexCount = 0;            // Number of vertices in grown region
};

// Full alignment record for one scan
struct AlignmentRecord {
    std::string sourceFile;                 // Relative path from input directory

    // Jaw type
    CoordinateNormalizer::JawType jawType = CoordinateNormalizer::JawType::Lower;

    // Landmarks
    LandmarkInfo midline;
    LandmarkInfo right;
    LandmarkInfo left;

    // Computed geometry
    std::array<double, 3> fittedPlaneNormal{0, 0, 0};
    std::array<double, 3> axisX{0, 0, 0};
    std::array<double, 3> axisY{0, 0, 0};
    std::array<double, 3> axisZ{0, 0, 0};
    std::array<double, 3> meshCentroid{0, 0, 0};
    std::array<double, 16> transform4x4{};  // Row-major

    std::string timestamp;

    bool valid = false;
};

// Session state for processing a directory of scans
class Session {
public:
    Session() = default;

    // Initialize session with input/output directories
    bool initialize(const std::string& inputDir, const std::string& outputDir,
                    std::string& errorMsg);

    // Scan discovery
    std::size_t totalScanCount() const { return m_allScans.size(); }
    std::size_t processedCount() const { return m_processedCount; }
    std::size_t remainingCount() const { return m_allScans.size() - m_processedCount; }

    // Get next unprocessed scan (returns nullptr if all done)
    std::shared_ptr<ScanData> loadNextUnprocessed(std::string& errorMsg);

    // Get current scan info
    std::string currentScanRelativePath() const { return m_currentRelPath; }
    std::string currentScanAbsolutePath() const { return m_currentAbsPath; }
    std::size_t currentIndex() const { return m_currentIndex; }

    // Save alignment and write normalized mesh. If saveAsStl is true, output uses .stl; otherwise it preserves input format.
    bool saveAlignment(const AlignmentRecord& record, bool saveAsStl, std::string& errorMsg);

    // Skip current scan (no alignment saved)
    void skipCurrent();

    // Mark current scan as processed (for progress tracking when saving in background)
    void markCurrentAsProcessed();

    // Load existing alignment for a scan (for review mode)
    bool loadAlignment(const std::string& relPath, AlignmentRecord& record,
                       std::string& errorMsg);

    // Get list of all scans (relative paths)
    const std::vector<std::string>& allScans() const { return m_allScans; }

    // Check if a scan has alignment
    bool hasAlignment(const std::string& relPath) const;

    // Paths
    std::string inputDirectory() const { return m_inputDir; }
    std::string outputDirectory() const { return m_outputDir; }
    std::string normalizedOutputPath(const std::string& relPath, bool saveAsStl) const;

private:
    void scanDirectory();
    void updateProcessedCount();
    std::string alignmentJsonPath(const std::string& relPath) const;

    std::string m_inputDir;
    std::string m_outputDir;
    std::vector<std::string> m_allScans;        // Relative paths to all STLs
    std::size_t m_processedCount = 0;
    std::size_t m_currentIndex = 0;
    std::string m_currentRelPath;
    std::string m_currentAbsPath;
};

// JSON I/O functions
bool writeJson(const AlignmentRecord& record, const std::string& path,
               std::string& errorMsg);
bool readJson(const std::string& path, AlignmentRecord& record,
              std::string& errorMsg);

} // namespace AlignmentSession
