#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_SETUP_BREAKPAD_TARGET TRGT )
    if( BREAKPAD${MOD}_RELEASE AND EXISTS "${BREAKPAD${MOD}_RELEASE}" )
	list( APPEND BREAKPAD_CONFIGS RELEASE )
	set( BREAKPAD_LOCATION_RELEASE "${BREAKPAD${MOD}_RELEASE}" )
	set( BREAKPAD_LOCATION "${BREAKPAD_LOCATION_RELEASE}" )
	unset( BREAKPAD${MOD}_RELEASE CACHE )
    endif()
    if ( BREAKPAD${MOD}_DEBUG AND EXISTS "${BREAKPAD${MOD}_DEBUG}" )
	list( APPEND BREAKPAD_CONFIGS DEBUG )
	set( BREAKPAD_LOCATION_DEBUG "${BREAKPAD${MOD}_DEBUG}" )
	if ( NOT BREAKPAD_LOCATION )
	    set( BREAKPAD_LOCATION "${BREAKPAD_LOCATION_DEBUG}" )
	endif()
	unset( BREAKPAD${MOD}_DEBUG CACHE )
    endif()
    if ( NOT BREAKPAD_LOCATION )
	message( FATAL_ERROR "BREAKPAD${MOD} (${LIBNAME}) is missing" )
    endif()
    if ( NOT IS_DIRECTORY "${BREAKPAD_DIR}/include/breakpad" )
	message( FATAL_ERROR "Cannot find breakpad header files at ${BREAKPAD_DIR}/include/breakpad" )
    endif()
    add_library( breakpad::${TRGT} STATIC IMPORTED GLOBAL )
    set_target_properties( breakpad::${TRGT} PROPERTIES
	IMPORTED_LOCATION "${BREAKPAD_LOCATION}"
	IMPORTED_CONFIGURATIONS "${BREAKPAD_CONFIGS}"
	INTERFACE_INCLUDE_DIRECTORIES "${BREAKPAD_DIR}/include/breakpad" )
    unset( BREAKPAD_LOCATION )
    foreach( config ${BREAKPAD_CONFIGS} )
	set_target_properties( breakpad::${TRGT} PROPERTIES
		IMPORTED_LOCATION_${config} "${BREAKPAD_LOCATION_${config}}" )
	unset( BREAKPAD_LOCATION_${config} )
    endforeach()
    unset( BREAKPAD_CONFIGS )
endmacro()

macro( OD_ADD_BREAKPAD )
   if ( OD_ENABLE_BREAKPAD )
	set( BREAKPAD_DIR "" CACHE PATH "BREAKPAD Location (upto the 'src' directory)" )
	if( WIN32 )
	    set( BREAKPADMODULES COMMON CLIENT HANDLER )
	    set( BREAKPADNAMES common crash_generation_client exception_handler )
	    set( BREAKPADCOMPS breakpad client handler )
	else()
	    set( BREAKPADMODULES COMMON CLIENT )
	    set( BREAKPADNAMES libbreakpad.a libbreakpad_client.a )
	    set( BREAKPADCOMPS breakpad client )
	endif()

	foreach( MOD LIBNAME TRGT IN ZIP_LISTS BREAKPADMODULES BREAKPADNAMES BREAKPADCOMPS )
	    find_library( BREAKPAD${MOD}_RELEASE NAMES ${LIBNAME}
			  PATHS "${BREAKPAD_DIR}"
			  PATH_SUFFIXES lib lib/Release )
	    find_library( BREAKPAD${MOD}_DEBUG NAMES ${LIBNAME}
			  PATHS "${BREAKPAD_DIR}"
			  PATH_SUFFIXES lib/Debug )
	    OD_SETUP_BREAKPAD_TARGET( ${TRGT} )
	endforeach()

	find_program( BREAKPAD_DUMPSYMS_EXECUTABLE NAMES dump_syms
		      PATHS ${BREAKPAD_DIR}/bin )

	install( DIRECTORY "${OD_BINARY_BASEDIR}/${OD_RUNTIME_DIRECTORY}/symbols"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}" )

   endif()
endmacro(OD_ADD_BREAKPAD)

macro( OD_SETUP_BREAKPAD )

    if( OD_ENABLE_BREAKPAD )

	if ( NOT TARGET breakpad::breakpad )
	    message( STATUS "Cannot link against breakpad::breakpad" )
	endif()
	if ( NOT TARGET breakpad::client )
	    message( STATUS "Cannot link against breakpad::client" )
	endif()
	if ( WIN32 AND NOT TARGET breakpad::handler )
	    message( STATUS "Cannot link against breakpad::handler" )
	endif()

	list(APPEND OD_MODULE_EXTERNAL_LIBS
			breakpad::breakpad
			breakpad::client )
	if ( WIN32 )
	    list(APPEND OD_MODULE_EXTERNAL_LIBS breakpad::handler )
	endif()
	add_definitions( -DHAS_BREAKPAD )

    endif(OD_ENABLE_BREAKPAD)

endmacro(OD_SETUP_BREAKPAD)

macro ( OD_GENERATE_SYMBOLS TRGT )
    if ( OD_ENABLE_BREAKPAD AND EXISTS "${BREAKPAD_DUMPSYMS_EXECUTABLE}" )
	#Method to create timestamp file (and symbols). Will only trigger if out of date
	add_custom_command( TARGET ${TRGT} POST_BUILD 
		COMMAND ${CMAKE_COMMAND}
			-DLIBRARY=$<TARGET_FILE:${TRGT}>
			-DSYM_DUMP_EXECUTABLE=${BREAKPAD_DUMPSYMS_EXECUTABLE}
			-P ${OpendTect_DIR}/CMakeModules/GenerateSymbols.cmake
		DEPENDS ${TRGT}
		COMMENT "Generating symbols for ${TRGT}" )
    endif()
endmacro()
