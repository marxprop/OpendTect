/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emfault3d.h"

#include "emfaultauxdata.h"
#include "emfaultstickset.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "faultstickset.h"
#include "undo.h"
#include "posfilter.h"
#include "randcolor.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "zdomain.h"

namespace EM {

mImplementEMObjFuncs( Fault3D, EMFault3DTranslatorGroup::sGroupName() )


Fault3D::Fault3D( EMManager& em )
    : Fault(em)
    , geometry_( *this )
    , auxdata_( 0 )
{
    setPreferredColor( OD::getRandomColor() );
    setPosAttrMarkerStyle( 0,
	MarkerStyle3D(MarkerStyle3D::Cube,3,OD::Color::Yellow()) );

}


Fault3D::~Fault3D()
{
    delete auxdata_;
}


FaultAuxData* Fault3D::auxData()
{
    if ( !auxdata_ )
    {
	auxdata_ = new FaultAuxData( *this );
	if ( !auxdata_->init() )
	{
	    delete auxdata_;
	    auxdata_ = 0;
	}
    }

    return auxdata_;
}


Fault3DGeometry& Fault3D::geometry()
{ return geometry_; }


const Fault3DGeometry& Fault3D::geometry() const
{ return geometry_; }


const IOObjContext& Fault3D::getIOObjContext() const
{ return EMFault3DTranslatorGroup::ioContext(); }


uiString Fault3D::getUserTypeStr() const
{ return EMFault3DTranslatorGroup::sTypeName(); }


void Fault3D::apply( const Pos::Filter& pf )
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	mDynamicCastGet(Geometry::FaultStickSurface*,fssg,
			 geometryElement());
	if ( !fssg ) continue;

	const StepInterval<int> rowrg = fssg->rowRange();
	if ( rowrg.isUdf() ) continue;

	RowCol rc;
        for ( rc.row()=rowrg.stop_; rc.row()>=rowrg.start_; rc.row()-=rowrg.step_ )
	{
	    const StepInterval<int> colrg = fssg->colRange( rc.row() );
	    if ( colrg.isUdf() ) continue;

            for ( rc.col()=colrg.stop_; rc.col()>=colrg.start_;
                  rc.col()-=colrg.step_ )
	    {
		const Coord3 pos = fssg->getKnot( rc );
                if ( !pf.includes( (Coord) pos, (float) pos.z_) )
		    fssg->removeKnot( rc );
	    }
	}
    }

    // TODO: Handle cases in which fault becomes fragmented.
}


EMObjectIterator* Fault3D::createIterator( const TrcKeyZSampling* tkzs ) const
{
    return geometry_.createIterator( tkzs );
}



// Fault3DGeometry
Fault3DGeometry::Fault3DGeometry( Surface& surf )
    : FaultGeometry(surf)
{ }


Fault3DGeometry::~Fault3DGeometry()
{}


Geometry::FaultStickSurface* Fault3DGeometry::geometryElement()
{
    Geometry::Element* res = SurfaceGeometry::geometryElement();
    return sCast(Geometry::FaultStickSurface*,res);
}


const Geometry::FaultStickSurface* Fault3DGeometry::geometryElement() const
{
    const Geometry::Element* res = SurfaceGeometry::geometryElement();
    return sCast(const Geometry::FaultStickSurface*,res);
}


Geometry::FaultStickSurface* Fault3DGeometry::createGeometryElement() const
{ return new Geometry::FaultStickSurface; }


EMObjectIterator* Fault3DGeometry::createIterator(
					 const TrcKeyZSampling* tkzs ) const
{
    return new RowColIterator( surface_, tkzs );
}


int Fault3DGeometry::nrSticks() const
{
    const Geometry::FaultStickSurface* fss = geometryElement();
    return fss ? fss->nrSticks() : 0;
}


int Fault3DGeometry::nrKnots( int sticknr ) const
{
    const Geometry::FaultStickSurface* fss = geometryElement();
    return fss ? fss->nrKnots(sticknr) : 0;
}


