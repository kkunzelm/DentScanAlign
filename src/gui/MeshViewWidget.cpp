#include "MeshViewWidget.h"

#include <QVBoxLayout>
#include <QMouseEvent>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkCellPicker.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>

MeshViewWidget::MeshViewWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_vtkWidget = new QVTKOpenGLNativeWidget(this);
    layout->addWidget(m_vtkWidget);

    buildPipeline();

    m_vtkWidget->installEventFilter(this);
}

MeshViewWidget::~MeshViewWidget() = default;

void MeshViewWidget::buildPipeline()
{
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_vtkWidget->setRenderWindow(m_renderWindow);

    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(0.2, 0.2, 0.25);
    m_renderWindow->AddRenderer(m_renderer);

    m_polyData = vtkSmartPointer<vtkPolyData>::New();
    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_mapper->SetInputData(m_polyData);

    m_actor = vtkSmartPointer<vtkActor>::New();
    m_actor->SetMapper(m_mapper);
    m_actor->GetProperty()->SetColor(0.9, 0.85, 0.78);  // Ivory color
    m_actor->GetProperty()->SetAmbient(0.2);
    m_actor->GetProperty()->SetDiffuse(0.8);
    m_actor->GetProperty()->SetSpecular(0.3);
    m_renderer->AddActor(m_actor);

    auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    m_renderWindow->GetInteractor()->SetInteractorStyle(style);
}

vtkSmartPointer<vtkPolyData> MeshViewWidget::cgalToVTK(const SurfaceMesh& mesh)
{
    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto polys = vtkSmartPointer<vtkCellArray>::New();

    // Add vertices
    std::vector<vtkIdType> idMap(mesh.num_vertices());
    vtkIdType idx = 0;
    for (auto v : mesh.vertices()) {
        const Point3& p = mesh.point(v);
        points->InsertNextPoint(CGAL::to_double(p.x()),
                                CGAL::to_double(p.y()),
                                CGAL::to_double(p.z()));
        idMap[v.idx()] = idx++;
    }

    // Add faces
    for (auto f : mesh.faces()) {
        vtkIdType ids[3];
        int i = 0;
        for (auto v : mesh.vertices_around_face(mesh.halfedge(f))) {
            ids[i++] = idMap[v.idx()];
        }
        polys->InsertNextCell(3, ids);
    }

    polyData->SetPoints(points);
    polyData->SetPolys(polys);
    return polyData;
}

vtkSmartPointer<vtkPolyData> MeshViewWidget::cgalToVTKTransformed(
    const SurfaceMesh& mesh, const Eigen::Matrix4d& transform)
{
    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto polys = vtkSmartPointer<vtkCellArray>::New();

    // Add transformed vertices
    std::vector<vtkIdType> idMap(mesh.num_vertices());
    vtkIdType idx = 0;
    for (auto v : mesh.vertices()) {
        const Point3& p = mesh.point(v);
        Eigen::Vector4d pt(CGAL::to_double(p.x()),
                           CGAL::to_double(p.y()),
                           CGAL::to_double(p.z()),
                           1.0);
        Eigen::Vector4d tp = transform * pt;
        points->InsertNextPoint(tp[0], tp[1], tp[2]);
        idMap[v.idx()] = idx++;
    }

    // Add faces
    for (auto f : mesh.faces()) {
        vtkIdType ids[3];
        int i = 0;
        for (auto v : mesh.vertices_around_face(mesh.halfedge(f))) {
            ids[i++] = idMap[v.idx()];
        }
        polys->InsertNextCell(3, ids);
    }

    polyData->SetPoints(points);
    polyData->SetPolys(polys);
    return polyData;
}

void MeshViewWidget::setMesh(const std::shared_ptr<ScanData>& scan, bool resetCam)
{
    clearHighlights();
    m_currentScan = scan;

    if (!scan || scan->mesh.is_empty()) {
        m_polyData->Reset();
        m_renderWindow->Render();
        return;
    }

    auto pd = cgalToVTK(scan->mesh);
    m_polyData->ShallowCopy(pd);

    if (resetCam)
        resetCamera();

    m_renderWindow->Render();
}

