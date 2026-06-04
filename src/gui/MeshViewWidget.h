#pragma once

#include "Mesh.h"
#include <QWidget>
#include <QLabel>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <Eigen/Core>
#include <QPoint>
#include <array>
#include <memory>
#include <vector>

class QVTKOpenGLNativeWidget;

// VTK-based 3D mesh viewer with point picking support.
// Used for DentScanAlign's landmark picking workflow.
class MeshViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MeshViewWidget(QWidget* parent = nullptr);
    ~MeshViewWidget() override;

    // Set mesh to display
    void setMesh(const std::shared_ptr<ScanData>& scan, bool resetCamera = true);

    // Set mesh with a transform applied for preview
    void setMeshTransformed(const std::shared_ptr<ScanData>& scan,
                            const Eigen::Matrix4d& transform,
                            bool resetCamera = true);

    // Set mesh with curvature-based coloring (convex=warm, concave=cool)
    void setMeshWithCurvature(const std::shared_ptr<ScanData>& scan, bool resetCamera = true);

    // Set transformed mesh with curvature coloring
    void setMeshTransformedWithCurvature(const std::shared_ptr<ScanData>& scan,
                                          const Eigen::Matrix4d& transform,
                                          bool resetCamera = true);

    // Clear the mesh
    void clearMesh();

    // Reset camera to fit the mesh
    void resetCamera();

    // Point picking mode
    void setPickMode(bool active);
    bool pickMode() const { return m_pickMode; }

    // Show yellow spheres at picked points
    void showPickedPoints(const std::vector<std::array<double, 3>>& points);

    // Highlight region vertices (show in different color)
    void highlightRegion(const std::vector<VertexDesc>& vertices);

    // Clear all highlight/picked visualizations
    void clearHighlights();

    // Get renderer for external access
    vtkRenderer* renderer() const { return m_renderer; }

signals:
    // Emitted when user clicks on mesh surface in pick mode
    void pointPicked(double x, double y, double z);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void buildPipeline();
    static vtkSmartPointer<vtkPolyData> cgalToVTK(const SurfaceMesh& mesh);
    static vtkSmartPointer<vtkPolyData> cgalToVTKTransformed(
        const SurfaceMesh& mesh, const Eigen::Matrix4d& transform);

    QVTKOpenGLNativeWidget* m_vtkWidget = nullptr;
    bool m_pickMode = false;
    QPoint m_pressPos;

    // Current mesh data (for highlighting)
    std::shared_ptr<ScanData> m_currentScan;

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkPolyData> m_polyData;
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;

    // Picked point spheres
    std::vector<vtkSmartPointer<vtkActor>> m_sphereActors;

    // Highlighted region actor
    vtkSmartPointer<vtkActor> m_highlightActor;
};
