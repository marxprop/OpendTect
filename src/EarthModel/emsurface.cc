/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emsurface.h"

#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posfilter.h"
#include "posinfo2dsurv.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "unitofmeasure.h"
#include "zdomain.h"


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";
static const char* sSections = "Patches";


namespace EM
{

SurfaceIOData::SurfaceIOData()
    : zinfo_(&SI().zDomainInfo())
{}


SurfaceIOData::~SurfaceIOData()
{
    clear();
}


void SurfaceIOData::clear()
{
    rg.init(false);
    dbinfo = "";
    valnames.setEmpty();
    sections.setEmpty();
    linenames.setEmpty();
    linesets.setEmpty();
    geomids.setEmpty();
    trcranges.setEmpty();
}


void SurfaceIOData::setZDomain( const ZDomain::Info& zinfo )
{
    if ( zinfo_->isCompatibleWith(zinfo) )
	return;

    zinfo_ = &zinfo;
}


const ZDomain::Info& SurfaceIOData::zDomain() const
{
    return *zinfo_;
}


void SurfaceIOData::use( const Surface& surf )
{
    clear();

    mDynamicCastGet( const RowColSurfaceGeometry*, rcsg, &surf.geometry() );
    if ( rcsg )
    {
	StepInterval<int> hrg = rcsg->rowRange();
	rg.start_.inl() = hrg.start_; rg.stop_.inl() = hrg.stop_;
	rg.step_.inl() = hrg.step_;
	hrg = rcsg->colRange();
	rg.start_.crl() = hrg.start_; rg.stop_.crl() = hrg.stop_;
	rg.step_.crl() = hrg.step_;
    }

    for ( int idx=0; idx<surf.nrSections(); idx++ )
	sections += new BufferString( surf.sectionName(surf.sectionID(idx)) );

    mDynamicCastGet(const Horizon3D*,horizon,&surf);
    if ( horizon )
    {
	for ( int idx=0; idx<horizon->auxdata.nrAuxData(); idx++ )
	    valnames += new BufferString( horizon->auxdata.auxDataName(idx) );
    }

    linenames.erase();
    linesets.erase();
    geomids.erase();
    trcranges.erase();
    mDynamicCastGet(const Horizon2D*,horizon2d,&surf);
    if ( horizon2d )
    {
	const Horizon2DGeometry& emgeom = horizon2d->geometry();
	for ( int idx=0; idx<emgeom.nrLines(); idx++ )
	{
	    const Pos::GeomID geomid = emgeom.geomID( idx );
	    const BufferString linenm = emgeom.lineName( idx );
	    if ( mIsUdf(geomid) || linenm.isEmpty() )
		continue;

	    linenames.add( linenm );
	    geomids += geomid;
	    trcranges += emgeom.colRange( geomid );
	}
    }
}


void SurfaceIOData::fillPar( IOPar& iopar ) const
{
    iopar.set( sDbInfo, dbinfo );

    IOPar bidpar;
    rg.fillPar( bidpar );
    iopar.mergeComp( bidpar, sRange );

    IOPar valnmspar;
    valnames.fillPar( valnmspar );
    iopar.mergeComp( valnmspar, sValnms );

    IOPar sectionpar;
    sections.fillPar( sectionpar );
    iopar.mergeComp( sectionpar, sSections );

    int nrlines = geomids.size();
    if ( !nrlines ) nrlines = linenames.size();
    iopar.set( Horizon2DGeometry::sKeyNrLines(), nrlines );
    for ( int idx=0; idx<nrlines; idx++ )
    {
	if ( geomids.validIdx(idx) && geomids[idx].isValid() )
	    iopar.set( IOPar::compKey(sKey::GeomID(),idx), geomids[idx] );
	else if ( linesets.validIdx(idx) && linenames.validIdx(idx) )
	{
	    const Pos::GeomID geomid = Survey::GM().getGeomID(linesets.get(idx),
							   linenames.get(idx) );
	    if ( !geomid.is2D() )
		continue;

	    iopar.set( IOPar::compKey(sKey::GeomID(),idx), geomid );
	}

	BufferString key = IOPar::compKey( "Line", idx );
	iopar.set( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		   trcranges[idx] );
    }
}


void SurfaceIOData::usePar( const IOPar& iopar )
{
    iopar.get( sDbInfo, dbinfo );

    IOPar* bidpar = iopar.subselect(sRange);
    if ( bidpar ) rg.usePar( *bidpar );

    IOPar* valnmspar = iopar.subselect(sValnms);
    if ( valnmspar ) valnames.usePar( *valnmspar );

    IOPar* sectionpar = iopar.subselect(sSections);
    if ( sectionpar ) sections.usePar( *sectionpar );

    if ( iopar.hasKey(Horizon2DGeometry::sKeyNrLines()) )
    {
	int nrlines = 0;
	iopar.get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    BufferString key = IOPar::compKey( "Line", idx );
	    Pos::GeomID geomid;
	    if ( !iopar.get(IOPar::compKey(sKey::GeomID(),idx),geomid) )
	    {
		BufferString idstr;
		if ( iopar.get(IOPar::compKey(key,Horizon2DGeometry::sKeyID()),
			       idstr) )
		{
		    PosInfo::Line2DKey l2dkey; l2dkey.fromString( idstr );
		    if ( S2DPOS().curLineSetID() != l2dkey.lsID() )
			S2DPOS().setCurLineSet( l2dkey.lsID() );
		    geomid = Survey::GM().getGeomID(
				S2DPOS().getLineSet(l2dkey.lsID()),
				S2DPOS().getLineName(l2dkey.lineID()) );
		}
	    }

	    if ( !geomid.is2D() )
		continue;

	    const BufferString linenm = Survey::GM().getName( geomid );
	    if ( linenm.isEmpty() )
		continue;

	    geomids += geomid;
	    linenames.add( linenm );
	    StepInterval<int> trcrange;
	    iopar.get( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		       trcrange );
	    trcranges += trcrange;
	}
    }
}


