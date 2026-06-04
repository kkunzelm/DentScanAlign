#include "MainWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QSettings>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUI();
    loadSettings();
}

void MainWindow::loadSettings()
{
    QSettings settings("DentScan", "DentScanAlign");
    m_inputDirEdit->setText(settings.value("inputDir").toString());
    m_outputDirEdit->setText(settings.value("outputDir").toString());
}

void MainWindow::saveSettings()
{
    QSettings settings("DentScan", "DentScanAlign");
    settings.setValue("inputDir", m_inputDirEdit->text());
    settings.setValue("outputDir", m_outputDirEdit->text());
}

void MainWindow::setupUI()
{
    setWindowTitle("DentScanAlign - Coordinate Normalization Tool");
    resize(1200, 800);

    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);

    // Directory selection group
    auto* dirGroup = new QGroupBox("Directories");
    auto* dirLayout = new QVBoxLayout(dirGroup);

    auto* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("Input:"));
    m_inputDirEdit = new QLineEdit();
    m_inputDirEdit->setPlaceholderText("Select directory containing STL files...");
    inputLayout->addWidget(m_inputDirEdit);
    m_browseInputBtn = new QPushButton("Browse...");
    inputLayout->addWidget(m_browseInputBtn);
    dirLayout->addLayout(inputLayout);

    auto* outputLayout = new QHBoxLayout();
    outputLayout->addWidget(new QLabel("Output:"));
    m_outputDirEdit = new QLineEdit();
    m_outputDirEdit->setPlaceholderText("Select output directory...");
    outputLayout->addWidget(m_outputDirEdit);
    m_browseOutputBtn = new QPushButton("Browse...");
    outputLayout->addWidget(m_browseOutputBtn);
    dirLayout->addLayout(outputLayout);

    auto* startLayout = new QHBoxLayout();
    m_startBtn = new QPushButton("Start Processing");
    startLayout->addWidget(m_startBtn);
    startLayout->addStretch();
    dirLayout->addLayout(startLayout);

    mainLayout->addWidget(dirGroup);

    // Progress bar
    auto* progressLayout = new QHBoxLayout();
    m_progressLabel = new QLabel("Ready");
    progressLayout->addWidget(m_progressLabel);
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    progressLayout->addWidget(m_progressBar);
    mainLayout->addLayout(progressLayout);

    // Main content: mesh view + controls
    auto* splitter = new QSplitter(Qt::Horizontal);

    m_meshView = new MeshViewWidget();
    m_meshView->setMinimumSize(600, 500);
    splitter->addWidget(m_meshView);

    m_alignWidget = new AlignmentWidget();
    m_alignWidget->setMinimumWidth(250);
    splitter->addWidget(m_alignWidget);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);

    // Connect signals
    connect(m_browseInputBtn, &QPushButton::clicked, this, &MainWindow::onBrowseInput);
    connect(m_browseOutputBtn, &QPushButton::clicked, this, &MainWindow::onBrowseOutput);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartProcessing);

    connect(m_meshView, &MeshViewWidget::pointPicked, this, &MainWindow::onPointPicked);

    connect(m_alignWidget, &AlignmentWidget::undoClicked, this, &MainWindow::onUndoLandmark);
    connect(m_alignWidget, &AlignmentWidget::clearClicked, this, &MainWindow::onClearLandmarks);
    connect(m_alignWidget, &AlignmentWidget::computeClicked, this, &MainWindow::onComputeTransform);
    connect(m_alignWidget, &AlignmentWidget::previewToggled, this, &MainWindow::onPreviewToggled);
    connect(m_alignWidget, &AlignmentWidget::skipClicked, this, &MainWindow::onSkipScan);
    connect(m_alignWidget, &AlignmentWidget::saveClicked, this, &MainWindow::onSaveAndNext);
}

void MainWindow::onBrowseInput()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Input Directory",
                                                     m_inputDirEdit->text());
    if (!dir.isEmpty()) {
        m_inputDirEdit->setText(dir);
        saveSettings();
    }
}

void MainWindow::onBrowseOutput()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory",
                                                     m_outputDirEdit->text());
    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(dir);
        saveSettings();
    }
}

