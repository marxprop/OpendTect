/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "polygonsurfeditor.h"

#include "emmanager.h"
#include "empolygonbody.h"
#include "mpeengine.h"
#include "polygonsurfaceedit.h"
#include "selector.h"
#include "survinfo.h"
#include "trigonometry.h"
#include "undo.h"


RefMan<MPE::PolygonBodyEditor>
	MPE::PolygonBodyEditor::create( const EM::PolygonBody& polygonsurf )
{
    return new PolygonBodyEditor( polygonsurf );
}


MPE::PolygonBodyEditor::PolygonBodyEditor( const EM::PolygonBody& polygonsurf )
    : ObjectEditor(polygonsurf)
    , sowingpivot_(Coord3::udf())
{}


MPE::PolygonBodyEditor::~PolygonBodyEditor()
{}


Geometry::ElementEditor* MPE::PolygonBodyEditor::createEditor()
{
    RefMan<EM::EMObject> emobject = emObject();
    Geometry::Element* ge = emobject ? emobject->geometryElement() : nullptr;
    mDynamicCastGet(Geometry::PolygonSurface*,surface,ge);
    return surface ? new Geometry::PolygonSurfEditor( *surface ) : nullptr;
}


void MPE::PolygonBodyEditor::setLastClicked( const EM::PosID& pid )
{
    RefMan<EM::EMObject> emobject = emObject();
    if ( !emobject )
	return;

    lastclickedpid() = pid;

    if ( sowingpivot_.isDefined() )
    {
	const Coord3 pos = emobject->getPos( pid );
	if ( pos.isDefined() )
	    sowinghistory_.insert( 0, pos );
    }
}


void MPE::PolygonBodyEditor::setSowingPivot( const Coord3 pos )
{
    if ( sowingpivot_.isDefined() && !pos.isDefined() )
	sowinghistory_.erase();

    sowingpivot_ = pos;
}


#define mCompareCoord( crd ) Coord3( crd, crd.z_*zfactor )

void MPE::PolygonBodyEditor::getInteractionInfo( EM::PosID& nearestpid0,
					    EM::PosID& nearestpid1,
					    EM::PosID& insertpid,
					    const Coord3& mousepos,
					    float zfactor ) const
{
    ConstRefMan<EM::EMObject> emobject = emObject();
    if ( !emobject )
	return;

    nearestpid0 = EM::PosID::udf();
    nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();

    const Coord3& pos = sowingpivot_.isDefined() && sowinghistory_.isEmpty()
			? sowingpivot_ : mousepos;

    int polygon;
    const float mindist = getNearestPolygon( polygon, pos, zfactor );
    if ( mIsUdf(mindist) )
    {
	if ( !emobject->nrSections() )
	    return;

	const Geometry::Element* ge = emobject->geometryElement();
	if ( !ge )
	    return;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface )
	    return;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( !rowrange.isUdf() )
	    return;

	insertpid.setObjectID( emobject->id() );
	insertpid.setSubID( RowCol(0,0).toInt64() );
	return;
    }

    if ( fabs(mindist)>50 )
    {
	if ( !emobject->nrSections() )
	    return;

	const Geometry::Element* ge = emobject->geometryElement();
	if ( !ge )
	    return;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface )
	    return;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    return;

	insertpid.setObjectID( emobject->id() );
	const int newpolygon = mindist>0
	       ? polygon+rowrange.step_
	       : polygon==rowrange.start_ ? polygon-rowrange.step_ : polygon;

	insertpid.setSubID( RowCol(newpolygon,0).toInt64() );
	return;
    }

    getPidsOnPolygon( nearestpid0, nearestpid1, insertpid, polygon,
		      pos, zfactor );
}


