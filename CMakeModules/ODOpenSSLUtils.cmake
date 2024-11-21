#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_SETUP_OPENSSL_PROG )
    set( OPENSSL_EXEC "${OPENSSL_ROOT_DIR}/bin/openssl" )
    if ( WIN32 )
	set( OPENSSL_EXEC "${OPENSSL_EXEC}.exe" )
    endif()
    
endmacro( OD_SETUP_OPENSSL_PROG )

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

macro( OD_FIND_OPENSSL )
    if ( NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto )
	find_package( OpenSSL 1.1.1 QUIET COMPONENTS SSL Crypto )
	if ( NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto )
	    OD_ADD_QTOPENSSL_HINT()
	    unset( OPENSSL_FOUND )
	    unset( OPENSSL_INCLUDE_DIR CACHE )
	    unset( OPENSSL_CRYPTO_LIBRARY CACHE )
	    unset( OPENSSL_SSL_LIBRARY CACHE )
	    unset( OPENSSL_SSL_VERSION CACHE )
	    find_package( OpenSSL 1.1.1 QUIET COMPONENTS SSL Crypto )
	endif()
    endif()
endmacro(OD_FIND_OPENSSL)

macro( OD_SETUP_OPENSSLCOMP COMP )

    if ( TARGET OpenSSL::${COMP} )
	OD_READ_TARGETINFO( OpenSSL::${COMP} )
	if ( EXISTS "${SOURCEFILE}" )
	    get_filename_component( SOURCEFILE "${SOURCEFILE}" REALPATH )
	    if ( UNIX )
		get_filename_component( SOURCEPATH "${SOURCEFILE}" DIRECTORY )
		add_definitions( -D__OpenSSL_${COMP}_PATH__="${SOURCEPATH}" )
		unset( SOURCEPATH )
	    endif()
	    get_filename_component( SOURCEFILE "${SOURCEFILE}" NAME )
	    add_definitions( -D__OpenSSL_${COMP}_LIBRARY__="${SOURCEFILE}" )
	endif()
    else()
	message( WARNING "OpenSSL component ${COMP} is not available.")
    endif()

endmacro( OD_SETUP_OPENSSLCOMP )

macro( OD_SETUP_CRYPTO )

    if ( EXISTS "${OPENSSL_CRYPTO_LIBRARY}" )
	if ( OD_LINKCRYPTO )
	    list( APPEND OD_MODULE_INCLUDESYSPATH ${OpenSSL_INCLUDE_DIR} )
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
	    list( APPEND OD_MODULE_INCLUDESYSPATH ${OpenSSL_INCLUDE_DIR} )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS OpenSSL::SSL )
	elseif( OD_USEOPENSSL )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS OpenSSL::SSL )
	    OD_SETUP_OPENSSLCOMP( SSL )
	endif()
	OD_SETUP_OPENSSL_PROG()
	if ( EXISTS "${OPENSSL_EXEC}" )
	    add_definitions( -D__OPENSSL_EXEC__="${OPENSSL_EXEC}" )
	endif()
	if ( UNIX AND NOT APPLE AND (OD_LINKCRYPTO OR OD_LINKOPENSSL) )
	    OD_INSTALL_DATADIR_MOD( "OpenSSL" )
	endif()
    endif()

endmacro( OD_SETUP_OPENSSL )

