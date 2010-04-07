/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jube 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initgmt.cc,v 1.9 2010-04-07 09:26:18 cvsnageswara Exp $";


#include "gmtbasemap.h"
#include "gmtcoastline.h"
#include "gmtcontour.h"
#include "gmtfault.h"
#include "gmtlocations.h"
#include "gmt2dlines.h"
#include "initgmt.h"


void GMT::initStdClasses()
{
    GMTBaseMap::initClass();
    GMTLegend::initClass();
    GMTLocations::initClass();
    GMTPolyline::initClass();
    GMTContour::initClass();
    GMTFault::initClass();
    GMTCoastline::initClass();
    GMTWells::initClass();
    GMT2DLines::initClass();
    GMTRandLines::initClass();
    GMTCommand::initClass();
}
