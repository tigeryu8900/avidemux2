MACRO(checkOpusDec)
	IF(NOT OPUS_CHECKED)
		OPTION(OPUS "" ON)

		MESSAGE(STATUS "Checking for OPUS")
		MESSAGE(STATUS "*****************")

		IF(OPUS)
                        FIND_HEADER_AND_LIB(LIBOPUS opus/opus_multistream.h opus opus_multistream_decoder_init ${LIBOPUS_REQUIRED_FLAGS})
	                IF(LIBOPUS_FOUND)
				SET(LIBOPUS_FOUND 1)
				SET(USE_LIBOPUS True CACHE BOOL "")
				SET(LIBOPUS_INCLUDE_DIR "${LIBOPUS_INCLUDE_DIR}")
				SET(LIBOPUS_LIBRARY_DIR "${LIBOPUS_LIBRARY_DIR}")
			ENDIF()

			PRINT_LIBRARY_INFO("LIBOPUS" LIBOPUS_FOUND "${LIBOPUS_INCLUDE_DIR}" "${LIBOPUS_LIBRARY_DIR}")
                ELSE()
                  MESSAGE("${MSG_DISABLE_OPTION}")
                ENDIF()
	ENDIF()
	APPEND_SUMMARY_LIST("Audio Decoder" "Opus" "${LIBOPUS_FOUND}")
ENDMACRO()
