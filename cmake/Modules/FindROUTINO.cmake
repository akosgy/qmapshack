# - Try to find ROUTINO
# Once done this will define
#
#  ROUTINO_FOUND - system has ROUTINO
#  ROUTINO_INCLUDE_DIRS - the ROUTINO include directory
#  ROUTINO_LIBRARIES - Link these to use ROUTINO
#  ROUTINO_DEFINITIONS - Compiler switches required for using ROUTINO
#  ROUTINO_XML_PATH - Path to profile.xml and translation.xml
#
#  Copyright (c) 2009 Andreas Schneider <mail@cynapses.org>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (ROUTINO_LIBRARIES AND ROUTINO_INCLUDE_DIRS AND ROUTINO_XML_PATH)
    # in cache already
    set(ROUTINO_FOUND TRUE)
else (ROUTINO_LIBRARIES AND ROUTINO_INCLUDE_DIRS AND ROUTINO_XML_PATH)

    find_path(ROUTINO_INCLUDE_DIR
        NAMES
            routino.h
        PATHS
            if(WIN32)
            ${ROUTINO_DEV_PATH}
            endif(WIN32)
            /usr/include
            /usr/local/include
            /opt/local/include
            /sw/include
            ${CMAKE_SOURCE_DIR}/Win32/Routino/include
    )
    mark_as_advanced(ROUTINO_INCLUDE_DIR)

    find_library(LIBROUTINO_LIBRARY
        NAMES
            routino
        PATHS
            if(WIN32)
            ${ROUTINO_DEV_PATH}
            endif(WIN32)
            /usr/lib
            /usr/local/lib
            /opt/local/lib
            /sw/lib
            ${CMAKE_SOURCE_DIR}/Win32/Routino/lib
    )
    mark_as_advanced(LIBROUTINO_LIBRARY)

    find_path(ROUTINO_XML_PATH
        NAMES
            profiles.xml
            translations.xml
        PATHS
            /usr/share
            /usr/local/share
            /opt/local/share
            ${ROUTINO_DEV_PATH}
        PATH_SUFFIXES
            routino
    )
    mark_as_advanced(ROUTINO_XML_PATH)

    if (LIBROUTINO_LIBRARY)
        set(LIBROUTINO_FOUND TRUE)
    endif (LIBROUTINO_LIBRARY)

    set(ROUTINO_INCLUDE_DIRS
        ${ROUTINO_INCLUDE_DIR}
    )

    if (LIBROUTINO_FOUND)
        set(ROUTINO_LIBRARIES ${ROUTINO_LIBRARIES}  ${LIBROUTINO_LIBRARY})
    endif (LIBROUTINO_FOUND)

    if (ROUTINO_INCLUDE_DIRS AND ROUTINO_LIBRARIES AND ROUTINO_XML_PATH)
        set(ROUTINO_FOUND TRUE)
    endif (ROUTINO_INCLUDE_DIRS AND ROUTINO_LIBRARIES AND ROUTINO_XML_PATH)

    if (ROUTINO_FOUND)
        if (NOT ROUTINO_FIND_QUIETLY)
            message(STATUS "Found ROUTINO: ${ROUTINO_LIBRARIES}")
        endif (NOT ROUTINO_FIND_QUIETLY)
    else (ROUTINO_FOUND)
        if (ROUTINO_FIND_REQUIRED)
            message(FATAL_ERROR "Could not find ROUTINO")
        endif (ROUTINO_FIND_REQUIRED)
    endif (ROUTINO_FOUND)

    # show the ROUTINO_INCLUDE_DIRS and ROUTINO_LIBRARIES variables only in the advanced view
    mark_as_advanced(ROUTINO_INCLUDE_DIRS ROUTINO_LIBRARIES ROUTINO_XML_PATH)

endif (ROUTINO_LIBRARIES AND ROUTINO_INCLUDE_DIRS AND ROUTINO_XML_PATH)
