// SPDX-FileCopyrightText: 2024 Prof. Dr. Karl-Heinz Kunzelmann <https://www.kunzelmann.de>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Mesh.h"
#include "MeshViewWidget.h"
#include "AlignmentWidget.h"
#include "AlignmentSession.h"
#include "ToothSegmentation.h"
#include "LandmarkPlaneFitter.h"
#include "CoordinateNormalizer.h"
#include "STLWriter.h"

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>

#include <array>
#include <memory>
#include <vector>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onBrowseInput();
    void onBrowseOutput();
    void onStartProcessing();
    void onPointPicked(double x, double y, double z);
    void onUndoLandmark();
    void onClearLandmarks();
    void onComputeTransform();
    void onPreviewToggled(bool checked);
    void onSkipScan();
    void onSaveAndNext();
    void onAbout();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void loadNextScan();
    void updateProgress();
    void updateLandmarkStatus();
    void growRegion(int landmarkIndex);

    // UI elements
    QLineEdit* m_inputDirEdit;
    QLineEdit* m_outputDirEdit;
    QPushButton* m_browseInputBtn;
    QPushButton* m_browseOutputBtn;
    QPushButton* m_startBtn;
    QLabel* m_progressLabel;
    QProgressBar* m_progressBar;

    MeshViewWidget* m_meshView;
    AlignmentWidget* m_alignWidget;

    // Session state
    AlignmentSession::Session m_session;
    std::shared_ptr<ScanData> m_currentScan;

    // Landmark picking state
    struct LandmarkData {
        std::array<double, 3> seedPoint{0, 0, 0};
        std::vector<VertexDesc> regionVertices;
        bool picked = false;
    };
    LandmarkData m_landmarks[3];  // midline, right, left
    int m_currentLandmarkIndex = 0;

    // Computed transform
    LandmarkPlaneFitter::PlaneResult m_planeResult;
    CoordinateNormalizer::NormalizationResult m_normResult;
    bool m_transformComputed = false;
};
