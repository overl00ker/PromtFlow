cmake_minimum_required(VERSION 3.20)
project(PromptFlow LANGUAGES CXX)

set(QT_COMPILER "msvc2022_64" CACHE STRING "Qt compiler subdirectory")
set(QT_ROOT     "D:/Qt/6.10.0" CACHE PATH   "Root Qt installation path")
set(CMAKE_PREFIX_PATH "${QT_ROOT}/${QT_COMPILER}" CACHE PATH "Qt prefix" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

set(SOURCES
    main.cpp
    src/resources/app_icon.rc
    src/resources/resources.qrc
    src/app/MainWindow.cpp
    src/app/SettingsDialog.cpp
    src/graph/NodeItem.cpp
    src/graph/ConnectionLine.cpp
    src/graph/NodeScene.cpp
    src/nodes/TextPromptNode.cpp
    src/nodes/ImageRefNode.cpp
    src/nodes/JsonPromptNode.cpp
    src/nodes/GenerationNode.cpp
    src/nodes/StylePresetNode.cpp
    src/network/GeminiApiClient.cpp
    src/widgets/ClickableLabel.cpp
    src/widgets/HistoryPanel.cpp
)

set(HEADERS
    src/app/MainWindow.h
    src/app/SettingsDialog.h
    src/graph/NodeItem.h
    src/graph/ConnectionLine.h
    src/graph/NodeScene.h
    src/nodes/TextPromptNode.h
    src/nodes/ImageRefNode.h
    src/nodes/JsonPromptNode.h
    src/nodes/GenerationNode.h
    src/nodes/StylePresetNode.h
    src/network/GeminiApiClient.h
    src/widgets/ClickableLabel.h
    src/widgets/HistoryPanel.h
    src/app/HistoryEntry.h
)

qt_add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
)

# Allow #include "SomeHeader.h" to resolve from any src/ subdirectory
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/graph
    ${CMAKE_SOURCE_DIR}/src/nodes
    ${CMAKE_SOURCE_DIR}/src/network
    ${CMAKE_SOURCE_DIR}/src/widgets
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Network)

find_program(WINDEPLOYQT_EXECUTABLE windeployqt
    HINTS "${QT_ROOT}/${QT_COMPILER}/bin"
)

if(WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Running windeployqt..."
    )
else()
    message(WARNING "windeployqt not found")
endif()
