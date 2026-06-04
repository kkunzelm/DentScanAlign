#include "AlignmentWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

AlignmentWidget::AlignmentWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);

    // Landmark indicators
    auto* landmarkGroup = new QGroupBox("Landmarks");
    auto* landmarkLayout = new QHBoxLayout(landmarkGroup);

    const char* landmarkNames[3] = {"Midline", "Right", "Left"};
    for (int i = 0; i < 3; ++i) {
        m_landmarkLabels[i] = new QLabel(QString("O %1").arg(landmarkNames[i]));
        m_landmarkLabels[i]->setStyleSheet("color: gray;");
        landmarkLayout->addWidget(m_landmarkLabels[i]);
    }
    mainLayout->addWidget(landmarkGroup);

    // Action buttons row 1
    auto* actionLayout1 = new QHBoxLayout();
    m_undoBtn = new QPushButton("Undo Last");
    m_clearBtn = new QPushButton("Clear All");
    m_computeBtn = new QPushButton("Compute Transform");
    m_computeBtn->setEnabled(false);

    actionLayout1->addWidget(m_undoBtn);
    actionLayout1->addWidget(m_clearBtn);
    actionLayout1->addWidget(m_computeBtn);
    mainLayout->addLayout(actionLayout1);

    // Preview toggle
    auto* previewLayout = new QHBoxLayout();
    m_previewCheck = new QCheckBox("Preview normalized orientation");
    m_previewCheck->setEnabled(false);
    previewLayout->addWidget(m_previewCheck);
    previewLayout->addStretch();
    mainLayout->addLayout(previewLayout);

    // Action buttons row 2
    auto* actionLayout2 = new QHBoxLayout();
    m_skipBtn = new QPushButton("Skip");
    m_saveBtn = new QPushButton("Save && Next");
    m_saveBtn->setEnabled(false);

    actionLayout2->addWidget(m_skipBtn);
    actionLayout2->addStretch();
    actionLayout2->addWidget(m_saveBtn);
    mainLayout->addLayout(actionLayout2);

    // Status label
    m_statusLabel = new QLabel("Pick landmark 1 (midline)");
    m_statusLabel->setStyleSheet("font-style: italic; color: #555;");
    mainLayout->addWidget(m_statusLabel);

    // Connect signals
    connect(m_undoBtn, &QPushButton::clicked, this, &AlignmentWidget::undoClicked);
    connect(m_clearBtn, &QPushButton::clicked, this, &AlignmentWidget::clearClicked);
    connect(m_computeBtn, &QPushButton::clicked, this, &AlignmentWidget::computeClicked);
    connect(m_previewCheck, &QCheckBox::toggled, this, &AlignmentWidget::previewToggled);
    connect(m_skipBtn, &QPushButton::clicked, this, &AlignmentWidget::skipClicked);
    connect(m_saveBtn, &QPushButton::clicked, this, &AlignmentWidget::saveClicked);
}

void AlignmentWidget::setLandmarkStatus(int index, bool picked)
{
    if (index < 0 || index >= 3) return;

    const char* landmarkNames[3] = {"Midline", "Right", "Left"};
    if (picked) {
        m_landmarkLabels[index]->setText(QString("* %1").arg(landmarkNames[index]));
        m_landmarkLabels[index]->setStyleSheet("color: green; font-weight: bold;");
    } else {
        m_landmarkLabels[index]->setText(QString("O %1").arg(landmarkNames[index]));
        m_landmarkLabels[index]->setStyleSheet("color: gray;");
    }
}

void AlignmentWidget::clearLandmarks()
{
    for (int i = 0; i < 3; ++i) {
        setLandmarkStatus(i, false);
    }
}

void AlignmentWidget::setComputeEnabled(bool enabled)
{
    m_computeBtn->setEnabled(enabled);
}

void AlignmentWidget::setSaveEnabled(bool enabled)
{
    m_saveBtn->setEnabled(enabled);
    m_previewCheck->setEnabled(enabled);
    if (!enabled) {
        m_previewCheck->setChecked(false);
    }
}

bool AlignmentWidget::isPreviewChecked() const
{
    return m_previewCheck->isChecked();
}

void AlignmentWidget::setPreviewChecked(bool checked)
{
    m_previewCheck->setChecked(checked);
}

void AlignmentWidget::setStatus(const QString& msg)
{
    m_statusLabel->setText(msg);
}
