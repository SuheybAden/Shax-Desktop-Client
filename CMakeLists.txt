cmake_minimum_required(VERSION 3.19)

project(shax-desktop-client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools WebSockets)

qt_standard_project_setup(
    I18N_TRANSLATED_LANGUAGES so_SO
)

set(PROJECT_SOURCES
        src/main.cpp
        src/gui/mainwindow.cpp
        src/gui/mainwindow.ui
        src/backend/boardmanager.cpp
        src/backend/gamepiece.cpp
        src/backend/node.cpp
)

qt_add_executable(
    shax-desktop-client
    WIN32 MACOSX_BUNDLE
    ${PROJECT_SOURCES}
    images.qrc
)

qt_add_translations(
    shax-desktop-client
    TS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(shax-desktop-client 
    PRIVATE 
    Qt::Widgets 
    Qt::WebSockets
)

install(TARGETS shax-desktop-client
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET shax-desktop-client
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})