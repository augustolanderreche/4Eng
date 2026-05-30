QT += widgets network sql
CONFIG += c++17

TARGET = 4eng-Login
TEMPLATE = app

SOURCES += \
    main.cpp \
    api_client.cpp \
    localdbmanager.cpp \
    mainwindow.cpp \
    registerwindow.cpp \
    adminwindow.cpp \
    empresawindow.cpp \
    userwindow.cpp

HEADERS += \
    api_client.h \
    localdbmanager.h \
    adminwindow.h \
    empresawindow.h \
    mainwindow.h \
    registerwindow.h \
    userwindow.h
