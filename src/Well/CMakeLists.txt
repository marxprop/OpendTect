#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS General )
set( OD_FOLDER "Base" )

set( OD_MODULE_SOURCES
	initwell.cc
	directionalsurvey.cc
	laswriter.cc
	well.cc
	wellascio.cc
	wellcopier.cc
	welld2tmodel.cc
	welldatafilter.cc
	welldbupdater.cc
	welldisp.cc
	wellelasticmodelcomputer.cc
	wellextractdata.cc
	wellhdf5reader.cc
	wellhdf5writer.cc
	wellimplas.cc
	wellioprov.cc
	welllog.cc
	wellman.cc
	wellmarker.cc
	wellposprovider.cc
	wellreader.cc
	wellselection.cc
	wellt2dtransform.cc
	welltrack.cc
	welltransl.cc
	wellwriter.cc
	wellupdate.cc
)

OD_INIT_MODULE()
