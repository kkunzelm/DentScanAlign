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
#include <vtkUnsignedCharArray.h>

#include <cmath>

namespace {

// Compute mean curvature per vertex using Laplacian approximation
std::vector<double> computeMeanCurvature(const SurfaceMesh& mesh)
{
    std::vector<double> curvature(mesh.num_vertices(), 0.0);

    // For each vertex, compute mean curvature using umbrella operator
    for (auto v : mesh.vertices()) {
        const Point3& p = mesh.point(v);
        Eigen::Vector3d pos(CGAL::to_double(p.x()),
                            CGAL::to_double(p.y()),
                            CGAL::to_double(p.z()));

        // Compute centroid of neighbors (umbrella operator)
        Eigen::Vector3d neighborSum = Eigen::Vector3d::Zero();
        int neighborCount = 0;

        for (auto h : mesh.halfedges_around_target(mesh.halfedge(v))) {
            auto vn = mesh.source(h);
            const Point3& pn = mesh.point(vn);
            neighborSum += Eigen::Vector3d(CGAL::to_double(pn.x()),
                                           CGAL::to_double(pn.y()),
                                           CGAL::to_double(pn.z()));
            ++neighborCount;
        }

        if (neighborCount > 0) {
            Eigen::Vector3d centroid = neighborSum / neighborCount;
            Eigen::Vector3d laplacian = centroid - pos;

            // Compute vertex normal as average of incident face normals
            Eigen::Vector3d normal = Eigen::Vector3d::Zero();
            for (auto h : mesh.halfedges_around_target(mesh.halfedge(v))) {
                if (!mesh.is_border(h)) {
                    auto f = mesh.face(h);
                    auto fh = mesh.halfedge(f);
                    std::array<Eigen::Vector3d, 3> fv;
                    int i = 0;
                    for (auto fvd : mesh.vertices_around_face(fh)) {
                        const Point3& fp = mesh.point(fvd);
                        fv[i++] = Eigen::Vector3d(CGAL::to_double(fp.x()),
                                                  CGAL::to_double(fp.y()),
                                                  CGAL::to_double(fp.z()));
                    }
                    Eigen::Vector3d fn = (fv[1] - fv[0]).cross(fv[2] - fv[0]);
                    double len = fn.norm();
                    if (len > 1e-12) normal += fn / len;
                }
            }
            double nlen = normal.norm();
            if (nlen > 1e-12) {
                normal /= nlen;
                // Signed mean curvature: positive = convex, negative = concave
                curvature[v.idx()] = laplacian.dot(normal);
            }
        }
    }

    return curvature;
}

// Map curvature to RGB color: convex (positive) = orange/warm, concave (negative) = blue/cool
void curvatureToColor(double curv, double scale, unsigned char& r, unsigned char& g, unsigned char& b)
{
    // Normalize curvature to [-1, 1] range
    double t = std::clamp(curv / scale, -1.0, 1.0);

    if (t > 0) {
        // Convex: white to orange
        r = 255;
        g = static_cast<unsigned char>(255 * (1.0 - t * 0.6));
        b = static_cast<unsigned char>(255 * (1.0 - t));
    } else {
        // Concave: white to blue
        double s = -t;
        r = static_cast<unsigned char>(255 * (1.0 - s));
        g = static_cast<unsigned char>(255 * (1.0 - s * 0.5));
        b = 255;
    }
}

} // anonymous namespace

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

void MeshViewWidget::setMeshWithCurvature(const std::shared_ptr<ScanData>& scan, bool resetCam)
{
    clearHighlights();
    m_currentScan = scan;

    if (!scan || scan->mesh.is_empty()) {
        m_polyData->Reset();
        m_renderWindow->Render();
        return;
    }

    auto pd = cgalToVTK(scan->mesh);

    // Compute curvature and add colors
    auto curvature = computeMeanCurvature(scan->mesh);

    // Find curvature scale (use 90th percentile for robustness)
    std::vector<double> absCurv;
    absCurv.reserve(curvature.size());
    for (double c : curvature) absCurv.push_back(std::abs(c));
    std::sort(absCurv.begin(), absCurv.end());
    double scale = absCurv.empty() ? 1.0 : absCurv[absCurv.size() * 9 / 10];
    if (scale < 0.001) scale = 0.001;

    auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colors->SetName("CurvatureColor");
    colors->SetNumberOfComponents(3);
    colors->SetNumberOfTuples(static_cast<vtkIdType>(scan->mesh.num_vertices()));

    for (auto v : scan->mesh.vertices()) {
        unsigned char r, g, b;
        curvatureToColor(curvature[v.idx()], scale, r, g, b);
        colors->SetTuple3(static_cast<vtkIdType>(v.idx()), r, g, b);
    }

    pd->GetPointData()->SetScalars(colors);
    m_polyData->DeepCopy(pd);
    m_mapper->SetColorModeToDirectScalars();
    m_mapper->ScalarVisibilityOn();

    if (resetCam)
        resetCamera();

    m_renderWindow->Render();
}

void MeshViewWidget::setMeshTransformedWithCurvature(const std::shared_ptr<ScanData>& scan,
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

    // Compute curvature and add colors
    auto curvature = computeMeanCurvature(scan->mesh);

    // Find curvature scale
    std::vector<double> absCurv;
    absCurv.reserve(curvature.size());
    for (double c : curvature) absCurv.push_back(std::abs(c));
    std::sort(absCurv.begin(), absCurv.end());
    double scale = absCurv.empty() ? 1.0 : absCurv[absCurv.size() * 9 / 10];
    if (scale < 0.001) scale = 0.001;

    auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colors->SetName("CurvatureColor");
    colors->SetNumberOfComponents(3);
    colors->SetNumberOfTuples(static_cast<vtkIdType>(scan->mesh.num_vertices()));

    for (auto v : scan->mesh.vertices()) {
        unsigned char r, g, b;
        curvatureToColor(curvature[v.idx()], scale, r, g, b);
        colors->SetTuple3(static_cast<vtkIdType>(v.idx()), r, g, b);
    }

    pd->GetPointData()->SetScalars(colors);
    m_polyData->DeepCopy(pd);
    m_mapper->SetColorModeToDirectScalars();
    m_mapper->ScalarVisibilityOn();

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