bool Fault3DGeometry::insertStick( int sticknr,
				 int firstcol, const Coord3& pos,
				 const Coord3& editnormal, bool addtohistory )
{
    Geometry::FaultStickSurface* fss = geometryElement();
    if ( !fss || !fss->insertStick(pos,editnormal,sticknr, firstcol) )
	return false;


    if ( addtohistory )
    {
	const PosID posid( surface_.id(), SectionID::def(),
			   RowCol(sticknr,0).toInt64());
	UndoEvent* undo = new FaultStickUndoEvent( posid );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool Fault3DGeometry::removeStick( int sticknr, bool addtohistory )
{
    Geometry::FaultStickSurface* fss = geometryElement();
    if ( !fss )
	return false;

    const StepInterval<int> colrg = fss->colRange( sticknr );
    if ( colrg.isUdf() || colrg.width() )
	return false;

    const RowCol rc( sticknr, colrg.start_ );

    const Coord3 pos = fss->getKnot( rc );
    const Coord3 normal = getEditPlaneNormal( sticknr );
    if ( !normal.isDefined() || !pos.isDefined() )
	return false;

    if ( !fss->removeStick(sticknr) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), SectionID::def(), rc.toInt64() );

	UndoEvent* undo = new FaultStickUndoEvent( posid, pos, normal );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool Fault3DGeometry::insertKnot( const SubID& subid,
				const Coord3& pos, bool addtohistory )
{
    Geometry::FaultStickSurface* fss = geometryElement();
    RowCol rc = RowCol::fromInt64( subid );
    if ( !fss || !fss->insertKnot(rc,pos) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), SectionID::def(), subid );
	UndoEvent* undo = new FaultKnotUndoEvent( posid );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool Fault3DGeometry::areSticksVertical() const
{
    const Geometry::FaultStickSurface* fss = geometryElement();
    return fss ? fss->areSticksVertical() : false;
}


#define mEps 1e-3
bool Fault3DGeometry::areEditPlanesMostlyCrossline() const
{
    int nrcrls=0, nrnoncrls=0;
    const Coord crldir = SI().binID2Coord().crlDir().normalize();
    const Geometry::FaultStickSurface* fss = geometryElement();
    if ( !fss )
	return false;

    StepInterval<int> stickrg = fss->rowRange();
    for ( int sticknr=stickrg.start_; sticknr<=stickrg.stop_; sticknr++ )
    {
	const Coord3& normal = fss->getEditPlaneNormal( sticknr );
        if ( fabs(normal.z_) < 0.5 && mIsEqual(normal.x_,crldir.x_,mEps)
             && mIsEqual(normal.y_,crldir.y_,mEps) )
	    nrcrls++;
	else
	    nrnoncrls++;
    }

    return nrcrls > nrnoncrls;
}


bool Fault3DGeometry::removeKnot( const SubID& subid, bool addtohistory )
{
    Geometry::FaultStickSurface* fss = geometryElement();
    if ( !fss ) return false;

    RowCol rc = RowCol::fromInt64( subid );
    const Coord3 pos = fss->getKnot( rc );

    if ( !pos.isDefined() || !fss->removeKnot(rc) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), SectionID::def(), subid );
	UndoEvent* undo = new FaultKnotUndoEvent( posid, pos );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


#define mDefEditNormalStr( editnormstr, sid, sticknr ) \
    BufferString editnormstr("Edit normal of section "); \
    editnormstr += sid; editnormstr += " sticknr "; editnormstr += sticknr;

void Fault3DGeometry::fillPar( IOPar& par ) const
{
    const Geometry::FaultStickSurface* fss = geometryElement();
    if ( !fss )
	return;

    StepInterval<int> stickrg = fss->rowRange();
    for ( int sticknr=stickrg.start_; sticknr<=stickrg.stop_; sticknr++ )
    {
	mDefEditNormalStr( editnormstr, 0, sticknr );
	par.set( editnormstr.buf(), fss->getEditPlaneNormal(sticknr) );
    }
}


bool Fault3DGeometry::usePar( const IOPar& par )
{
    Geometry::FaultStickSurface* fss = geometryElement();
    if ( !fss )
	return false;

    StepInterval<int> stickrg = fss->rowRange();
    for ( int sticknr=stickrg.start_; sticknr<=stickrg.stop_; sticknr++ )
    {
	fss->setSticksVertical( false );
	mDefEditNormalStr( editnormstr, 0, sticknr );
	Coord3 editnormal( Coord3::udf() );
	par.get( editnormstr.buf(), editnormal );
	fss->addEditPlaneNormal( editnormal, sticknr );
        if ( editnormal.isDefined() && fabs(editnormal.z_)<0.5 )
	    fss->setSticksVertical( true );
    }

    FaultDataUpdater fssupdater( *fss );
    if ( fssupdater.execute() )
	tkzsenvelope_ = fssupdater.getEnvelope(); //Does not have zrange

    return true;
}


// ***** FaultAscIO *****
FaultAscIO::FaultAscIO( const Table::FormatDesc& fd )
    : Table::AscIO(fd)
{}


FaultAscIO::~FaultAscIO()
{}


Table::FormatDesc* FaultAscIO::getDesc( bool is2d, const ZDomain::Def& def )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Fault" );


    return fd;
}


void FaultAscIO::createDescBody( Table::FormatDesc* fd, bool is2d,
						    const ZDomain::Def& def )
{
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    if ( def.isTime() )
	fd->bodyinfos_ += Table::TargetInfo::mkTimePosition( true );
    else
	fd->bodyinfos_ += Table::TargetInfo::mkDepthPosition( true );

    fd->bodyinfos_ += new Table::TargetInfo( "Stick index", IntInpSpec(),
	Table::Optional );
    if ( is2d )
	fd->bodyinfos_ += new Table::TargetInfo( "Line name", StringInpSpec(),
	    Table::Required );
}


void FaultAscIO::updateDesc( Table::FormatDesc& fd, bool is2d,
					    const ZDomain::Def& def )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, is2d, def );
}


