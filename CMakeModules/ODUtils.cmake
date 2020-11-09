#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

if ( (CMAKE_GENERATOR STREQUAL "Unix Makefiles") OR
     (CMAKE_GENERATOR STREQUAL "Ninja") OR
     (CMAKE_GENERATOR STREQUAL "NMake Makefiles") )
    if ( CMAKE_BUILD_TYPE STREQUAL "" )
	set ( DEBUGENV $ENV{DEBUG} )
	if ( DEBUGENV AND
	    ( (${DEBUGENV} MATCHES "yes" ) OR
	      (${DEBUGENV} MATCHES "Yes" ) OR
	      (${DEBUGENV} MATCHES "YES" ) ) )
	    set ( CMAKE_BUILD_TYPE "Debug"
		  CACHE STRING "Debug or Release" FORCE )
	else()
	    set ( CMAKE_BUILD_TYPE "Release"
		  CACHE STRING "Debug or Release" FORCE)
	endif()

	message( STATUS "Setting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE}" )
    endif()

    set( OD_BUILDSUBDIR "/${CMAKE_BUILD_TYPE}" )
endif()

add_definitions("-D__cmake__")

if ( APPLE )
    set( MISC_INSTALL_PREFIX Contents/Resources )
else()
    set( MISC_INSTALL_PREFIX . )
endif()


set ( OD_SOURCELIST_FILE ${CMAKE_BINARY_DIR}/CMakeModules/sourcefiles_${OD_SUBSYSTEM}.txt )
file ( REMOVE ${OD_SOURCELIST_FILE} )

if ( APPLE )
    set ( OD_EXEC_OUTPUT_RELPATH "Contents/MacOS" )
    set ( OD_EXEC_RELPATH_RELEASE ${OD_EXEC_OUTPUT_RELPATH} )
    set ( OD_EXEC_RELPATH_DEBUG ${OD_EXEC_OUTPUT_RELPATH} )
    set ( OD_LIB_OUTPUT_RELPATH "Contents/Frameworks" )
    set ( OD_LIB_RELPATH_RELEASE ${OD_LIB_OUTPUT_RELPATH} )
    set ( OD_LIB_RELPATH_DEBUG ${OD_LIB_OUTPUT_RELPATH} )
else()
    set ( OD_EXEC_OUTPUT_RELPATH "bin/${OD_PLFSUBDIR}${OD_BUILDSUBDIR}" )
    set ( OD_EXEC_RELPATH_RELEASE "bin/${OD_PLFSUBDIR}/Release" )
    set ( OD_EXEC_RELPATH_DEBUG "bin/${OD_PLFSUBDIR}/Debug" )
    set ( OD_LIB_OUTPUT_RELPATH "bin/${OD_PLFSUBDIR}${OD_BUILDSUBDIR}" )
    set ( OD_LIB_RELPATH_RELEASE "bin/${OD_PLFSUBDIR}/Release" )
    set ( OD_LIB_RELPATH_DEBUG "bin/${OD_PLFSUBDIR}/Debug" )
endif()

