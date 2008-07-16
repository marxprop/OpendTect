/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C. Glas
 * DATE     : November 2007
-*/

static const char* rcsID = "$Id: isocontourtracer.cc,v 1.3 2008-07-16 17:52:59 cvsnanne Exp $";

#include "isocontourtracer.h"

#include "arrayndinfo.h"


IsoContourTracer::IsoContourTracer( const Array2D<float>& field )
    : field_( field )
    , xsampling_( 0, field.info().getSize(0)-1, 1 )
    , ysampling_( 0, field.info().getSize(1)-1, 1 )
    , xrange_( 0, field.info().getSize(0)-1 )
    , yrange_( 0, field.info().getSize(1)-1 )
    , polyroi_( 0 )
{}


void IsoContourTracer::setSampling( const StepInterval<int>& xsamp,
				    const StepInterval<int>& ysamp )
{
    xsampling_ = xsamp;
    ysampling_ = ysamp;
    xsampling_.stop = xsampling_.atIndex( field_.info().getSize(0)-1 );
    ysampling_.stop = ysampling_.atIndex( field_.info().getSize(1)-1 );
}


void IsoContourTracer::selectRectROI( const Interval<int>& xintv,
				      const Interval<int>& yintv )
{
    xrange_.start = xsampling_.nearestIndex( xintv.start );
    xrange_.stop = xsampling_.nearestIndex( xintv.stop );
    yrange_.start = ysampling_.nearestIndex( yintv.start );
    yrange_.stop = ysampling_.nearestIndex( yintv.stop );
    xrange_.limitTo( Interval<int>(0, field_.info().getSize(0)-1) );
    yrange_.limitTo( Interval<int>(0, field_.info().getSize(1)-1) );
}

void IsoContourTracer::selectPolyROI( const ODPolygon<float>* poly )
{ polyroi_ = poly; }


#define mFieldX(idx) xsampling_.atIndex( xrange_.start+idx )
#define mFieldY(idy) ysampling_.atIndex( yrange_.start+idy )
#define mFieldZ(idx,idy) field_.get( xrange_.start+idx, yrange_.start+idy )

#define mMakeVertex( vertex, idx, idy, hor, frac ) \
\
    const Geom::Point2D<float> vertex( \
			(1.0-frac)*mFieldX(idx) + frac*mFieldX(idx+hor), \
			(1.0-frac)*mFieldY(idy) + frac*mFieldY(idy+1-hor) );


bool IsoContourTracer::getContours( ObjectSet<ODPolygon<float> >& contours,
				    float z, bool closedonly ) const
{
    deepErase( contours );
    Array3DImpl<float>* crossings = 
	new Array3DImpl<float>( xrange_.width()+1, yrange_.width()+1, 2 );

    findCrossings( *crossings, z );
    traceContours( *crossings, contours, closedonly );
    delete crossings;

    return !contours.isEmpty();
}


void IsoContourTracer::findCrossings( Array3DImpl<float>& crossings, 
				      float z ) const
{
    const int xsize = crossings.info().getSize(0);
    const int ysize = crossings.info().getSize(1);
    
    for ( int idx=0; idx<xsize; idx++)
    {
	for ( int idy=0; idy<ysize; idy++ )
	{
	    const float z0 = mFieldZ( idx, idy );
	    for ( int hor=0; hor<=1; hor++ )
	    {
		crossings.set( idx, idy, hor, mUdf(float) );

		if  ( (hor && idx==xsize-1) || (!hor && idy==ysize-1) )
		    continue;

		const float z1 = mFieldZ( idx+hor, idy+1-hor );

		if ( mIsUdf(z0) || mIsUdf(z1) )
		    continue;
		    
		if ( (z0<z && z<=z1) || (z1<z && z<=z0) )
		{
		    const float frac = (z-z0) / (z1-z0);

		    if ( polyroi_ )
		    {
			mMakeVertex( vertex, idx, idy, hor, frac );
			if ( !polyroi_->isInside(vertex, true, mDefEps) )
			    continue;
		    }

		    crossings.set( idx, idy, hor, frac );
		}
	    }
	}
    }
}