bool FaultAscIO::isXY() const
{
    const Table::TargetInfo* posinfo = fd_.bodyinfos_[0];
    return !posinfo || posinfo->selection_.form_ == 0;
}


bool FaultAscIO::get( od_istream& strm, EM::Fault& flt, bool sortsticks,
		      bool is2d ) const
{
    getHdrVals( strm );

    Coord3 crd;
    int curstickidx = -1;
    bool hasstickidx = false;

    bool oninl = false; bool oncrl = false; bool ontms = false;

    double firstz = mUdf(double);
    BinID firstbid;

    ObjectSet<Geometry::FaultStick> sticks;
    const bool isxy = isXY();
    const UnitOfMeasure* tabledepthunit = getDepthUnit();
    const UnitOfMeasure* emobjdepthunit = UnitOfMeasure::surveyDefDepthUnit();
    while ( true )
    {
	const int ret = getNextBodyVals( strm );
	if ( ret < 0 )
	    return false;

	if ( ret == 0 )	
	    break;

	if ( isxy )
	    crd = getPos3D( 0, 1, 2 );
	else
	{
	    Coord xycrd = SI().transform( getBinID(0,1) );
            crd.setXY( xycrd.x_, xycrd.y_ );
            crd.z_ = getDValue( 2 );
	}

	if ( !crd.isDefined() )
	    continue;

        convValue( crd.z_, tabledepthunit, emobjdepthunit );
	const int stickidx = getIntValue( 3 );

	BufferString lnm;
	if ( is2d )
	    lnm = getText( 4 );

	if ( sticks.isEmpty() && !mIsUdf(stickidx) )
	    hasstickidx = true;

	if ( !crd.isDefined() )
	    continue;

	if ( hasstickidx )
	{
	    if ( !mIsUdf(stickidx) && stickidx!=curstickidx )
	    {
		curstickidx = stickidx;
		sticks += new Geometry::FaultStick( curstickidx );
	    }
	}
	else
	{
	    const BinID curbid = SI().transform( crd );

	    oninl = oninl && curbid.inl()==firstbid.inl();
	    oncrl = oncrl && curbid.crl()==firstbid.crl();
            ontms = ontms && fabs(crd.z_-firstz) < fabs(0.5*SI().zStep());

	    if ( !oninl && !oncrl && !ontms )
	    {
		curstickidx++;
		sticks += new Geometry::FaultStick( stickidx );

                firstbid = curbid; firstz = crd.z_;
		oninl = true; oncrl = true; ontms = true;
	    }
	}

	sticks[ sticks.size()-1 ]->locs_ += crd;
	if ( !lnm.isEmpty() )
	    sticks[ sticks.size()-1 ]->geomid_ = Survey::GM().getGeomID( lnm );
    }

    // Index-based stick sort
    if ( sortsticks && hasstickidx )
    {
	for ( int idx=0; idx<sticks.size()-1; idx++ )
	{
	    for ( int idy=idx+1; idy<sticks.size(); idy++ )
	    {
		if ( sticks[idx]->getStickIdx() > sticks[idy]->getStickIdx() )
		    sticks.swap( idx, idy );
	    }
	}
    }

    int sticknr = !sticks.isEmpty() && hasstickidx ? sticks[0]->getStickIdx()
						   : 0;

    for ( int idx=0; idx<sticks.size(); idx++ )
    {
	Geometry::FaultStick* stick = sticks[idx];
	if ( stick->locs_.isEmpty() )
	    continue;

	if ( is2d )
	{
	    mDynamicCastGet(EM::FaultStickSet*,fss,&flt)
	    fss->geometry().insertStick( sticknr, 0,
				stick->locs_[0].pos(), stick->getNormal(),
				stick->geomid_, false );
	}
	else
	{
	    bool res = flt.geometry().insertStick( sticknr, 0,
			stick->locs_[0].pos(), stick->getNormal(), false);
	    if ( !res ) continue;
	}

	for ( int crdidx=1; crdidx<stick->locs_.size(); crdidx++ )
	{
	    const RowCol rc( sticknr, crdidx );
	    flt.geometry().insertKnot( rc.toInt64(),
				       stick->locs_[crdidx].pos(), false);
	}

	sticknr++;
    }

    deepErase( sticks );
    return true;
}


