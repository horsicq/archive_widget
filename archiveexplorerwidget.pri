INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/archiveexplorerwidget.h

SOURCES += \
    $$PWD/archiveexplorerwidget.cpp

FORMS += \
    $$PWD/archiveexplorerwidget.ui

!contains(XCONFIG, use_archive) {
    XCONFIG += use_archive
}

!contains(XCONFIG, xmodel_archiverecords) {
    XCONFIG += xmodel_archiverecords
    include($$PWD/../Controls/xmodel_archiverecords.pri)
}

!contains(XCONFIG, xtableview) {
    XCONFIG += xtableview
    include($$PWD/../Controls/xtableview.pri)
}

!contains(XCONFIG, xarchives) {
    XCONFIG += xarchives
    include($$PWD/../XArchive/xarchives.pri)
}

!contains(XCONFIG, xshortcuts) {
    XCONFIG += xshortcuts
    include($$PWD/../XShortcuts/xshortcuts.pri)
}

DISTFILES += \
    $$PWD/archiveexplorerwidget.cmake
