include_directories(${CMAKE_CURRENT_LIST_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/../FormatWidgets/allformatwidgets.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../XArchive/xarchives.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../XShortcuts/xshortcuts.cmake)

set(ARCHIVE_WIDGET_SOURCES
    ${ALLFORMATWIDGETS_SOURCES}
    ${XARCHIVES_SOURCES}
    ${XSHORTCUTS_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/archive_widget.cmake
    ${CMAKE_CURRENT_LIST_DIR}/archive_widget.cpp
    ${CMAKE_CURRENT_LIST_DIR}/archive_widget.ui
    ${CMAKE_CURRENT_LIST_DIR}/createviewmodelprocess.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dialogarchive.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dialogarchive.ui
    ${CMAKE_CURRENT_LIST_DIR}/dialogcreateviewmodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dialogshowimage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dialogshowimage.ui
    ${CMAKE_CURRENT_LIST_DIR}/dialogunpackfile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/unpackfileprocess.cpp
    )