bool MPE::PolygonBodyEditor::removeSelection( const Selector<Coord3>& selector )
{
    RefMan<EM::EMObject> emobject = emObject();
    mDynamicCastGet(EM::PolygonBody*,polygonsurf,emobject.ptr());
    if ( !polygonsurf )
	return false;

    bool change = false;
    for ( int sectidx=polygonsurf->nrSections()-1; sectidx>=0; sectidx--)
    {
	const Geometry::Element* ge = polygonsurf->geometryElement();
	if ( !ge )
	    continue;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface )
	    continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int polygonidx=rowrange.nrSteps(); polygonidx>=0; polygonidx-- )
	{
	    Coord3 avgpos( 0, 0, 0 );
	    const int curpolygon = rowrange.atIndex(polygonidx);
	    const StepInterval<int> colrange = surface->colRange( curpolygon );
	    if ( colrange.isUdf() )
		continue;

	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const RowCol rc( curpolygon,colrange.atIndex(knotidx) );
		const Coord3 pos = surface->getKnot( rc );

		if ( !pos.isDefined() || !selector.includes(pos) )
		    continue;

		EM::PolygonBodyGeometry& fg = polygonsurf->geometry();
		const bool res = fg.nrKnots(curpolygon)==1
		   ? fg.removePolygon(curpolygon, true )
		   : fg.removeKnot( rc.toInt64(), true );

		if ( res ) change = true;
	    }
	}
    }

    if ( change )
    {
	EM::EMM().undo(emobject->id()).setUserInteractionEnd(
		EM::EMM().undo(emobject->id()).currentEventID() );
    }

    return change;
}


float MPE::PolygonBodyEditor::getNearestPolygon( int& polygon,
				const Coord3& mousepos, float zfactor ) const
{
    if ( !mousepos.isDefined() )
	return mUdf(float);

    ConstRefMan<EM::EMObject> emobject = emObject();
    if ( !emobject )
	return mUdf(float);

    int selsectionidx = -1, selpolygon = mUdf(int);
    float mindist = mUdf(float);

    for ( int sectionidx=emobject->nrSections()-1; sectionidx>=0; sectionidx--)
    {
	const Geometry::Element* ge = emobject->geometryElement();
	if ( !ge )
	    continue;

	mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
	if ( !surface )
	    continue;

	const StepInterval<int> rowrange = surface->rowRange();
	if ( rowrange.isUdf() )
	    continue;

	for ( int polygonidx=rowrange.nrSteps(); polygonidx>=0; polygonidx-- )
	{
	    Coord3 avgpos( 0, 0, 0 );
	    const int curpolygon = rowrange.atIndex(polygonidx);
	    const StepInterval<int> colrange = surface->colRange( curpolygon );
	    if ( colrange.isUdf() )
		continue;

	    int count = 0;
	    for ( int knotidx=colrange.nrSteps(); knotidx>=0; knotidx-- )
	    {
		const Coord3 pos = surface->getKnot(
			RowCol(curpolygon,colrange.atIndex(knotidx)));

		if ( pos.isDefined() )
		{
		    avgpos += mCompareCoord( pos );
		    count++;
		}
	    }

	    if ( !count )
		continue;

	    avgpos /= count;

	    const Plane3 plane( surface->getPolygonNormal(curpolygon),
				avgpos, false );
	    const float disttoplane = (float)
		plane.distanceToPoint( mCompareCoord(mousepos), true );

	    if ( selsectionidx==-1 || fabs(disttoplane)<fabs(mindist) )
	    {
		mindist = disttoplane;
		selpolygon = curpolygon;
		selsectionidx = sectionidx;
	    }
	}
    }

    if ( selsectionidx==-1 )
	return mUdf(float);

    polygon = selpolygon;

    return mindist;
}


#define mRetNotInsideNext \
    if ( !nextdefined || (!sameSide3D(curpos,nextpos,v0,v1,0) && \
			  !sameSide3D(v0,v1,curpos,nextpos,0)) ) \
	return false;

#define mRetNotInsidePrev \
    if ( !prevdefined || (!sameSide3D(curpos,prevpos,v0,v1,0) && \
			  !sameSide3D(v0,v1,curpos,prevpos,0)) ) \
	return false;