void MainWindow::onStartProcessing()
{
    QString inputDir = m_inputDirEdit->text();
    QString outputDir = m_outputDirEdit->text();

    if (inputDir.isEmpty()) {
        QMessageBox::warning(this, "Missing Input", "Please select an input directory.");
        return;
    }
    if (outputDir.isEmpty()) {
        QMessageBox::warning(this, "Missing Output", "Please select an output directory.");
        return;
    }

    std::string errorMsg;
    if (!m_session.initialize(inputDir.toStdString(), outputDir.toStdString(), errorMsg)) {
        QMessageBox::critical(this, "Error", QString::fromStdString(errorMsg));
        return;
    }

    if (m_session.totalScanCount() == 0) {
        QMessageBox::information(this, "No Scans", "No STL files found in the input directory.");
        return;
    }

    m_progressBar->setVisible(true);
    m_progressBar->setMaximum(static_cast<int>(m_session.totalScanCount()));

    // Enable picking mode
    m_meshView->setPickMode(true);

    loadNextScan();
}

void MainWindow::loadNextScan()
{
    // Reset state
    for (int i = 0; i < 3; ++i) {
        m_landmarks[i] = LandmarkData();
    }
    m_currentLandmarkIndex = 0;
    m_transformComputed = false;

    m_alignWidget->clearLandmarks();
    m_alignWidget->setComputeEnabled(false);
    m_alignWidget->setSaveEnabled(false);

    std::string errorMsg;
    m_currentScan = m_session.loadNextUnprocessed(errorMsg);

    if (!m_currentScan) {
        if (m_session.remainingCount() == 0) {
            m_progressLabel->setText("All scans processed!");
            m_meshView->clearMesh();
            m_meshView->setPickMode(false);
            QMessageBox::information(this, "Complete",
                                     QString("All %1 scans have been processed.")
                                         .arg(m_session.totalScanCount()));
        } else {
            QMessageBox::critical(this, "Error", QString::fromStdString(errorMsg));
        }
        return;
    }

    m_meshView->setMesh(m_currentScan);
    updateProgress();
    updateLandmarkStatus();
}

void MainWindow::updateProgress()
{
    m_progressBar->setValue(static_cast<int>(m_session.processedCount()));

    QString scanName = QString::fromStdString(m_session.currentScanRelativePath());
    m_progressLabel->setText(QString("%1 (%2 of %3)")
                                 .arg(scanName)
                                 .arg(m_session.currentIndex() + 1)
                                 .arg(m_session.totalScanCount()));
}

void MainWindow::updateLandmarkStatus()
{
    const char* statusMsgs[4] = {
        "Pick landmark 1 (midline)",
        "Pick landmark 2 (right side)",
        "Pick landmark 3 (left side)",
        "All landmarks picked - compute transform"
    };

    int idx = std::min(m_currentLandmarkIndex, 3);
    m_alignWidget->setStatus(statusMsgs[idx]);

    bool allPicked = (m_currentLandmarkIndex >= 3);
    m_alignWidget->setComputeEnabled(allPicked);
}

void MainWindow::onPointPicked(double x, double y, double z)
{
    if (m_currentLandmarkIndex >= 3)
        return;

    m_landmarks[m_currentLandmarkIndex].seedPoint = {x, y, z};
    m_landmarks[m_currentLandmarkIndex].picked = true;

    m_alignWidget->setLandmarkStatus(m_currentLandmarkIndex, true);
    ++m_currentLandmarkIndex;

    // Update visualizations - just show picked points, no region growing yet
    std::vector<std::array<double, 3>> pickedPoints;
    for (int i = 0; i < m_currentLandmarkIndex; ++i) {
        pickedPoints.push_back(m_landmarks[i].seedPoint);
    }
    m_meshView->showPickedPoints(pickedPoints);

    updateLandmarkStatus();
}

void MainWindow::growRegion(int landmarkIndex)
{
    if (!m_currentScan)
        return;

    ToothSegmentation::Params params;
    params.maxGeodesicMm = 2.0;  // Small 2mm region for alignment landmarks
    params.curvatureRepulsion = 0.0;  // Pure geodesic distance
    params.maxFacesPerSeed = 5000;  // Limit for faster computation

    m_landmarks[landmarkIndex].regionVertices =
        ToothSegmentation::getRegionVertices(*m_currentScan,
                                             m_landmarks[landmarkIndex].seedPoint,
                                             params);
}

void MainWindow::onUndoLandmark()
{
    if (m_currentLandmarkIndex <= 0)
        return;

    --m_currentLandmarkIndex;
    m_landmarks[m_currentLandmarkIndex] = LandmarkData();
    m_alignWidget->setLandmarkStatus(m_currentLandmarkIndex, false);

    // Reset transform if computed
    m_transformComputed = false;
    m_alignWidget->setSaveEnabled(false);

    // Update visualizations
    std::vector<std::array<double, 3>> pickedPoints;
    for (int i = 0; i < m_currentLandmarkIndex; ++i) {
        pickedPoints.push_back(m_landmarks[i].seedPoint);
    }
    m_meshView->showPickedPoints(pickedPoints);

    // Restore original view if preview was on
    if (m_alignWidget->isPreviewChecked()) {
        m_meshView->setMesh(m_currentScan, false);
    }

    updateLandmarkStatus();
}

