#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiIo Seis Velocity )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
	inituiseis.cc
	uibatchtime2depthsetup.cc
	uiclusterproc.cc
	uielasticpropsel.cc
	uielasticmodelprov.cc
	uifkspectrum.cc
	uifreqtaper.cc
	uigisexp2dlines.cc
	uimergeseis.cc
	uimultcomputils.cc
	uiprestkmergedlg.cc
	uiprestacksel.cc
	uiseis2dfileman.cc
	uiseis2dfrom3d.cc
	uiseis2dgeom.cc
	uiseis2dto3d.cc
	uiseis2dto3dinterpol.cc
	uiseisbayesclass.cc
	uiseisbrowser.cc
	uiseiscbvsimpfromothersurv.cc
	uiseiscopy.cc
	uiseisdirectfiledatadlg.cc
	uiseisexpcubepositions.cc
	uiseisfileman.cc
	uiseisfmtscale.cc
	uiseisimportcbvs.cc
	uiseisioobjinfo.cc
	uiseisiosimple.cc
	uiseislinesel.cc
	uiseismmproc.cc
	uiseismulticubeps.cc
	uiseispartserv.cc
	uiseispreloadmgr.cc
	uiseispsman.cc
	uiseisrandto2dline.cc
	uiseissel.cc
	uiseissingtrcdisp.cc
	uiseissubsel.cc
	uiseistransf.cc
	uiseistrcbufviewer.cc
	uiseiswriteopts.cc
	uiseiswvltattr.cc
	uiseiswvltgen.cc
	uiseiswvltimpexp.cc
	uiseiswvltman.cc
	uiseiswvltsel.cc
	uistaticsdesc.cc
	uisynthgrp.cc
	uitrcpositiondlg.cc
	uiveldesc.cc
	uivelocityvolumeconversion.cc
	uiwaveletextraction.cc
	uiwaveletmatchdlg.cc
)

set( OD_FIREWALL_EXCEPTION_EXEC od_SeisMMBatch.cc )
set( OD_FIREWALL_EXECEP_DESC "Distributed Computing Client" )
set( OD_FIREWALL_TYPE OD )

set( OD_MODULE_PROGS
	${OD_FIREWALL_EXCEPTION_EXEC}
	od_ClusterProc.cc
)

OD_INIT_MODULE()
