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

# The legacy QHexView dialog wrapper references removed process-dialog APIs.
# Its core view is self-contained, read-only capable, and provides both hex and
# ASCII columns without pulling the stale wrapper dependencies.
include_directories(${CMAKE_CURRENT_LIST_DIR}/../QHexView)
if (NOT DEFINED QHEXVIEW_CORE_SOURCES)
    set(QHEXVIEW_CORE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/../QHexView/qhexview.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../QHexView/qhexview.h
    )
endif()
set(ARCHIVEEXPLORERWIDGET_SOURCES ${ARCHIVEEXPLORERWIDGET_SOURCES} ${QHEXVIEW_CORE_SOURCES})

set(ARCHIVEEXPLORERWIDGET_SOURCES
    ${ARCHIVEEXPLORERWIDGET_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/archiveexplorerwidget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/archiveexplorerwidget.h
    ${CMAKE_CURRENT_LIST_DIR}/archiveexplorerwidget.ui
    )