//FaultDataUpdater
FaultDataUpdater::FaultDataUpdater( Geometry::FaultStickSurface& fss )
    : ParallelTask("FaultStickSet Data Updater")
    , faultsurf_(fss)
{
    totnr_ = faultsurf_.nrSticks();
    tkzsenvelope_.setEmpty();
    tkzsenvelope_.hsamp_.setGeomID( Survey::default3DGeomID() );
    tkzsset_.setSize( totnr_, tkzsenvelope_ );
}


FaultDataUpdater::~FaultDataUpdater()
{}


od_int64 FaultDataUpdater::nrIterations() const
{
    return totnr_;
}


bool FaultDataUpdater::doWork( od_int64 start, od_int64 stop, int /**/ )
{
    TrcKey trckey( BinID::udf() );
    for ( int index=start; index<=stop; index++ )
    {
	Geometry::FaultStick* fss = faultsurf_.getStick( index );
	if ( !fss )
	    return true;

	const int size = fss->size();
	TrcKeyZSampling& tkzs = tkzsset_[index];
	for ( int kidx=0; kidx<size; kidx++ )
	{
	    LocationBase& loc = fss->locs_[kidx];
	    trckey.setFrom( loc.pos() );
	    loc.setTrcKey( trckey );
	    tkzs.hsamp_.include( trckey );
            tkzs.zsamp_.include( loc.pos().z_ );
	}
    }

    return true;
}


bool FaultDataUpdater::doFinish( bool success )
{
    if ( !success || tkzsset_.isEmpty() )
	return false;

    const int sz = tkzsset_.size();
    for ( int idx=0; idx<sz; idx++ )
	tkzsenvelope_.include( tkzsset_[idx] );

    return true;
}

} // namespace EM