// SurfaceIODataSelection
SurfaceIODataSelection::SurfaceIODataSelection( const SurfaceIOData& s )
    : sd(s)
{}


SurfaceIODataSelection::~SurfaceIODataSelection()
{}


void SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    seltrcranges = sd.trcranges;
    selvalues.erase(); selsections.erase();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;

    for ( int idx=0; idx<sd.sections.size(); idx++ )
	selsections += idx;
}


// Surface
Surface::Surface( EMManager& man)
    : EMObject( man )
{
}


Surface::~Surface()
{}


void Surface::removeAll()
{}

bool Surface::isAtEdge( const PosID& posid ) const
{ return geometry().isAtEdge(posid); }

bool Surface::isLoaded() const
{ return geometry().isLoaded(); }

Executor* Surface::saver()
{ return geometry().saver(); }

Executor* Surface::loader()
{ return geometry().loader(); }

Geometry::Element* Surface::geometryElementInternal()
{ return geometry().geometryElement(); }

Geometry::Element* Surface::sectionGeometryInternal( const SectionID& )
{ return geometryElementInternal(); }

EMObjectIterator* Surface::createIterator(  const TrcKeyZSampling* tkzs ) const
{ return geometry().createIterator( tkzs ); }

bool Surface::enableGeometryChecks( bool nv )
{ return geometry().enableChecks( nv ); }

bool Surface::isGeometryChecksEnabled() const
{ return geometry().isChecksEnabled(); }

const SurfaceGeometry& Surface::geometry() const
{ return const_cast<Surface*>(this)->geometry(); }


void Surface::apply( const Pos::Filter& pf )
{
    PtrMan<EM::EMObjectIterator>iterator = createIterator();
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( !pid.isValid() )
	    break;

	const Coord3 pos = getPos( pid );
        if ( !pf.includes(pos.coord(),(float)pos.z_) )
	   unSetPos( pid, false );
    }
}


BufferString Surface::getParFileName( const IOObj& ioobj )
{
    if ( !EM::canOverwrite(ioobj.key()) )
	return BufferString::empty();

    FilePath fp( ioobj.fullUserExpr(true) );
    fp.setExtension( "par" );
    return fp.fullPath();
}


BufferString Surface::getSetupFileName( const IOObj& ioobj )
{
    if ( !EM::canOverwrite(ioobj.key()) )
	return BufferString::empty();

    FilePath fp( ioobj.fullUserExpr(true) );
    fp.setExtension( "ts" );
    return fp.fullPath();
}


BufferString Surface::getParentChildFileName( const IOObj& ioobj )
{
    if ( !EM::canOverwrite(ioobj.key()) )
	return BufferString::empty();

    FilePath fp( ioobj.fullUserExpr(true) );
    fp.setExtension( "pcd" );
    return fp.fullPath();
}


bool Surface::usePar( const IOPar& par )
{
    return EMObject::usePar(par) && geometry().usePar(par);
}


void Surface::fillPar( IOPar& par ) const
{
    EMObject::fillPar( par );
    geometry().fillPar( par );
}



// HorizonGeometry
HorizonGeometry::HorizonGeometry( Surface& surf )
    : RowColSurfaceGeometry(surf)
{}


HorizonGeometry::~HorizonGeometry()
{}



// Horizon
Horizon::Horizon( EMManager& emm )
    : Surface(emm)
{}


Horizon::~Horizon()
{}


void Horizon::fillPar( IOPar& par ) const
{
    Surface::fillPar( par );
    par.set( sKey::StratRef(), stratlevelid_ );
}


bool Horizon::usePar( const IOPar& par )
{
    par.get( sKey::StratRef(), stratlevelid_ );
    return Surface::usePar( par );
}


IOObjContext Horizon::ioContext( bool is2d, const ZDomain::Info* zinfo,
			       bool forread )
{
    IOObjContext ctxt = is2d ? mIOObjContext(EMHorizon2D)
			     : mIOObjContext(EMHorizon3D);
    ctxt.forread_ = forread;
    if ( zinfo )
    {
	const ZDomain::Info& siinfo = SI().zDomainInfo();
	ctxt.requireZDomain( *zinfo, siinfo == *zinfo );
    }

    return ctxt;
}


IOObjContext Horizon::ioContext( bool is2d,  bool forread )
{
    return ioContext( is2d, &SI().zDomainInfo(), forread);
}

} // namespace EM
