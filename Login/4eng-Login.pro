QT += widgets sql
CONFIG += c++17

TARGET = 4eng-Login
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    registerwindow.cpp \
    empresawindow.cpp \
    userwindow.cpp

HEADERS += \
    databaseconfig.h \
    empresawindow.h \
    mainwindow.h \
    registerwindow.h \
    userwindow.h
