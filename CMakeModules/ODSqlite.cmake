#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_SETUP_SQLITE_TARGET )
    add_library( SQLite::SQLite3 SHARED IMPORTED GLOBAL )
    if ( WIN32 )
	set( SQLITE_LIB_LOCATION_RELEASE "${LIBSQLITE}" )
	od_get_dll( "${LIBSQLITE}" SQLITE_LOCATION_RELEASE )
    else()
	get_filename_component( SQLITE_SONAME_RELEASE "${LIBSQLITE}" NAME )
	get_filename_component( SQLITE_LOCATION_RELEASE "${LIBSQLITE}" REALPATH )
    endif()
    set_target_properties( SQLite::SQLite3 PROPERTIES
	IMPORTED_LOCATION "${SQLITE_LOCATION_RELEASE}"
	IMPORTED_CONFIGURATIONS "RELEASE"
	IMPORTED_LOCATION_RELEASE "${SQLITE_LOCATION_RELEASE}" )
    unset( SQLITE_LOCATION_RELEASE )
    if ( WIN32 )
	set_target_properties( SQLite::SQLite3 PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${SQLITE_LIB_LOCATION_RELEASE}" )
	unset( SQLITE_LIB_LOCATION_RELEASE )
    else()
	set_target_properties( SQLite::SQLite3 PROPERTIES
		IMPORTED_SONAME_RELEASE "${SQLITE_SONAME_RELEASE}" )
	unset( SQLITE_SONAME_RELEASE )
    endif()
    unset( LIBSQLITE CACHE )
endmacro()

macro( OD_ADD_SQLITE )
    find_package( SQLite3 QUIET GLOBAL )
    if ( NOT SQLite3_FOUND )
	unset( SQLite3_INCLUDE_DIR CACHE )
	unset( SQLite3_LIBRARY CACHE )
	#Setting the target from the library only
	if ( DEFINED SQLITE_DIR )
	    if ( UNIX )
		SET( LIBSEARCHPATHS "${SQLITE_DIR}/lib64;${SQLITE_DIR}/lib" )
	    else()
		SET( LIBSEARCHPATHS "${SQLITE_DIR}/lib" )
	    endif()
	endif()

	if ( WIN32 )
	    od_find_library( LIBSQLITE sqlite3.lib )
	elseif ( APPLE )
	    SET( LIBSEARCHPATHS /usr/lib/sqlite3 )
	    od_find_library( LIBSQLITE libtclsqlite3.dylib )
	else()
	    od_find_library( LIBSQLITE libsqlite3.so.0 )
	endif()

	if ( NOT EXISTS "${LIBSQLITE}" )
	    set( SQLITE_DIR "" CACHE PATH "SQLite3 location" )
	    message( FATAL_ERROR "SQLITE_DIR is not defined" )
	endif()

	OD_SETUP_SQLITE_TARGET()
    endif()
endmacro()

macro ( OD_SETUP_SQLITE )

    if ( TARGET SQLite::SQLite3 )
	if ( OD_LINKSQLITE )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS SQLite::SQLite3 )
	elseif ( OD_USESQLITE )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS SQLite::SQLite3 )
	endif()
    endif()
endmacro( OD_SETUP_SQLITE )