static bool nextCrossing( Array3DImpl<float>& crossings, int& idx, int& idy,
			  int& hor, int& up, float& frac ) 
{
    const int xsize = crossings.info().getSize(0);
    const int ysize = crossings.info().getSize(1);

    const int lidx =   up    ? idx : ( hor ? idx+1 : idx-1 );	/* [l]eft */
    const int lidy = up==hor ? idy : ( hor ? idy-1 : idy+1 );
    const int ridx = up!=hor ? idx : ( hor ? idx+1 : idx-1 );	/* [r]ight */
    const int ridy =   up    ? idy : ( hor ? idy-1 : idy+1 );

    float lfrac = mUdf(float); 
    if ( lidx>=0 && lidx<xsize && lidy>=0 && lidy<ysize ) 
	lfrac = crossings.get( lidx, lidy, 1-hor );

    float rfrac = mUdf(float); 
    if ( ridx>=0 && ridx<xsize && ridy>=0 && ridy<ysize ) 
	rfrac = crossings.get( ridx, ridy, 1-hor );

    if ( !mIsUdf(lfrac) && !mIsUdf(rfrac) )			/* Tie-break */
    {
	const float ldist =   up    ? lfrac : 1.0-lfrac;
	const float rdist =   up    ? rfrac : 1.0-rfrac;
	const float  dist = up==hor ?  frac : 1.0-frac;

	if ( ldist*ldist+dist*dist > rdist*rdist+(1.0-dist)*(1.0-dist) )
	    lfrac = mUdf(float);
	else
	    rfrac = mUdf(float);
    }

    if ( !mIsUdf(lfrac) )
    {
	idx = lidx; idy = lidy; frac = lfrac; hor = 1-hor;
	up = up==hor ? 1 : 0;
	return true;
    }

    if ( !mIsUdf(rfrac) )
    {
	idx = ridx; idy = ridy; frac = rfrac; hor = 1-hor;
	up = up==hor ? 0 : 1;
	return true;
    }
		
    const int sidx =  hor ? idx : ( up ? idx+1 : idx-1 );	/* [s]traight */
    const int sidy = !hor ? idy : ( up ? idy+1 : idy-1 );

    float sfrac = mUdf(float); 
    if ( sidx>=0 && sidx<xsize && sidy>=0 && sidy<ysize ) 
	sfrac = crossings.get( sidx, sidy, hor );
    
    if ( mIsUdf(sfrac) )
	return false;
    
    idx = sidx; idy = sidy; frac = sfrac;
    return true;
}


void IsoContourTracer::traceContours( Array3DImpl<float>& crossings,
				      ObjectSet<ODPolygon<float> >& contours,
				      bool closedonly ) const
{
    for ( int pidx=0; pidx<crossings.info().getSize(0); pidx++ )  /* [p]ivot */
    {
	for ( int pidy=0; pidy<crossings.info().getSize(1); pidy++ )
	{
	    for ( int phor=0; phor<=1; phor++ )
	    {
		const float pfrac = crossings.get( pidx, pidy, phor );
		if ( mIsUdf(pfrac) )
		    continue;

		ODPolygon<float>* contour = new ODPolygon<float>();
		contour->setClosed( false );
		
		for ( int pup=0; pup<=1; pup++ )
		{
		    int idx = pidx; int idy = pidy; int hor = phor; 
		    int up = pup; float frac = pfrac;
		    
		    if ( !pup && !nextCrossing(crossings,idx,idy,hor,up,frac) )
			continue;

		    do
		    {
			addVertex( *contour, pup, idx, idy, hor, frac );
			crossings.set( idx, idy, hor, mUdf(float) );
		    }
		    while( nextCrossing(crossings,idx,idy,hor,up,frac) );
			
		    if ( !pup && idx==pidx && idy==pidy && hor==phor )
		    {
			contour->setClosed( true );
			break;
		    }
		}

		const int sz = contour->size();
		if ( sz<2 || (closedonly && !contour->isClosed()) )
		    delete contour;
		else
		{
		    int idx = 0;
		    while ( idx<contours.size() && contours[idx]->size()>=sz )
			idx++;
		    contours.insertAt( contour, idx );
		}
	    }
	}
    }
}


void IsoContourTracer::addVertex( ODPolygon<float>& contour, bool headinsert,
				  int idx, int idy, int hor, float frac ) const
{
    mMakeVertex( vertex, idx, idy, hor, frac );

    const int prev = headinsert ? 0 : contour.size()-1;
    if ( contour.size() && vertex==contour.getVertex(prev) )
	return;

    contour.insert( (headinsert ? 0 : prev+1), vertex );
}
