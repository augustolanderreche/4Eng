QT += widgets sql
CONFIG += c++17

TARGET = 4eng-Login
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    registerwindow.cpp \
    adminwindow.cpp \
    empresawindow.cpp \
    userwindow.cpp

HEADERS += \
    adminwindow.h \
    databaseconfig.h \
    localdbconfig.h \
    empresawindow.h \
    mainwindow.h \
    registerwindow.h \
    userwindow.h
