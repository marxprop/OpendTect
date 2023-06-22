#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_ADD_QTOPENSSL_HINT )
    if ( EXISTS "${QT_DIR}" )
	get_filename_component( QTINSTDIR ${QT_DIR} REALPATH )
	set( OPENSSL_HINT_DIR "${QTINSTDIR}/../../../../../Tools/OpenSSL" )
	get_filename_component( OPENSSL_HINT_DIR ${OPENSSL_HINT_DIR} REALPATH )
	if ( EXISTS ${OPENSSL_HINT_DIR} )
	    set( OPENSSL_ROOT_DIR ${OPENSSL_HINT_DIR} )
	    if ( WIN32 )
		set( OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR}/Win_x64 )
	    elseif( NOT DEFINED APPLE )
		set( OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR}/binary )
	    endif( WIN32 )
	endif()
    endif( EXISTS "${QT_DIR}" )
endmacro( OD_ADD_QTOPENSSL_HINT )

macro( OD_SETUP_OPENSSL_TARGET TRGT )
    get_target_property( OPENSSL_CONFIGS ${TRGT} IMPORTED_CONFIGURATIONS )
    get_target_property( OPENSSL_LOCATION ${TRGT} IMPORTED_LOCATION )
    foreach( config ${OPENSSL_CONFIGS} )
	get_target_property( OPENSSL_LOCATION_${config} ${TRGT} IMPORTED_LOCATION_${config} )
	if ( OPENSSL_LOCATION_${config} )
	    if ( WIN32 )
		set( OPENSSL_IMPLIB_LOCATION_${config} "${OPENSSL_LOCATION_${config}}" )
		od_get_dll( "${OPENSSL_IMPLIB_LOCATION_${config}}" OPENSSL_LOCATION_${config} )
	    else()
		get_filename_component( OPENSSL_LOCATION_${config} "${OPENSSL_LOCATION_${config}}" REALPATH )
	    endif()
	    set_target_properties( ${TRGT} PROPERTIES
		IMPORTED_LOCATION_${config} "${OPENSSL_LOCATION_${config}}" )
	    if ( NOT OPENSSL_LOCATION )
		set( OPENSSL_LOCATION "${OPENSSL_LOCATION_${config}}" )
	    endif()
	    unset( OPENSSL_IMPORTED_LOCATION_${config} )
	    if ( WIN32 )
		set_target_properties( ${TRGT} PROPERTIES
		    IMPORTED_IMPLIB_${config} "${OPENSSL_IMPLIB_LOCATION_${config}}" )
		unset( OPENSSL_IMPLIB_LOCATION_${config} )
	    endif()
	endif()
    endforeach()
    unset( OPENSSL_CONFIGS )
    if ( NOT OPENSSL_LOCATION )
	message( FATAL_ERROR "Cannot retrieve OpenSSL import location" )
    endif()
    if ( WIN32 )
	set( OPENSSL_IMPLIB_LOCATION "${OPENSSL_LOCATION}" )
	od_get_dll( "${OPENSSL_IMPLIB_LOCATION}" OPENSSL_LOCATION )
    else()
	get_filename_component( OPENSSL_LOCATION "${OPENSSL_LOCATION}" REALPATH )
    endif()
    set_target_properties( ${TRGT} PROPERTIES
			   IMPORTED_LOCATION "${OPENSSL_LOCATION}" )
    unset( OPENSSL_LOCATION )
    if ( WIN32 )
	set_target_properties( ${TRGT} PROPERTIES
			       IMPORTED_IMPLIB "${OPENSSL_IMPLIB_LOCATION}" )
	unset( OPENSSL_IMPLIB_LOCATION )
    endif()
endmacro( OD_SETUP_OPENSSL_TARGET )

macro( OD_FIND_OPENSSL )
    if ( NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto )
	find_package( OpenSSL 1.1.1 QUIET COMPONENTS SSL Crypto GLOBAL )
	if ( NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto )
	    OD_ADD_QTOPENSSL_HINT()
	    unset( OPENSSL_FOUND )
	    unset( OPENSSL_INCLUDE_DIR CACHE )
	    unset( OPENSSL_CRYPTO_LIBRARY CACHE )
	    unset( OPENSSL_SSL_LIBRARY CACHE )
	    unset( OPENSSL_SSL_VERSION CACHE )
	    find_package( OpenSSL 1.1.1 QUIET COMPONENTS SSL Crypto GLOBAL )
	endif()
	OD_SETUP_OPENSSL_TARGET( OpenSSL::SSL )
	OD_SETUP_OPENSSL_TARGET( OpenSSL::Crypto )
    endif()
endmacro(OD_FIND_OPENSSL)

macro( OD_SETUP_OPENSSLCOMP COMP )

    if ( TARGET OpenSSL::${COMP} )
	list( APPEND OD_MODULE_COMPILE_DEFINITIONS
	      "__OpenSSL_${COMP}_LIBRARY__=\"$<TARGET_FILE:OpenSSL::${COMP}>\"" )
    else()
	message( WARNING "OpenSSL component ${COMP} is not available.")
    endif()

endmacro( OD_SETUP_OPENSSLCOMP )

macro( OD_SETUP_CRYPTO )

    if ( EXISTS "${OPENSSL_CRYPTO_LIBRARY}" )
	if ( OD_LINKCRYPTO )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS OpenSSL::Crypto )
	elseif ( OD_USECRYPTO )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS OpenSSL::Crypto )
	    OD_SETUP_OPENSSLCOMP( Crypto )
	endif()
    endif()

endmacro( OD_SETUP_CRYPTO )

macro( OD_SETUP_OPENSSL )

    if ( EXISTS "${OPENSSL_SSL_LIBRARY}" )
	if ( OD_LINKOPENSSL )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS OpenSSL::SSL )
	elseif( OD_USEOPENSSL )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS OpenSSL::SSL )
	    OD_SETUP_OPENSSLCOMP( SSL )
	endif()
    endif()

endmacro( OD_SETUP_OPENSSL )

