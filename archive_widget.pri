INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

FORMS += \
    $$PWD/archive_widget.ui

HEADERS += \
    $$PWD/archive_widget.h

SOURCES += \
    $$PWD/archive_widget.cpp

!contains(XCONFIG, xarchives) {
    XCONFIG += xarchives
    include($$PWD/../XArchive/xarchives.pri)
}
