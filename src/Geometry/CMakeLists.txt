#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS General )
set( OD_FOLDER "Base" )

set( OD_MODULE_SOURCES
	arraytesselator.cc
	binidsurface.cc
	cubicbeziercurve.cc
	cubicbeziersurface.cc
	explfaultsticksurface.cc
	explicitmarchingcubes.cc
	explplaneintersection.cc
	explpolygonsurface.cc
	faulthorintersect.cc
	faultstickset.cc
	faultsticksurface.cc
	geeditor.cc
	geeditorimpl.cc
	geom2dinit.cc
	geom2dintersections.cc
	geomelement.cc
	horizon2dline.cc
	horizon3dtesselator.cc
	impbodyplaneintersect.cc
	indexedshape.cc
	initgeometry.cc
	marchingcubes.cc
	marchingcubeseditor.cc
	parametriccurve.cc
	parametricsurface.cc
	picksettr.cc
	polygonsurface.cc
	polygonsurfaceedit.cc
	polyposprovider.cc
	posvecdatasettr.cc
	probdenfunctr.cc
	randomlinegeom.cc
	randomlinetr.cc
	rowcolsurface.cc
	stickseteditor.cc
	survgeometry.cc
	tableposprovider.cc
)

OD_ADD_SOURCE_FILES( marchingcubeslist.cc )

OD_INIT_MODULE()
