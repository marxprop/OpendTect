/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectionselectorimpl.cc,v 1.2 2005-01-18 12:57:13 kristofer Exp $";

#include "sectionselectorimpl.h"

#include "binidsurface.h"
#include "emhorizon.h"
#include "geomtube.h"
#include "survinfo.h"
#include "trackplane.h"


namespace MPE 
{


BinIDSurfaceSourceSelector::BinIDSurfaceSourceSelector(
	const EM::Horizon& hor, const EM::SectionID& sid )
    : SectionSourceSelector( hor, sid )
    , surface( hor )
{}


void BinIDSurfaceSourceSelector::setTrackPlane( const MPE::TrackPlane& plane )
{
    if ( !plane.isVertical() )
	return;

    const BinID& startbid = plane.boundingBox().hrg.start;
    const BinID& stopbid = plane.boundingBox().hrg.stop;
    const BinID& step = plane.boundingBox().hrg.step;

    BinID currentbid( startbid );
    while ( true )
    {
	if ( surface.geometry.isDefined(sectionid, currentbid) &&
	     surface.geometry.isDefined(sectionid, currentbid+plane.motion().binid) )
	{
	    selpos += currentbid.getSerialized();
	}

	if ( startbid.inl==stopbid.inl )
	{
	    currentbid.crl += step.crl;
	    if ( currentbid.crl>stopbid.crl ) break;
	}
	else 
	{
	    currentbid.inl += step.inl;
	    if ( currentbid.inl>stopbid.inl ) break;
	}
    }
}


TubeSurfaceSourceSelector::TubeSurfaceSourceSelector(
	const EM::EMObject& obj_, const EM::SectionID& sid )
    : SectionSourceSelector( obj_, sid )
    , emobject( obj_ )
{}


void TubeSurfaceSourceSelector::setTrackPlane( const MPE::TrackPlane& plane )
{
    if ( !plane.isVertical() )
	return;

    mDynamicCastGet( const Geometry::Tube*,geomtube,
		     emobject.getElement(sectionid));

    TypeSet<GeomPosID> allnodes;
    geomtube->getPosIDs( allnodes );

    Interval<int> inlrange(plane.boundingBox().hrg.start.inl,
	    		   plane.boundingBox().hrg.stop.inl );
    Interval<int> crlrange(plane.boundingBox().hrg.start.crl,
	    		   plane.boundingBox().hrg.stop.crl );

    inlrange.include( plane.boundingBox().hrg.start.inl+
	    	      plane.motion().binid.inl);
    crlrange.include( plane.boundingBox().hrg.start.crl+
	    	      plane.motion().binid.crl);

    for ( int idx=0; idx<allnodes.size(); idx++ )
    {
	const RowCol node(allnodes[idx]);
	const Coord3 pos = geomtube->getKnot(node);
	const BinID bid = SI().transform(pos);
	if ( !inlrange.includes(bid.inl) || !crlrange.includes(bid.crl) )
	    continue;

	if ( !geomtube->isAtEdge(node) )
	    continue;

	selpos += allnodes[idx];
    }
}


};
