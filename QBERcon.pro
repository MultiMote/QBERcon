QT += core widgets network

TARGET = QBERcon

TEMPLATE = app

INCLUDEPATH += src example

SOURCES += \
    example/main.cpp \
    example/examplegui.cpp \
    src/QBERcon.cpp

HEADERS += \
    example/examplegui.h \
    src/QBERcon.h

FORMS += \
    example/examplegui.ui
