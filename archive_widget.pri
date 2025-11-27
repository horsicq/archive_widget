QT       += concurrent

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

!contains(XCONFIG, use_dex) {
    XCONFIG += use_dex
}

!contains(XCONFIG, use_pdf) {
    XCONFIG += use_pdf
}

!contains(XCONFIG, use_archive) {
    XCONFIG += use_archive
}

HEADERS += \
    $$PWD/archive_widget.h \
    $$PWD/createviewmodelprocess.h \
    $$PWD/dialogarchive.h \
    $$PWD/dialogcreateviewmodel.h \
    $$PWD/dialogshowimage.h \
    $$PWD/dialogunpackfile.h \
    $$PWD/unpackfileprocess.h \
    $$PWD/xarchivewidget.h

SOURCES += \
    $$PWD/archive_widget.cpp \
    $$PWD/createviewmodelprocess.cpp \
    $$PWD/dialogarchive.cpp \
    $$PWD/dialogcreateviewmodel.cpp \
    $$PWD/dialogshowimage.cpp \
    $$PWD/dialogunpackfile.cpp \
    $$PWD/unpackfileprocess.cpp \
    $$PWD/xarchivewidget.cpp

FORMS += \
    $$PWD/archive_widget.ui \
    $$PWD/dialogarchive.ui \
    $$PWD/dialogshowimage.ui \
    $$PWD/xarchivewidget.ui

!contains(XCONFIG, allformatwidgets) {
    XCONFIG += allformatwidgets
    include($$PWD/../FormatWidgets/allformatwidgets.pri)
}

!contains(XCONFIG, xarchives) {
    XCONFIG += xarchives
    include($$PWD/../XArchive/xarchives.pri)
}

DISTFILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/archive_widget.cmake

