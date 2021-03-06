QT       += concurrent

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

!contains(XCONFIG, use_dex) {
    XCONFIG += use_dex
}

!contains(XCONFIG, use_archive) {
    XCONFIG += use_archive
}

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

!contains(XCONFIG, allformatwidgets) {
    XCONFIG += allformatwidgets
    include($$PWD/../FormatWidgets/allformatwidgets.pri)
}

