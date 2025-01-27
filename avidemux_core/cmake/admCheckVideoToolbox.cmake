MACRO(checkVideoToolbox)

OPTION(VIDEOTOOLBOX "" ON)

MESSAGE(STATUS "Checking for VideoToolbox")
MESSAGE(STATUS "****************")

IF(VIDEOTOOLBOX)
        FIND_HEADER_AND_LIB(VIDEOTOOLBOX VideoToolbox/VideoToolbox.h )
        IF(VIDEOTOOLBOX_FOUND)
                MESSAGE(STATUS "Assuming VideoToolbox is usable")
                PRINT_LIBRARY_INFO("VIDEOTOOLBOX" VIDEOTOOLBOX_FOUND "${VIDEOTOOLBOX_INCLUDE_DIR}" "${VIDEOTOOLBOX_LIBRARY_DIR}")
                SET(USE_VIDEOTOOLBOX True CACHE BOOL "")
        ELSE()
                MESSAGE("${MSG_DISABLE_OPTION}")
        ENDIF()
ELSE()
        MESSAGE("${MSG_DISABLE_OPTION}")
ENDIF()

MESSAGE("")
APPEND_SUMMARY_LIST("Miscellaneous" "VIDEOTOOLBOX" "${USE_VIDEOTOOLBOX}")
ENDMACRO()
