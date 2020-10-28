QT       += concurrent

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

FORMS += \
    $$PWD/archive_widget.ui \
    $$PWD/dialogarchive.ui \
    $$PWD/dialogcreateviewmodel.ui \
    $$PWD/dialogshowimage.ui \
    $$PWD/dialogshowtext.ui \
    $$PWD/dialogunpackfile.ui

HEADERS += \
    $$PWD/archive_widget.h \
    $$PWD/createviewmodelprocess.h \
    $$PWD/dialogarchive.h \
    $$PWD/dialogcreateviewmodel.h \
    $$PWD/dialogshowimage.h \
    $$PWD/dialogshowtext.h \
    $$PWD/dialogunpackfile.h \
    $$PWD/unpackfileprocess.h

SOURCES += \
    $$PWD/archive_widget.cpp \
    $$PWD/createviewmodelprocess.cpp \
    $$PWD/dialogarchive.cpp \
    $$PWD/dialogcreateviewmodel.cpp \
    $$PWD/dialogshowimage.cpp \
    $$PWD/dialogshowtext.cpp \
    $$PWD/dialogunpackfile.cpp \
    $$PWD/unpackfileprocess.cpp

!contains(XCONFIG, xarchives) {
    XCONFIG += xarchives
    include($$PWD/../XArchive/xarchives.pri)
}
