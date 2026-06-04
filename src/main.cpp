#include "MainWindow.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>

int main(int argc, char* argv[])
{
    // VTK + Qt OpenGL setup
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    QApplication app(argc, argv);
    app.setApplicationName("DentScanAlign");
    app.setOrganizationName("DentScan");

    MainWindow window;
    window.show();

    return app.exec();
}
