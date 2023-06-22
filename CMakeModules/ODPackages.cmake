#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_CREATE_PACKAGE_DEFINITION )
    GET_OD_BASE_EXECUTABLES()
    configure_file( "${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/develdefs.cmake.in"
		    "${CMAKE_BINARY_DIR}/CMakeModules/packagescripts/develdefs.cmake"
		    @ONLY )

    configure_file( "${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/basedefs.cmake.in"
		    "${CMAKE_BINARY_DIR}/CMakeModules/packagescripts/basedefs.cmake"
		    @ONLY )
endmacro()


macro( OD_ADD_PACKAGES_TARGET )
    if ( NOT DEFINED PACKAGE_DIR )
	set( PACKAGE_DIR "${CMAKE_SOURCE_DIR}/../packages" )
	get_filename_component( PACKAGE_DIR "${PACKAGE_DIR}" ABSOLUTE )
    endif()

    if ( "${MAIN_GIT_BRANCH}" STREQUAL "main" )
	set( OpendTect_INST_DIR "0.0.0" )
    else()
	set( OpendTect_INST_DIR ${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH} )
    endif()

    set( CMAKE_FOLDER "Releases" )
    add_custom_target( packages ${CMAKE_COMMAND} 
	    -DOpendTect_INST_DIR=${OpendTect_INST_DIR}
	    -DOpendTect_FULL_VERSION=${OpendTect_FULL_VERSION}
	    "-DOD_QTPLUGINS=\"${OD_QTPLUGINS}\""
	    "-DOD_QT_TRANSLATION_FILES=\"${OD_QT_TRANSLATION_FILES}\""
	    -DQT_VERSION_MAJOR=${QT_VERSION_MAJOR}
	    -DOD_PLFSUBDIR=${OD_PLFSUBDIR} 
	    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
	    -DBINARY_DIR=${CMAKE_BINARY_DIR}
	    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} 
	    -DOD_LIBRARY_DIRECTORY=${OD_LIBRARY_DIRECTORY}
	    -DOD_RUNTIME_DIRECTORY=${OD_RUNTIME_DIRECTORY}
	    -DOD_LOCATION_DIRECTORY=${OD_LOCATION_DIRECTORY}
	    -DMATLAB_DIR=${MATLAB_DIR}
	    -DOD_ENABLE_BREAKPAD=${OD_ENABLE_BREAKPAD}
	    -DOD_NO_PROJ=${OD_NO_PROJ}
	    -DSQLITE_DIR=${SQLITE_DIR}
	    -DBREAKPAD_DIR=${BREAKPAD_DIR}
	    -DPACKAGE_DIR=${PACKAGE_DIR}
	    -DBUILD_DOCUMENTATION=${BUILD_DOCUMENTATION}
	    -DCLASSDOC_SCRIPT_LOCATION=${CLASSDOC_SCRIPT_LOCATION}
	    -DUSERDOC_SCRIPT_LOCATION=${USERDOC_SCRIPT_LOCATION}
	    -DBUILD_USERDOC=${BUILD_USERDOC}
	    -DUSERDOC_PROJECT=${USERDOC_PROJECT}
	    -P "${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/ODMakePackages.cmake" 
	    COMMENT "Creating packages" ) 
    unset( CMAKE_FOLDER )
endmacro()


macro( OD_ADD_SIGNLIBRARIES_TARGET )
    set( CMAKE_FOLDER "Releases" )
    if ( WIN32 )
	add_custom_target( signlibraries "${SIGN_SCRIPT_LOCATION}"
			   "${CMAKE_INSTALL_PREFIX}/${OD_RUNTIME_DIRECTORY}"
			   COMMENT "Signing DLLs and EXEs" )
    elseif ( APPLE )
	add_custom_target( signlibraries "${SIGN_SCRIPT_LOCATION}"
			   "${CMAKE_INSTALL_PREFIX}"
			   COMMENT "Signing Libraries and EXEs" )
    endif()
    unset( CMAKE_FOLDER )
endmacro()
