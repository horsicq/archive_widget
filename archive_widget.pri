QT       += concurrent

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

FORMS += \
    $$PWD/archive_widget.ui \
    $$PWD/dialogcreateviewmodel.ui \
    $$PWD/dialogunpackfile.ui

HEADERS += \
    $$PWD/archive_widget.h \
    $$PWD/createviewmodelprocess.h \
    $$PWD/dialogcreateviewmodel.h \
    $$PWD/dialogunpackfile.h \
    $$PWD/unpackfileprocess.h

SOURCES += \
    $$PWD/archive_widget.cpp \
    $$PWD/createviewmodelprocess.cpp \
    $$PWD/dialogcreateviewmodel.cpp \
    $$PWD/dialogunpackfile.cpp \
    $$PWD/unpackfileprocess.cpp

!contains(XCONFIG, xarchives) {
    XCONFIG += xarchives
    include($$PWD/../XArchive/xarchives.pri)
}
