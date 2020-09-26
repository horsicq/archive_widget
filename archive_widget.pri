QT       += concurrent

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

FORMS += \
    $$PWD/archive_widget.ui \
    $$PWD/dialogcreateviewmodel.ui

HEADERS += \
    $$PWD/archive_widget.h \
    $$PWD/dialogcreateviewmodel.h

SOURCES += \
    $$PWD/archive_widget.cpp \
    $$PWD/dialogcreateviewmodel.cpp

!contains(XCONFIG, xarchives) {
    XCONFIG += xarchives
    include($$PWD/../XArchive/xarchives.pri)
}
