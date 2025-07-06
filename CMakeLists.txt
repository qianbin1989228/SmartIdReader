cmake_minimum_required(VERSION 3.16)

project(SmartIdReader VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Multimedia) # 1. 添加Multimedia
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup(REQUIRES 6.8)


# --- 开始集成OpenCV ---
# 1. 设置OpenCV的根目录，请根据实际安装路径修改
set(OpenCV_DIR "D:/toolplace/opencv/build/x64/vc16/lib")

# 2. 查找OpenCV包，Core和Imgproc是我们需要的模块
find_package(OpenCV REQUIRED COMPONENTS core imgproc)

# 3. 包含OpenCV的头文件目录
include_directories(${OpenCV_INCLUDE_DIRS})
# --- 结束集成OpenCV ---




qt_add_executable(appSmartIdReader
    main.cpp
    logo.rc
)

qt_add_qml_module(appSmartIdReader
    URI SmartIdReader
    VERSION 1.0
    QML_FILES
        Main.qml
        QML_FILES PhotoFrame.qml
        SOURCES backend.h backend.cpp
        SOURCES cameraworker.h cameraworker.cpp
        SOURCES imageprovider.h imageprovider.cpp
)


qt_add_resources(appSmartIdReader "app_images"
        PREFIX "/"
        FILES
            icons/appicon.png
            icons/zap.svg
            icons/settings.svg
            icons/camera.svg
            icons/save.svg
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appSmartIdReader PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appSmartIdReader
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appSmartIdReader
    PRIVATE Qt6::Quick
)
target_link_libraries(appSmartIdReader PRIVATE Qt6::Core)
target_link_libraries(appSmartIdReader
    PRIVATE Qt6::Multimedia # 2. 链接Multimedia库
)
target_link_libraries(appSmartIdReader PRIVATE ${OpenCV_LIBS}) # 链接OpenCV库

include(GNUInstallDirs)
install(TARGETS appSmartIdReader
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
