#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

if ( NOT DEFINED QTDIR OR QTDIR STREQUAL "" )
    set( QTDIR "" CACHE PATH "QT Location" )
    option ( OD_NO_QT "Turn off all QT" NO )
    message( FATAL_ERROR "QTDIR is not defined" )
endif()

##Create launcher for linguist
set( LINGUIST_LAUNCHER "CMakeModules/templates/linguist.csh.in" )
if ( EXISTS ${LINGUIST_LAUNCHER} )
    configure_file( ${LINGUIST_LAUNCHER} dtect/linguist.csh @ONLY )
endif()

macro(ADD_TO_LIST_IF_NEW LISTNAME ITEMNAME)
    list( FIND ${LISTNAME} "${ITEMNAME}" ITMINDEX )
    if ( ${ITMINDEX} EQUAL -1 )
	list(APPEND ${LISTNAME} "${ITEMNAME}")
    endif()
endmacro(ADD_TO_LIST_IF_NEW)

#Try to find Qt5
list ( APPEND CMAKE_PREFIX_PATH ${QTDIR} )
find_package( Qt5Core QUIET PATHS ${QTDIR} NO_DEFAULT_PATH )

#Setup Qt5 Language tools
if ( Qt5Core_FOUND )
    find_package( Qt5 QUIET COMPONENTS LinguistTools )
    get_target_property( QT_LRELEASE_EXECUTABLE Qt5::lrelease
			 IMPORTED_LOCATION )
    get_target_property( QT_LUPDATE_EXECUTABLE Qt5::lupdate
			 IMPORTED_LOCATION )
endif( Qt5Core_FOUND )

cmake_policy( SET CMP0057 NEW )
if ( WIN32 )
    if ( POLICY CMP0020 )
	cmake_policy( SET CMP0020 NEW )
    endif()
endif()

macro( QT_SETUP_GUI )
    if ( UNIX )
	if ( APPLE )
	    #TODO: Write
	else()
	    od_find_library( LIBPNGLOC libpng16.so.16 libpng15.so.15 )
	    if ( LIBPNGLOC )
		OD_INSTALL_SYSTEM_LIBRARY( "${LIBPNGLOC}" )
		list( APPEND OD_THIRD_PARTY_LIBS "${LIBPNGLOC}" )
	    else()
		message( SEND_ERROR "Required system library not found: libpng" )
	    endif()
	    od_find_library( LIBFREETYPELOC libfreetype.so.6 )
	    if ( LIBFREETYPELOC )
		OD_INSTALL_SYSTEM_LIBRARY( "${LIBFREETYPELOC}" )
		list( APPEND OD_THIRD_PARTY_LIBS "${LIBFREETYPELOC}" )
	    else()
		message( SEND_ERROR "Required system library not found: libfreetype" )
	    endif()
	endif()
    endif(UNIX)
endmacro(QT_SETUP_GUI)