bool MPE::PolygonBodyEditor::setPosition( const EM::PosID& pid,
					  const Coord3& mpos )
{
    if ( !mpos.isDefined() )
	return false;

    const BinID bid = SI().transform( mpos );
    if ( !SI().inlRange( true ).includes(bid.inl(),false) ||
	 !SI().crlRange( true ).includes(bid.crl(),false) ||
         !SI().zRange( true ).includes(mpos.z_,false) )
	return false;

    RefMan<EM::EMObject> emobject = emObject();
    Geometry::Element* ge = emobject ? emobject->geometryElement() : nullptr;
    mDynamicCastGet(Geometry::PolygonSurface*,surface,ge);
    if ( !surface )
	return false;

    const RowCol rc = pid.getRowCol();
    const StepInterval<int> colrg = surface->colRange( rc.row() );
    if ( colrg.isUdf() )
	return false;

    const bool addtoundo = changedpids.indexOf(pid) == -1;
    if ( addtoundo )
	changedpids += pid;

    if ( colrg.nrSteps()<3 )
	return emobject->setPos( pid, mpos, addtoundo );

    const int zscale =  SI().zDomain().userFactor();
    const int previdx =
            rc.col()==colrg.start_ ? colrg.stop_ : rc.col()-colrg.step_;
    const int nextidx =
            rc.col()<colrg.stop_ ? rc.col()+colrg.step_ : colrg.start_;

    Coord3 curpos = mpos; curpos.z_ *= zscale;
    Coord3 prevpos = surface->getKnot( RowCol(rc.row(), previdx) );
    Coord3 nextpos = surface->getKnot( RowCol(rc.row(), nextidx) );

    const bool prevdefined = prevpos.isDefined();
    const bool nextdefined = nextpos.isDefined();
    if ( prevdefined ) prevpos.z_ *= zscale;
    if ( nextdefined ) nextpos.z_ *= zscale;

    for ( int knot=colrg.start_; knot<=colrg.stop_; knot += colrg.step_ )
    {
        const int nextknot = knot<colrg.stop_ ? knot+colrg.step_ : colrg.start_;
	if ( knot==previdx || knot==rc.col() )
	    continue;

	Coord3 v0 = surface->getKnot( RowCol(rc.row(), knot) );
	Coord3 v1 = surface->getKnot( RowCol(rc.row(),nextknot));
	if ( !v0.isDefined() || !v1.isDefined() )
	    return false;

        v0.z_ *= zscale;
        v1.z_ *= zscale;
	if ( previdx==nextknot )
	{
	    mRetNotInsideNext
	}
	else if ( knot==nextidx )
	{
	    mRetNotInsidePrev
	}
	else
	{
	    mRetNotInsidePrev
	    mRetNotInsideNext
	}
    }

    return emobject->setPos( pid, mpos, addtoundo );
}


