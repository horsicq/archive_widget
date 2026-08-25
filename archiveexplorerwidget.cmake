include_directories(${CMAKE_CURRENT_LIST_DIR})

if (NOT DEFINED XMODEL_ARCHIVERECORDS_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../Controls/xmodel_archiverecords.cmake)
    set(ARCHIVEEXPLORERWIDGET_SOURCES ${ARCHIVEEXPLORERWIDGET_SOURCES} ${XMODEL_ARCHIVERECORDS_SOURCES})
endif()

if (NOT DEFINED XTABLEVIEW_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../Controls/xtableview.cmake)
    set(ARCHIVEEXPLORERWIDGET_SOURCES ${ARCHIVEEXPLORERWIDGET_SOURCES} ${XTABLEVIEW_SOURCES})
endif()

if (NOT DEFINED XARCHIVES_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../XArchive/xarchives.cmake)
    set(ARCHIVEEXPLORERWIDGET_SOURCES ${ARCHIVEEXPLORERWIDGET_SOURCES} ${XARCHIVES_SOURCES})
endif()

if (NOT DEFINED XSHORTCUTS_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../XShortcuts/xshortcuts.cmake)
    set(ARCHIVEEXPLORERWIDGET_SOURCES ${ARCHIVEEXPLORERWIDGET_SOURCES} ${XSHORTCUTS_SOURCES})
endif()

if (NOT DEFINED SEARCHSTRINGSWIDGET_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../FormatWidgets/SearchStrings/searchstringswidget.cmake)
    set(ARCHIVEEXPLORERWIDGET_SOURCES ${ARCHIVEEXPLORERWIDGET_SOURCES} ${SEARCHSTRINGSWIDGET_SOURCES})
endif()

if (NOT DEFINED XENTROPYWIDGET_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../XEntropyWidget/xentropywidget.cmake)
    set(ARCHIVEEXPLORERWIDGET_SOURCES ${ARCHIVEEXPLORERWIDGET_SOURCES} ${XENTROPYWIDGET_SOURCES})
endif()

# Hex view of a record: the shared XHexView widget (via its DialogHexView wrapper),
# the same one the sibling archive_widget and the format widgets use.
#
# xhexview.cmake is a kitchen-sink that also pulls the YARA scanner (yara_widget + XYara) for a
# hex-view context-menu action that is #ifdef USE_YARA. XFileUnpacker does not define USE_YARA and
# does not link the yara library, so pre-empt those two source sets (their guards then skip them);
# otherwise xyara.cpp compiles and leaves unresolved yr_* externals at link time.
if (NOT DEFINED YARA_WIDGET_SOURCES)
    set(YARA_WIDGET_SOURCES "")
endif()
if (NOT DEFINED XYARA_SOURCES)
    set(XYARA_SOURCES "")
endif()
if (NOT DEFINED XHEXVIEW_SOURCES)
    include(${CMAKE_CURRENT_LIST_DIR}/../XHexView/xhexview.cmake)
    set(ARCHIVEEXPLORERWIDGET_SOURCES ${ARCHIVEEXPLORERWIDGET_SOURCES} ${XHEXVIEW_SOURCES})
endif()

set(ARCHIVEEXPLORERWIDGET_SOURCES
    ${ARCHIVEEXPLORERWIDGET_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/archiveexplorerwidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/archiveexplorerwidget.h
    ${CMAKE_CURRENT_LIST_DIR}/archiveexplorerwidget.ui
    )