void MeshViewWidget::setMeshTransformed(const std::shared_ptr<ScanData>& scan,
                                         const Eigen::Matrix4d& transform,
                                         bool resetCam)
{
    clearHighlights();

    if (!scan || scan->mesh.is_empty()) {
        m_polyData->Reset();
        m_renderWindow->Render();
        return;
    }

    auto pd = cgalToVTKTransformed(scan->mesh, transform);
    m_polyData->ShallowCopy(pd);

    if (resetCam)
        resetCamera();

    m_renderWindow->Render();
}

void MeshViewWidget::clearMesh()
{
    m_currentScan = nullptr;
    clearHighlights();
    m_polyData->Reset();
    m_renderWindow->Render();
}

void MeshViewWidget::resetCamera()
{
    m_renderer->ResetCamera();
    m_renderWindow->Render();
}

void MeshViewWidget::setPickMode(bool active)
{
    m_pickMode = active;
}

void MeshViewWidget::showPickedPoints(const std::vector<std::array<double, 3>>& points)
{
    // Remove existing spheres
    for (auto& actor : m_sphereActors) {
        m_renderer->RemoveActor(actor);
    }
    m_sphereActors.clear();

    // Add new spheres (matching DentScanCompare style)
    for (const auto& pt : points) {
        auto sphere = vtkSmartPointer<vtkSphereSource>::New();
        sphere->SetCenter(pt[0], pt[1], pt[2]);
        sphere->SetRadius(0.6);  // 0.6mm radius like DentScanCompare
        sphere->SetPhiResolution(12);
        sphere->SetThetaResolution(12);

        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphere->GetOutputPort());

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(1.0, 0.85, 0.0);  // Yellow like DentScanCompare
        actor->GetProperty()->SetAmbient(0.4);
        actor->GetProperty()->SetDiffuse(0.6);

        m_renderer->AddActor(actor);
        m_sphereActors.push_back(actor);
    }

    m_renderWindow->Render();
}

void MeshViewWidget::highlightRegion(const std::vector<VertexDesc>& vertices)
{
    if (!m_currentScan || vertices.empty()) {
        if (m_highlightActor) {
            m_renderer->RemoveActor(m_highlightActor);
            m_highlightActor = nullptr;
        }
        m_renderWindow->Render();
        return;
    }

    // Create a point cloud for highlighted vertices
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto cells = vtkSmartPointer<vtkCellArray>::New();

    for (auto v : vertices) {
        const Point3& p = m_currentScan->mesh.point(v);
        vtkIdType id = points->InsertNextPoint(
            CGAL::to_double(p.x()),
            CGAL::to_double(p.y()),
            CGAL::to_double(p.z()));
        cells->InsertNextCell(1, &id);
    }

    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetVerts(cells);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);

    if (m_highlightActor) {
        m_renderer->RemoveActor(m_highlightActor);
    }

    m_highlightActor = vtkSmartPointer<vtkActor>::New();
    m_highlightActor->SetMapper(mapper);
    m_highlightActor->GetProperty()->SetColor(0.2, 0.8, 0.2);  // Green
    m_highlightActor->GetProperty()->SetPointSize(3.0);

    m_renderer->AddActor(m_highlightActor);
    m_renderWindow->Render();
}

void MeshViewWidget::clearHighlights()
{
    // Remove spheres
    for (auto& actor : m_sphereActors) {
        m_renderer->RemoveActor(actor);
    }
    m_sphereActors.clear();

    // Remove highlight
    if (m_highlightActor) {
        m_renderer->RemoveActor(m_highlightActor);
        m_highlightActor = nullptr;
    }

    m_renderWindow->Render();
}

bool MeshViewWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != m_vtkWidget || !m_pickMode)
        return QWidget::eventFilter(obj, event);

    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            m_pressPos = me->pos();
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            // Only trigger pick if minimal movement (click, not drag)
            if ((me->pos() - m_pressPos).manhattanLength() < 6) {
                auto picker = vtkSmartPointer<vtkCellPicker>::New();
                picker->SetTolerance(0.0005);

                // Convert from Qt logical pixels to VTK device pixels (high-DPI support)
                const qreal dpr = m_vtkWidget->devicePixelRatioF();
                const int x = static_cast<int>(me->pos().x() * dpr);
                const int y = static_cast<int>((m_vtkWidget->height() - me->pos().y() - 1) * dpr);

                if (picker->Pick(x, y, 0, m_renderer)) {
                    double* pos = picker->GetPickPosition();
                    emit pointPicked(pos[0], pos[1], pos[2]);
                }
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}
