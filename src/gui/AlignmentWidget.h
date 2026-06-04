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
    void setLandmarkStatus(int index, bool picked);  // 0=point1, 1=point2, 2=point3
    void clearLandmarks();

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
    void previewToggled(bool checked);
    void skipClicked();
    void saveClicked();

private:
    QLabel* m_landmarkLabels[3];    // Point 1, 2, 3 indicators
    QPushButton* m_undoBtn;
    QPushButton* m_clearBtn;
    QCheckBox* m_previewCheck;
    QPushButton* m_skipBtn;
    QPushButton* m_saveBtn;
    QLabel* m_statusLabel;
};