set ( OD_EXEC_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${OD_EXEC_OUTPUT_RELPATH}" )
set ( OD_LIB_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${OD_LIB_OUTPUT_RELPATH}" )
set ( OD_DATA_INSTALL_RELPATH "${MISC_INSTALL_PREFIX}/data" )

set ( OD_EXEC_INSTALL_PATH_RELEASE ${OD_EXEC_RELPATH_RELEASE} )
set ( OD_EXEC_INSTALL_PATH_DEBUG ${OD_EXEC_RELPATH_DEBUG} )

set ( OD_LIB_INSTALL_PATH_RELEASE ${OD_LIB_RELPATH_RELEASE} )
set ( OD_LIB_INSTALL_PATH_DEBUG ${OD_LIB_RELPATH_DEBUG} )

set ( CMAKE_PDB_OUTPUT_DIRECTORY "${OD_EXEC_OUTPUT_PATH}" )

set ( OD_BUILD_VERSION "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}")
set ( OD_API_VERSION "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}" )

set ( OD_MAIN_EXEC od_main od_main_console )
set ( OD_ATTRIB_EXECS od_process_attrib od_process_attrib_em od_stratamp )
set ( OD_VOLUME_EXECS od_process_volume )
set ( OD_SEIS_EXECS od_copy_seis od_process_2dto3d od_process_segyio )
set ( OD_PRESTACK_EXECS od_process_prestack )
set ( OD_ZAXISTRANSFORM_EXECS od_process_time2depth )

#Should not be here.
set ( DGB_SR_EXECS od_SynthRock )
set ( DGB_ML_EXECS od_deeplearn_apply )
set ( DGB_SEGY_EXECS od_DeepLearning_CC )
set ( DGB_ML_UIEXECS od_DeepLearning od_DeepLearning_CC od_DeepLearning_EM od_DeepLearning_TM od_DeepLearning_ModelImport )
set ( DGB_PRO_UIEXECS od_LogPlot )

set( OD_INSTALL_DEPENDENT_LIBS_DEFAULT OFF )
if ( OSG_DIR )
    set( OD_INSTALL_DEPENDENT_LIBS_DEFAULT ON )
endif( OSG_DIR )

if ( QTDIR )
    set( OD_INSTALL_DEPENDENT_LIBS_DEFAULT ON )
endif( QTDIR )


# This option does two things
#
# 1. It installs the QT, OSG and other libraries to the installation structure when
#    building the "install" target. On systems where these dependencies are provided
#    through the operating system, this is not needed.
#
# 2. It copies dependencies into the build environment (bin/PLF/Debug and bin/PLF/Release)
#    This is as the build environment should be as similar to the runtime environment as
#    possible
#
# Default is to have it OFF when QTDIR and OSGDIR are not set

option( OD_INSTALL_DEPENDENT_LIBS "Install dependent libs" ${OD_INSTALL_DEPENDENT_LIBS_DEFAULT} )

#Macro for going through a list of modules and adding them
macro ( OD_ADD_MODULES )
    set( DIR ${ARGV0} )

    foreach( OD_MODULE_NAME ${ARGV} )
	if ( NOT ${OD_MODULE_NAME} STREQUAL ${DIR} )
	    add_subdirectory( ${DIR}/${OD_MODULE_NAME} 
		    	      ${DIR}/${OD_MODULE_NAME} )
	endif()
    endforeach()
endmacro()

# Macro for going through a list of modules and adding them
# as optional targets
macro ( OD_ADD_OPTIONAL_MODULES )
    set( DIR ${ARGV0} )

    foreach( OD_MODULE_NAME ${ARGV} )
	if ( NOT ${OD_MODULE_NAME} MATCHES ${DIR} )
	    add_subdirectory( ${DIR}/${OD_MODULE_NAME} EXCLUDE_FROM_ALL )
	endif()
    endforeach()
endmacro()

function(guess_runtime_library_dirs _var)
    # Start off with the link directories of the calling listfile's directory
    get_directory_property(_libdirs LINK_DIRECTORIES)

    # Add additional libraries passed to the function
    foreach(_lib ${ARGN})
	if ( EXISTS "${_lib}" )
	    set( _libdir "${_lib}" )
	else()
	    OD_READ_TARGETINFO( "${_lib}" )
	    set( _libdir "${SOURCEFILE}" )
	endif()
	if ( EXISTS "${_libdir}" )
	    get_filename_component( _libdir "${_libdir}" DIRECTORY )
	    list(APPEND _libdirs "${_libdir}")
	endif()
    endforeach()
    clean_directory_list(_libdirs)

    if ( UNIX )
	set(${_var} "${_libdirs}" PARENT_SCOPE)
    else()
	# Now, build a list of potential dll directories
	set(_dlldirs)
	foreach(_libdir ${_libdirs})
	    get_filename_component(_dlldir "${_libdir}/../bin" ABSOLUTE)
	    if ( EXISTS "${_dlldir}" )
		list(APPEND _dlldirs "${_dlldir}")
	    else()
		list(APPEND _dlldirs "${_libdir}")
	    endif()
	endforeach()

	clean_directory_list(_dlldirs)
	set(${_var} "${_dlldirs}" PARENT_SCOPE)
    endif()
endfunction()

function(guess_extruntime_library_dirs _var)
    foreach(_lib ${ARGN})
	if ( EXISTS "${_lib}" )
	    set( LIBLOC ${_lib} )
	else()
	    get_target_property( LIBLOC ${_lib} IMPORTED_LOCATION )
	endif()
	if ( EXISTS "${LIBLOC}" )
	    get_filename_component( LIBDIR ${LIBLOC} DIRECTORY )
	    if ( WIN32 )
		get_filename_component( DLLDIR ${LIBDIR} DIRECTORY )
		set( DLLDIR "${DLLDIR}/bin" )
		if ( EXISTS ${DLLDIR} )
		    set( RUNTIMEDIR ${DLLDIR} )
		else()
		    set( RUNTIMEDIR ${LIBDIR} )
		endif()
	    else()
		set( RUNTIMEDIR ${LIBDIR} )
	    endif( WIN32 )
	    if ( EXISTS ${RUNTIMEDIR} )
		list( APPEND _libdirs ${RUNTIMEDIR} )
	    endif()
	endif()
    endforeach()
    clean_directory_list(_libdirs)
    set(${_var} "${_libdirs}" PARENT_SCOPE)
endfunction()

macro( OD_READ_TARGETINFO TARGETNM )
    get_target_property( LIB_BUILD_TYPE ${TARGETNM} IMPORTED_CONFIGURATIONS )
    if(POLICY CMP0057)
	cmake_policy(SET CMP0057 NEW)
    endif()
    list( LENGTH LIB_BUILD_TYPE LIB_NRBUILDS )
    if ( LIB_NRBUILDS GREATER 1 )
	if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" AND "DEBUG" IN_LIST LIB_BUILD_TYPE )
	    set( LIB_BUILD_TYPE "DEBUG" )
	elseif ( ${CMAKE_BUILD_TYPE} STREQUAL "Release" AND "RELEASE" IN_LIST LIB_BUILD_TYPE )
	    set( LIB_BUILD_TYPE "RELEASE" )
	elseif( "RELEASE" IN_LIST LIB_BUILD_TYPE )
	    set( LIB_BUILD_TYPE "RELEASE" )
	elseif( "DEBUG" IN_LIST LIB_BUILD_TYPE )
	    set( LIB_BUILD_TYPE "DEBUG" )
	endif()
    endif()
    if ( LIB_BUILD_TYPE )
	set( IMPORTED_LOC_KEYWORD IMPORTED_LOCATION_${LIB_BUILD_TYPE} )
	set( IMPORTED_SONAME_KEYWORD IMPORTED_SONAME_${LIB_BUILD_TYPE} )
    else()
	set( IMPORTED_LOC_KEYWORD IMPORTED_LOCATION )
	set( IMPORTED_SONAME_KEYWORD IMPORTED_SONAME )
    endif()
    get_target_property( LIBLOC ${TARGETNM} ${IMPORTED_LOC_KEYWORD} )
    if ( NOT LIBLOC )
	get_target_property( LIBLOC ${TARGETNM} LOCATION )
	if ( NOT EXISTS "${LIBLOC}" )
	    get_target_property( LIBLOC ${TARGETNM} LOCATION_Debug )
	    if ( EXISTS "${LIBLOC}" )
		set( LIB_BUILD_TYPE "Debug" )
	    else()
		get_target_property( LIBLOC ${TARGETNM} LOCATION_Release )
		if ( EXISTS "${LIBLOC}" )
		    set( LIB_BUILD_TYPE "Release" )
		endif()
	    endif()
	endif()
    endif()
    if ( UNIX )
	get_target_property( LIBSONAME ${TARGETNM} ${IMPORTED_SONAME_KEYWORD} )
    endif()
    get_filename_component( LIBDIR ${LIBLOC} DIRECTORY )
    if ( LIBSONAME )
	set( LIBLOC "${LIBDIR}/${LIBSONAME}" )
    endif()

    if ( NOT LIBLOC )
	message( FATAL_ERROR "Unrecognized target to install: ${TARGETNM} ${LIBLOC}" )
    endif()

    set ( SOURCEFILE "${LIBLOC}" )
endmacro()

macro( OD_GET_WIN32_INST_VARS )
    get_filename_component( BASEFILENM ${FILEPATH} NAME_WE )
    get_filename_component( FILEEXT ${FILEPATH} EXT )
    if ( "${FILEEXT}" STREQUAL ".lib" )
	get_filename_component( DLLPATH ${FILEPATH} DIRECTORY )
	get_filename_component( DLLPATH ${DLLPATH} DIRECTORY )
	set( FILEPATH "" )
	set( DLLPATH "${DLLPATH}/bin" )
	if ( EXISTS "${DLLPATH}" )
	    file( GLOB DLLFILENMS "${DLLPATH}/*${BASEFILENM}*.dll" )
	    foreach( DLLFILE ${DLLFILENMS} )
		list( APPEND FILEPATH "${DLLFILE}" )
		get_filename_component( DLLBASEFILENM "${DLLFILE}" NAME_WE )
		file( GLOB PDBFILENMS "${DLLPATH}/${DLLBASEFILENM}*.pdb" )
		foreach( PDBFILE ${PDBFILENMS} )
		    list( APPEND FILEPATH "${PDBFILE}" )
		endforeach()
	    endforeach()
	endif()
    elseif ( "${FILEEXT}" STREQUAL ".dll" OR "${FILEEXT}" STREQUAL ".DLL" )
	get_filename_component( DLLPATH ${FILEPATH} DIRECTORY )
	file( GLOB PDBFILENMS "${DLLPATH}/${BASEFILENM}*.pdb" )
	foreach( PDBFILE ${PDBFILENMS} )
	    list( APPEND FILEPATH "${PDBFILE}" )
	endforeach()
    endif()
    set( FILENAMES "" )
    foreach( FILENM ${FILEPATH} )
	get_filename_component( FILENAME ${FILENM} NAME )
	list( APPEND FILENAMES ${FILENAME} )
    endforeach()
endmacro()

macro( OD_GET_UNIX_INST_VARS )
    set( FILENAMES "" )
    foreach( FILENM ${FILEPATH} )
	get_filename_component( FILENAME ${FILENM} NAME )
	if ( APPLE )
	    set( LIBEXT ".dylib" )
	else()
	    set( LIBEXT ".so" )
	endif()
	get_filename_component( SRCFILENAME ${SOURCEFILE} NAME )
	get_filename_component( INSTPATH ${SOURCEFILE} DIRECTORY )
	if ( DEFINED LIBSONAME AND LIBSONAME )
	    set( FILENAME ${LIBSONAME} )
	    unset( LIBSONAME )
	else()
	    get_filename_component( FILEEXT ${SOURCEFILE} EXT )
	    if ( "${FILEEXT}" STREQUAL "${LIBEXT}" AND NOT "${FILENAME}" STREQUAL "${SRCFILENAME}" )
		if ( CMAKE_VERSION VERSION_GREATER_EQUAL 3.14 )
		    file(READ_SYMLINK "${SOURCEFILE}" FILENAME)
		else()
		    file(GLOB FILENAME "${SOURCEFILE}.???")
		    if ( NOT EXISTS ${FILENAME} )
			file(GLOB FILENAME "${SOURCEFILE}.??")
			if ( NOT EXISTS ${FILENAME} )
			    get_filename_component( FILENAME "${SOURCEFILE}" REALPATH )
			    get_filename_component( FILENAME "${FILENAME}" NAME )
			endif()
		    endif()
		endif()
		if ( EXISTS "${FILENAME}")
		    get_filename_component( FILENAME "${FILENAME}" NAME )
		endif()
	    else()
		set( FILENAME "${SRCFILENAME}" )
	    endif()
	endif()
	list( APPEND FILENAMES ${FILENAME} )
    endforeach()
endmacro( OD_GET_UNIX_INST_VARS )

macro( OD_GET_INSTALL_VARS SOURCE )
    if ( EXISTS "${SOURCE}" )
	set( SOURCEFILE "${SOURCE}" )
    else()
	OD_READ_TARGETINFO( ${SOURCE} )
    endif()
    get_filename_component( FILEPATH ${SOURCEFILE} REALPATH )
    if ( WIN32 )
	OD_GET_WIN32_INST_VARS()
    else()
	OD_GET_UNIX_INST_VARS()
    endif( WIN32 )
endmacro( OD_GET_INSTALL_VARS )

macro ( OD_INSTALL_SYSTEM_LIBRARY SOURCE )
    OD_GET_INSTALL_VARS( ${SOURCE} )
    list(LENGTH FILEPATH len)
    if ( len GREATER 0 )
	math(EXPR len "${len} - 1")
	foreach( val RANGE ${len} )
	    list(GET FILEPATH ${val} FILENM)
	    list(GET FILENAMES ${val} FILENAME)
	    #message( STATUS "Installing: ${FILENM} as ${FILENAME}" )
	    install( PROGRAMS ${FILENM} DESTINATION ${OD_LIB_INSTALL_PATH_DEBUG}
		     RENAME ${FILENAME} CONFIGURATIONS Debug )
	    install( PROGRAMS ${FILENM} DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE}
		     RENAME ${FILENAME} CONFIGURATIONS Release )
	endforeach()
    endif()
endmacro( OD_INSTALL_SYSTEM_LIBRARY )

macro ( OD_INSTALL_LIBRARY SOURCE )
    OD_INSTALL_SYSTEM_LIBRARY( "${SOURCE}" )
    if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
	list( APPEND OD_MODULE_THIRD_PARTY_DEBUG_LIBS ${OD_THIRD_PARTY_LIBS_DEBUG} ${FILENAME} )
	list( REMOVE_DUPLICATES OD_MODULE_THIRD_PARTY_DEBUG_LIBS )
	set ( OD_THIRD_PARTY_LIBS_DEBUG ${OD_MODULE_THIRD_PARTY_DEBUG_LIBS} PARENT_SCOPE )
    else()
	list( APPEND OD_MODULE_THIRD_PARTY_LIBS ${OD_THIRD_PARTY_LIBS} ${FILENAME} )
	list( REMOVE_DUPLICATES OD_MODULE_THIRD_PARTY_LIBS )
	set ( OD_THIRD_PARTY_LIBS ${OD_MODULE_THIRD_PARTY_LIBS} PARENT_SCOPE )
    endif()
endmacro( OD_INSTALL_LIBRARY )

macro ( OD_INSTALL_RESSOURCE SOURCE )
    install( FILES ${SOURCE} DESTINATION ${OD_LIB_INSTALL_PATH_DEBUG}
	     CONFIGURATIONS Debug )
    install( FILES ${SOURCE} DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE}
	     CONFIGURATIONS Release )
