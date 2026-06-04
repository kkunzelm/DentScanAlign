cmake_minimum_required(VERSION 3.20)
project(DentScanAlign VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Custom VTK installation path
set(VTK_DIR "$ENV{HOME}/VTK-install-linux/lib/cmake/vtk-9.3")

# Find dependencies
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
find_package(VTK 9.3 REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    FiltersSources
    FiltersGeneral
    InteractionStyle
    RenderingCore
    RenderingOpenGL2
    GUISupportQt
)
find_package(CGAL REQUIRED)
find_package(Eigen3 REQUIRED CONFIG)

# Source files
set(CORE_SOURCES
    src/core/STLReader.cpp
    src/core/STLWriter.cpp
    src/core/ToothSegmentation.cpp
)

set(ALIGNMENT_SOURCES
    src/alignment/LandmarkPlaneFitter.cpp
    src/alignment/CoordinateNormalizer.cpp
    src/alignment/AlignmentSession.cpp
)

set(GUI_SOURCES
    src/gui/MainWindow.cpp
    src/gui/AlignmentWidget.cpp
    src/gui/MeshViewWidget.cpp
)

set(ALL_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${ALIGNMENT_SOURCES}
    ${GUI_SOURCES}
)

# Create executable
add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/alignment
    ${CMAKE_SOURCE_DIR}/src/gui
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    ${VTK_LIBRARIES}
    CGAL::CGAL
    Eigen3::Eigen
)

# VTK module autoinit
vtk_module_autoinit(
    TARGETS ${PROJECT_NAME}
    MODULES ${VTK_LIBRARIES}
)
