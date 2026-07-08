cmake_minimum_required(VERSION 3.20)

project(OfflineAIStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/core/agent.h
    src/core/agent.cpp
    src/core/tool.h
    src/core/tool.cpp
    src/core/llmclient.h
    src/core/llmclient.cpp
    src/core/promptbuilder.h
    src/core/promptbuilder.cpp
    src/core/orchestrator.h
    src/core/orchestrator.cpp
    src/agents/fileagent.h
    src/agents/fileagent.cpp
    src/agents/computeragent.h
    src/agents/computeragent.cpp
    src/ui/chatwidget.h
    src/ui/chatwidget.cpp
    src/ui/agentselector.h
    src/ui/agentselector.cpp
    src/ui/settingspanel.h
    src/ui/settingspanel.cpp
    resources/styles.qrc
)

add_executable(OfflineAIStudio
    ${PROJECT_SOURCES}
)

target_link_libraries(OfflineAIStudio PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

target_include_directories(OfflineAIStudio PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/agents
    ${PROJECT_SOURCE_DIR}/src/ui
    ${PROJECT_SOURCE_DIR}/src/utils
)

set_target_properties(OfflineAIStudio PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