endmacro( OD_INSTALL_RESSOURCE )

macro ( OD_INSTALL_PROGRAM SOURCE )
    install( PROGRAMS ${SOURCE} DESTINATION ${OD_LIB_INSTALL_PATH_DEBUG}
	     CONFIGURATIONS Debug )
    install( PROGRAMS ${SOURCE} DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE}
	     CONFIGURATIONS Release )
endmacro( OD_INSTALL_PROGRAM )

#Takes a library variable with both _RELEASE and _DEBUG variants, and constructs
# a variable that combinse both

macro ( OD_MERGE_LIBVAR VARNAME )
    if ( ${${VARNAME}_RELEASE} STREQUAL "${VARNAME}_RELEASE-NOTFOUND" )
	set( RELNOTFOUND 1 )
    endif()
    if ( ${${VARNAME}_DEBUG} STREQUAL "${VARNAME}_DEBUG-NOTFOUND" )
	set( DEBNOTFOUND 1 )
    endif()

    if ( DEFINED RELNOTFOUND AND DEFINED DEBNOTFOUND )
	message( FATAL_ERROR "${VARNAME} not found" )
    endif()

    if ( (NOT DEFINED RELNOTFOUND) )
	if ( NOT DEFINED DEBNOTFOUND )
	    set ( ${VARNAME} "optimized" ${${VARNAME}_RELEASE} "debug" ${${VARNAME}_DEBUG}  )
	else()
	    set ( ${VARNAME} ${${VARNAME}_RELEASE} )
	endif()
    else()
	set ( ${VARNAME} ${${VARNAME}_DEBUG} )
    endif()

    unset( RELNOTFOUND )
    unset( DEBNOTFOUND )