void MPE::PolygonBodyEditor::getPidsOnPolygon(	EM::PosID& nearestpid0,
		    EM::PosID& nearestpid1, EM::PosID& insertpid, int polygon,
		    const Coord3& mousepos, float zfactor ) const
{
    nearestpid0 = EM::PosID::udf();
    nearestpid1 = EM::PosID::udf();
    insertpid = EM::PosID::udf();
    if ( !mousepos.isDefined() ) return;

    ConstRefMan<EM::EMObject> emobject = emObject();
    const Geometry::Element* ge = emobject ? emobject->geometryElement()
					   : nullptr;
    mDynamicCastGet(const Geometry::PolygonSurface*,surface,ge);
    if ( !surface )
	return;

    const StepInterval<int> colrange = surface->colRange( polygon );
    if ( colrange.isUdf() )
	return;

    const Coord3 mp = mCompareCoord(mousepos);
    TypeSet<int> knots;
    int nearknotidx = -1;
    Coord3 nearpos;
    float minsqptdist = mUdf(float);
    for ( int knotidx=0; knotidx<colrange.nrSteps()+1; knotidx++ )
    {
	const Coord3 pt =
	    surface->getKnot( RowCol(polygon,colrange.atIndex(knotidx)) );
	if ( !pt.isDefined() )
	    continue;

	float sqdist = 0;
	if ( sowinghistory_.isEmpty() || sowinghistory_[0]!=pt )
	{
	    sqdist = (float) mCompareCoord(pt).sqDistTo(
		mCompareCoord(mousepos) );
	    if ( mIsZero(sqdist, 1e-4) ) //mousepos is duplicated.
		return;
	}

	 if ( nearknotidx==-1 || sqdist<minsqptdist )
	 {
	     minsqptdist = sqdist;
	     nearknotidx = knots.size();
	     nearpos = mCompareCoord( pt );
	 }

	 knots += colrange.atIndex( knotidx );
    }

    if ( nearknotidx==-1 )
	return;

    nearestpid0.setObjectID( emobject->id() );
    nearestpid0.setSubID( RowCol(polygon,knots[nearknotidx]).toInt64() );
    if ( knots.size()<=2 )
    {
	insertpid = nearestpid0;
	insertpid.setSubID( RowCol(polygon,knots.size()).toInt64() );
	return;
    }

    double minsqedgedist = -1;
    int nearedgeidx = mUdf(int);
    Coord3 v0, v1;
    for ( int knotidx=0; knotidx<knots.size(); knotidx++ )
    {
	const int col = knots[knotidx];
	Coord3 p0 = surface->getKnot(RowCol(polygon,col));
	Coord3 p1 = surface->getKnot( RowCol(polygon,
		    knots [ knotidx<knots.size()-1 ? knotidx+1 : 0 ]) );
	if ( !p0.isDefined() || !p1.isDefined() )
	    continue;

        p0.z_ *= zfactor;
        p1.z_ *= zfactor;

	const double t = (mp-p0).dot(p1-p0)/(p1-p0).sqAbs();
	if ( t<0 || t>1 )
	    continue;

	const double sqdist = mp.sqDistTo(p0+t*(p1-p0));
	if ( minsqedgedist==-1 || sqdist<minsqedgedist )
	{
	    minsqedgedist = sqdist;
	    nearedgeidx = knotidx;
	    v0 = p0;
	    v1 = p1;
	}
    }

    bool usenearedge = false;
    if ( minsqedgedist!=-1 && sowinghistory_.size()<=1 )
    {
	if ( nearknotidx==nearedgeidx ||
	     nearknotidx==(nearknotidx<knots.size()-1 ? nearknotidx+1 : 0) ||
	     ((v1-nearpos).cross(v0-nearpos)).dot((v1-mp).cross(v0-mp))<0  ||
	     minsqedgedist<minsqptdist )
	    usenearedge = true;
    }

    if ( usenearedge ) //use nearedgeidx only
    {
	if ( nearedgeidx<knots.size()-1 )
	{
	    nearestpid0.setSubID(
		    RowCol(polygon,knots[nearedgeidx]).toInt64() );
	    nearestpid1 = nearestpid0;
	    nearestpid1.setSubID(
		    RowCol(polygon,knots[nearedgeidx+1]).toInt64() );

	    insertpid = nearestpid1;
	}
	else
	{
	    insertpid = nearestpid0;
            const int nextcol = knots[nearedgeidx]+colrange.step_;
	    insertpid.setSubID( RowCol(polygon,nextcol).toInt64() );
	}

	return;
    }
    else  //use nearknotidx only
    {
	Coord3 prevpos = surface->getKnot( RowCol(polygon,
		    knots[nearknotidx ? nearknotidx-1 : knots.size()-1]) );
	Coord3 nextpos = surface->getKnot( RowCol(polygon,
		    knots[nearknotidx<knots.size()-1 ? nearknotidx+1 : 0]) );

	bool takeprevious;
	if ( sowinghistory_.size() <= 1 )
	{
	    const bool prevdefined = prevpos.isDefined();
	    const bool nextdefined = nextpos.isDefined();
            if ( prevdefined ) prevpos.z_ *= zfactor;
            if ( nextdefined ) nextpos.z_ *= zfactor;

	    takeprevious = prevdefined && nextdefined &&
			   sameSide3D(mp,prevpos,nearpos,nextpos,1e-3);
	}
	else
	    takeprevious = sowinghistory_[1]==prevpos;

	if ( takeprevious )
	{
	    if ( nearknotidx )
	    {
		nearestpid1 = nearestpid0;
		nearestpid1.setSubID(
			RowCol(polygon,knots[nearknotidx-1]).toInt64() );
		insertpid = nearestpid0;
	    }
	    else
	    {
		insertpid = nearestpid0;
                const int insertcol = knots[nearknotidx]-colrange.step_;
		insertpid.setSubID(RowCol(polygon,insertcol).toInt64());
	    }
	}
	else
	{
	    if ( nearknotidx<knots.size()-1 )
	    {
		nearestpid1 = nearestpid0;
		nearestpid1.setSubID(
			RowCol(polygon,knots[nearknotidx+1]).toInt64() );
		insertpid = nearestpid1;
	    }
	    else
	    {
		insertpid = nearestpid0;
                const int insertcol = knots[nearknotidx]+colrange.step_;
		insertpid.setSubID(RowCol(polygon,insertcol).toInt64());
	    }
	}
    }
}


EM::PosID& MPE::PolygonBodyEditor::lastclickedpid()
{
    static EM::PosID lastclickedpid_;
    return lastclickedpid_;
}
