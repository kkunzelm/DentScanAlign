#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <array>

// Widget containing alignment controls: landmark indicators, action buttons, preview toggle.
class AlignmentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AlignmentWidget(QWidget* parent = nullptr);

    // Update landmark status indicators
    void setLandmarkStatus(int index, bool picked);  // 0=midline, 1=right, 2=left
    void clearLandmarks();

    // Enable/disable compute button (requires all 3 landmarks)
    void setComputeEnabled(bool enabled);

    // Enable/disable save button (requires computed transform)
    void setSaveEnabled(bool enabled);

    // Get/set preview checkbox state
    bool isPreviewChecked() const;
    void setPreviewChecked(bool checked);

    // Set status message
    void setStatus(const QString& msg);

signals:
    void undoClicked();
    void clearClicked();
    void computeClicked();
    void previewToggled(bool checked);
    void skipClicked();
    void saveClicked();

private:
    QLabel* m_landmarkLabels[3];    // Midline, Right, Left indicators
    QPushButton* m_undoBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_computeBtn;
    QCheckBox* m_previewCheck;
    QPushButton* m_skipBtn;
    QPushButton* m_saveBtn;
    QLabel* m_statusLabel;
};