endmacro()


#Takes a list with both optimized and debug libraries, and removes one of them
#According to BUILD_TYPE

macro ( OD_FILTER_LIBRARIES INPUTLIST BUILD_TYPE )
    unset( OUTPUT )
    foreach ( LISTITEM ${${INPUTLIST}} )
	if ( DEFINED USENEXT )
	    if ( USENEXT STREQUAL "yes" )
		list ( APPEND OUTPUT ${LISTITEM} )
	    endif()
	    unset( USENEXT )
	else()
	    if ( LISTITEM STREQUAL "debug" )
		if ( "${BUILD_TYPE}" STREQUAL "Debug" )
		    set ( USENEXT "yes" )
		else()
		    set ( USENEXT "no" )
		endif()
	    else()
		if ( LISTITEM STREQUAL "optimized" )
		    if ( "${BUILD_TYPE}" STREQUAL "Release" )
			set ( USENEXT "yes" )
		    else()
			set ( USENEXT "no" )
		    endif()
		else()
		    list ( APPEND OUTPUT ${LISTITEM} )
		endif()
	    endif()
	endif()
    endforeach()

    set ( ${INPUTLIST} ${OUTPUT} )
endmacro()

function( od_find_library _var)
    foreach( _lib ${ARGN} )
	if ( DEFINED LIBSEARCHPATHS )
	    find_library( ODLIBLOC ${_lib} PATHS ${LIBSEARCHPATHS} REQUIRED )
	else()
	    find_library( ODLIBLOC ${_lib} REQUIRED )
	endif()
	if ( ODLIBLOC )
	    break()
	endif()
    endforeach()
    if ( ODLIBLOC )
	set(${_var} "${ODLIBLOC}" PARENT_SCOPE)
    endif()
    unset( ODLIBLOC CACHE )
endfunction()

OPTION ( OD_CREATE_COMPILE_DATABASE "Create compile_commands.json database for analyser tools to use" OFF )
if ( OD_CREATE_COMPILE_DATABASE )
    set( CMAKE_EXPORT_COMPILE_COMMANDS "ON" )
endif()
# Used compile_commands.json for include-what-you-use
# python3 /usr/local/bin/iwyu_tool.py -p . > iwyu_results.txt
# Note that this tool is of limited use as it wants to dictate all includes
