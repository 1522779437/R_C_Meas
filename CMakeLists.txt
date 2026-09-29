cmake_minimum_required(VERSION 3.16)

project(r_c_meas_v5 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appr_c_meas_v5
    main.cpp
)

qt_add_qml_module(appr_c_meas_v5
    URI r_c_meas_v5
    VERSION 1.0
    QML_FILES
        Main.qml
        UserInfoPanel.qml
        QML_FILES
        QML_FILES CableTestParameters.qml
        QML_FILES TestStatus.qml
        QML_FILES ControlButton.qml
        QML_FILES MeasResult.qml
        QML_FILES CableTypeDialog.qml
        QML_FILES MeasProject.qml
        QML_FILES CableNumberDialog.qml
        SOURCES userinfomodel.h userinfomodel.cpp
        SOURCES cabletestpara.h cabletestpara.cpp
        SOURCES testprojectconfig.h testprojectconfig.cpp
        SOURCES controlpanelmanager.h controlpanelmanager.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appr_c_meas_v5 PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appr_c_meas_v5
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

add_subdirectory(QXlsx)

target_link_libraries(appr_c_meas_v5
    PRIVATE Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 QXlsx::QXlsx
)

include(GNUInstallDirs)
install(TARGETS appr_c_meas_v5
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