void MainWindow::onClearLandmarks()
{
    for (int i = 0; i < 3; ++i) {
        m_landmarks[i] = LandmarkData();
    }
    m_currentLandmarkIndex = 0;
    m_transformComputed = false;

    m_alignWidget->clearLandmarks();
    m_alignWidget->setComputeEnabled(false);
    m_alignWidget->setSaveEnabled(false);
    m_meshView->clearHighlights();

    // Restore original view
    m_meshView->setMesh(m_currentScan, false);

    updateLandmarkStatus();
}

void MainWindow::onComputeTransform()
{
    if (!m_currentScan || m_currentLandmarkIndex < 3)
        return;

    // Grow regions around all seed points now
    for (int i = 0; i < 3; ++i) {
        growRegion(i);
    }

    // Fit plane through landmark regions
    m_planeResult = LandmarkPlaneFitter::fitPlane(
        m_currentScan->mesh,
        m_landmarks[0].regionVertices,
        m_landmarks[1].regionVertices,
        m_landmarks[2].regionVertices);

    if (!m_planeResult.valid) {
        QMessageBox::warning(this, "Computation Failed",
                             "Could not fit plane through landmark regions.");
        return;
    }

    // Compute normalization transform
    m_normResult = CoordinateNormalizer::computeTransform(m_currentScan->mesh, m_planeResult);

    if (!m_normResult.valid) {
        QMessageBox::warning(this, "Computation Failed",
                             "Could not compute normalization transform.");
        return;
    }

    m_transformComputed = true;
    m_alignWidget->setSaveEnabled(true);
    m_alignWidget->setStatus("Transform computed - preview or save");

    // Show preview by default
    m_alignWidget->setPreviewChecked(true);
    m_meshView->setMeshTransformed(m_currentScan, m_normResult.transform, true);
}

void MainWindow::onPreviewToggled(bool checked)
{
    if (!m_currentScan || !m_transformComputed)
        return;

    if (checked) {
        // Reset camera since transformed mesh is centered at origin
        m_meshView->setMeshTransformed(m_currentScan, m_normResult.transform, true);
    } else {
        // Reset camera to show original mesh position
        m_meshView->setMesh(m_currentScan, true);
    }
}

void MainWindow::onSkipScan()
{
    m_session.skipCurrent();
    loadNextScan();
}

void MainWindow::onSaveAndNext()
{
    if (!m_currentScan || !m_transformComputed)
        return;

    // Build alignment record
    AlignmentSession::AlignmentRecord record;
    record.sourceFile = m_session.currentScanRelativePath();

    // Landmarks
    record.midline.seed = m_landmarks[0].seedPoint;
    record.midline.vertexCount = m_landmarks[0].regionVertices.size();
    record.right.seed = m_landmarks[1].seedPoint;
    record.right.vertexCount = m_landmarks[1].regionVertices.size();
    record.left.seed = m_landmarks[2].seedPoint;
    record.left.vertexCount = m_landmarks[2].regionVertices.size();

    // Computed geometry
    record.fittedPlaneNormal = {m_planeResult.normal.x(),
                                m_planeResult.normal.y(),
                                m_planeResult.normal.z()};
    record.axisX = {m_planeResult.xAxis.x(),
                    m_planeResult.xAxis.y(),
                    m_planeResult.xAxis.z()};
    record.axisY = {m_planeResult.yAxis.x(),
                    m_planeResult.yAxis.y(),
                    m_planeResult.yAxis.z()};
    record.axisZ = {m_planeResult.zAxis.x(),
                    m_planeResult.zAxis.y(),
                    m_planeResult.zAxis.z()};
    record.meshCentroid = {m_normResult.meshCentroid.x(),
                           m_normResult.meshCentroid.y(),
                           m_normResult.meshCentroid.z()};
    record.transform4x4 = CoordinateNormalizer::matrixToArray(m_normResult.transform);
    record.valid = true;

    std::string errorMsg;
    if (!m_session.saveAlignment(record, errorMsg)) {
        QMessageBox::critical(this, "Save Error", QString::fromStdString(errorMsg));
        return;
    }

    loadNextScan();
}
