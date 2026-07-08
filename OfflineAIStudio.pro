#-------------------------------------------------
# Offline AI Studio - Qt Project File
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OfflineAIStudio
TEMPLATE = app

#-------------------------------------------------
# Source Files
#-------------------------------------------------
SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/core/agent.cpp \
    src/core/tool.cpp \
    src/core/llmclient.cpp \
    src/core/orchestrator.cpp \
    src/core/promptbuilder.cpp \
    src/agents/fileagent.cpp \
    src/agents/computeragent.cpp \
    src/ui/chatwidget.cpp \
    src/ui/agentselector.cpp \
    src/ui/settingspanel.cpp

#-------------------------------------------------
# Header Files
#-------------------------------------------------
HEADERS += \
    src/mainwindow.h \
    src/core/agent.h \
    src/core/tool.h \
    src/core/llmclient.h \
    src/core/orchestrator.h \
    src/core/promptbuilder.h \
    src/agents/fileagent.h \
    src/agents/computeragent.h \
    src/ui/chatwidget.h \
    src/ui/agentselector.h \
    src/ui/settingspanel.h

#-------------------------------------------------
# Resources
#-------------------------------------------------
RESOURCES += \
    resources/styles.qrc

#-------------------------------------------------
# Build Configuration
#-------------------------------------------------
CONFIG += c++20

# Release configuration
CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
    QMAKE_CXXFLAGS_RELEASE += -O2
}

# Debug configuration  
CONFIG(debug, debug|release) {
    DEFINES += QT_DEBUG
}

#-------------------------------------------------
# Include Paths
#-------------------------------------------------
INCLUDEPATH += \
    src \
    src/core \
    src/agents \
    src/ui

#-------------------------------------------------
# Output Directories
#-------------------------------------------------
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
UI_DIR = $$PWD/build/ui

#-------------------------------------------------
# Windows Specific
#-------------------------------------------------
win32 {
    # Set application icon (create a .ico file and uncomment)
    # RC_ICONS = resources/app.ico
    
    # Windows subsystem
    win32:CONFIG(release, debug|release): LIBS += -luser32 -lkernel32 -lshell32
}

#-------------------------------------------------
# Mac Specific
#-------------------------------------------------
macx {
    QMAKE_INFO_PLIST = resources/Info.plist
}

#-------------------------------------------------
# Linux Specific
#-------------------------------------------------
unix:!macx {
    target.path = /usr/local/bin
    INSTALLS += target
}