macro(OD_SETUP_QT)
    if ( OD_NO_QT )
	add_definitions( -DOD_NO_QT )
    else()
	#Setup Qt5
	if ( Qt5Core_FOUND )
	    set(QT_VERSION_MAJOR ${Qt5Core_VERSION_MAJOR} PARENT_SCOPE)
	    list( APPEND QT_INSTLIBS Svg Xml )
	    set( NEED_XCBQPA_SETUP False )
	    if ( UNIX )
		list( APPEND QT_INSTLIBS DBus )
		if ( ${OD_PLFSUBDIR} STREQUAL "lux64" )
		    if ( Qt5Core_VERSION_MINOR GREATER 12 )
			list( APPEND QT_INSTLIBS XcbQpa )
		    else() #No cmake support prior to Qt5.13
			set( NEED_XCBQPA_SETUP True )
		    endif()
		endif()
	    endif()
	    if ( Qt5Core_VERSION_MINOR GREATER 13 )
		list( APPEND QT_INSTLIBS Qml QmlModels )
	    endif()
	    set( OD_QTALLLIBS ${OD_USEQT} )
	    list( APPEND OD_QTALLLIBS ${QT_INSTLIBS} )
	    find_package( Qt5 REQUIRED ${OD_QTALLLIBS} )

	    if( QT_MOC_HEADERS )
		set ( OD_MODULE_DIR ${CMAKE_SOURCE_DIR}/src/${OD_MODULE_NAME} )
		if ( NOT EXISTS ${OD_MODULE_DIR} ) # Allow plugins
		    set ( OD_MODULE_DIR
			  ${CMAKE_SOURCE_DIR}/plugins/${OD_MODULE_NAME} )
		endif()

		foreach( HEADER ${QT_MOC_HEADERS} )
		    list( APPEND QT_MOC_INPUT ${OD_MODULE_DIR}/${HEADER} )
		endforeach()
		QT5_WRAP_CPP( QT_MOC_OUTFILES ${QT_MOC_INPUT} )
	    endif( QT_MOC_HEADERS )

	    foreach ( QTMOD ${OD_USEQT} )
		list( APPEND OD_MODULE_INCLUDESYSPATH ${Qt5${QTMOD}_INCLUDE_DIRS} )
		list( APPEND OD_QT_LIBS ${Qt5${QTMOD}_LIBRARIES} )
		if ( "${QTMOD}" STREQUAL "Gui" )
		    QT_SETUP_GUI()
		endif()
	    endforeach()

	    get_target_property( QT5CORE_LIBRARY Qt5::Core LOCATION )
	    get_filename_component( QT_LIBRARY_DIR ${QT5CORE_LIBRARY} DIRECTORY )
	    list ( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${QT_LIBRARY_DIR} )

	    list( REMOVE_DUPLICATES OD_QT_LIBS )
	    list( REMOVE_DUPLICATES OD_MODULE_INCLUDESYSPATH )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS ${OD_QT_LIBS} )

	    foreach( MODNM ${OD_USEQT} )
		list( APPEND OD_MODULE_EXTERNAL_LIBS Qt5::${MODNM} )
	    endforeach()
	    foreach( MODNM ${QT_INSTLIBS} )
		list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS Qt5::${MODNM} )
	    endforeach()

	    #Install only, no direct cmake support
	    OD_INSTALL_RESSOURCE( ${QTDIR}/bin/qt.conf ${CMAKE_BUILD_TYPE} )
	    install( DIRECTORY ${QTDIR}/translations
		     DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}
		     CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		     USE_SOURCE_PERMISSIONS
		     FILES_MATCHING
		     PATTERN "qt_*.qm"
		     PATTERN "qtbase_*.qm" )

	    if ( WIN32 )
		set( QTPOSTFIX "" )
		if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
		    set( QTPOSTFIX "d" )
		endif()
		list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS
			"${QTDIR}/bin/libEGL${QTPOSTFIX}.dll"
			"${QTDIR}/bin/libGLESv2${QTPOSTFIX}.dll"
			"${QTDIR}/bin/opengl32sw.dll" )
	    endif()

	    set( QT_REQ_PLUGINS iconengines imageformats platforms )
	    if ( UNIX )
		if ( ${NEED_XCBQPA_SETUP} )
		    set ( xcpalibnm "${QTDIR}/lib/libQt${QT_VERSION_MAJOR}XcbQpa.so.${QT_VERSION_MAJOR}" )
		    if ( EXISTS "${xcpalibnm}" )
			list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS "${xcpalibnm}" )
		    endif()
		endif()
		if( NOT APPLE )
		    list( APPEND QT_REQ_PLUGINS xcbglintegrations )
		    set( ICU_VERSION_MAJOR "56" )
		    set( LIBNMS i18n data uc )
		    foreach( LIBNM ${LIBNMS} )
			set( FILENM "${QTDIR}/lib/libicu${LIBNM}.so.${ICU_VERSION_MAJOR}" )
			if ( EXISTS "${FILENM}" )
			    list ( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS "${FILENM}" )
			endif()
		    endforeach()
		endif()
	    else()
		list( APPEND QT_REQ_PLUGINS styles )
	    endif()
	    foreach( QTPLUGIN ${QT_REQ_PLUGINS} )
		install( DIRECTORY ${QTDIR}/plugins/${QTPLUGIN}
			 DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}
			 CONFIGURATIONS ${CMAKE_BUILD_TYPE}
			 USE_SOURCE_PERMISSIONS 
			 FILES_MATCHING
			 PATTERN "*.so"
			 PATTERN "*.dll"
			 PATTERN "*d.dll" EXCLUDE
			 PATTERN "*.pdb" EXCLUDE
			 PATTERN "*.so.debug" EXCLUDE )
	    endforeach()

	    if ( WIN32 )
		set ( CMAKE_CXX_FLAGS "/wd4481 ${CMAKE_CXX_FLAGS}" )
	    endif( WIN32 )
	else() # Use Qt4

	    if ( QTDIR )
	        set( ENV{QTDIR} ${QTDIR} )
		set ( QT_QMAKE_EXECUTABLE ${QTDIR}/bin/qmake${CMAKE_EXECUTABLE_SUFFIX} )
		list( APPEND OD_MODULE_INCLUDESYSPATH ${QTDIR}/include )
	    endif()

	    find_package( Qt4 REQUIRED QtGui QtCore QtSql QtNetwork )
	    string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" QT_VERSION_MAJOR
		   "${Qt5Core_VERSION_STRING}")
	    set(QT_VERSION_MAJOR ${QT_VERSION_MAJOR} PARENT_SCOPE)

	    include(${QT_USE_FILE})

	    STRING( FIND "${OD_USEQT}" "Core" USE_QT_CORE )
	    if( NOT "${USE_QT_CORE}" EQUAL -1 )
	    list( APPEND OD_MODULE_INCLUDESYSPATH
		    ${QT_QTCORE_INCLUDE_DIR} )
	    ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTCORE_LIBRARY}" )
	    endif()

	    STRING( FIND "${OD_USEQT}" "Network" USE_QT_NETWORK )
	    if( NOT "${USE_QT_NETWORK}" EQUAL -1 )
	    list( APPEND OD_MODULE_INCLUDESYSPATH
		${QT_QTNETWORK_INCLUDE_DIR} )
	    ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTNETWORK_LIBRARY}" )
	    endif()

	    STRING( FIND "${OD_USEQT}" "Sql" USE_QT_SQL )
	    if( NOT "${USE_QT_SQL}" EQUAL -1 )
	    list(APPEND OD_MODULE_INCLUDESYSPATH
		${QT_QTSQL_INCLUDE_DIR} )
	    ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTSQL_LIBRARY}")
	    endif()

	    STRING( FIND "${OD_USEQT}" "Widgets" USE_QT_GUI )
	    if( NOT "${USE_QT_GUI}" EQUAL -1 )
	    list(APPEND OD_MODULE_INCLUDESYSPATH
		${QT_QTCORE_INCLUDE_DIR}
		${QT_QTGUI_INCLUDE_DIR} )
	    ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTGUI_LIBRARY}")
	    endif()

	    STRING( FIND "${OD_USEQT}" "OpenGL" USE_QT_OPENGL )
	    if( NOT "${USE_QT_OPENGL}" EQUAL -1 )
	    list(APPEND OD_MODULE_INCLUDESYSPATH
		${QT_QTOPENGL_INCLUDE_DIR} )
	    ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTOPENGL_LIBRARY}")
	    endif()

	    STRING( FIND "${OD_USEQT}" "Xml" USE_QT_XML )
	    if( NOT "${USE_QT_XML}" EQUAL -1 )
	    list(APPEND OD_MODULE_INCLUDESYSPATH
		${QT_QTXML_INCLUDE_DIR} )
	    ADD_TO_LIST_IF_NEW( OD_QT_LIBS "${QT_QTXML_LIBRARY}")
	    endif()

	    if ( OD_MODULE_INCLUDESYSPATH )
		list( REMOVE_DUPLICATES OD_MODULE_INCLUDESYSPATH )
	    endif()

	    if( QT_MOC_HEADERS )
		set ( OD_MODULE_DIR ${CMAKE_SOURCE_DIR}/src/${OD_MODULE_NAME} )
		if ( NOT EXISTS ${OD_MODULE_DIR} ) # Allow plugins
		    set ( OD_MODULE_DIR
			  ${CMAKE_SOURCE_DIR}/plugins/${OD_MODULE_NAME} )
		endif()

		foreach( HEADER ${QT_MOC_HEADERS} )
		    list(APPEND QT_MOC_INPUT ${OD_MODULE_DIR}/${HEADER})
		endforeach()

		QT4_WRAP_CPP (QT_MOC_OUTFILES ${QT_MOC_INPUT})
	    endif( QT_MOC_HEADERS )

	    list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_QT_LIBS} )
	    if ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
		foreach( BUILD_TYPE Debug Release )
		    set( QARGS ${QT_QTOPENGL_LIBRARY} ${QT_QTCORE_LIBRARY}
			       ${QT_QTNETWORK_LIBRARY} ${QT_QTSQL_LIBRARY}
			       ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY} )

		    OD_FILTER_LIBRARIES( QARGS ${BUILD_TYPE} )
		    unset( ARGS )

		    foreach( QLIB ${QARGS} )
			get_filename_component( QLIBNAME ${QLIB} NAME_WE )
			if( UNIX OR APPLE )
			    if( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
				set( FILENM "${QLIBNAME}.so.${QT_VERSION_MAJOR}" )
			    elseif( APPLE )
				set( FILENM "${QLIBNAME}.${QT_VERSION_MAJOR}.dylib" )
			    endif()
			    OD_INSTALL_LIBRARY( ${QTDIR}/lib/${FILENM} )
			elseif( WIN32 )
			    OD_INSTALL_LIBRARY( ${QTDIR}/bin/${QLIBNAME}.dll )
			    install( PROGRAMS ${QLIB}
				     DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${BUILD_TYPE}
				     CONFIGURATIONS ${BUILD_TYPE} )
			endif()
		    endforeach()
		endforeach()

	    endif()
	endif()

    endif( OD_NO_QT )
endmacro( OD_SETUP_QT )

macro ( SETUP_QT_TRANSLATION POSTFIX )
    if ( EXISTS ${QT_LRELEASE_EXECUTABLE} )
	if ( WIN32 )
	    set ( COMPILE_TRANSLATIONS_EXTENSION cmd )
	else()
	    set ( COMPILE_TRANSLATIONS_EXTENSION sh )
	endif()
	add_custom_target( Compile_Translations_${POSTFIX} EXCLUDE_FROM_ALL
	    ${OpendTect_DIR}/dtect/compile_translations.${COMPILE_TRANSLATIONS_EXTENSION}
	    ${QT_LRELEASE_EXECUTABLE} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
	    VERBATIM
	COMMENT "Compiling translations" )
    
	install(DIRECTORY data/localizations/ DESTINATION ${MISC_INSTALL_PREFIX}/data/localizations
          FILES_MATCHING PATTERN "*.qm")
	
    endif()
endmacro( SETUP_QT_TRANSLATION )

macro( OD_GET_SETUP_QTWEBENGINE )
    get_property( HAS_WEBENGINE VARIABLE PROPERTY USE_QtWebEngine )
    if ( ${HAS_WEBENGINE} )
	add_definitions( -D__withqtwebengine__ )
    endif()
endmacro(  )
