/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seisioobjinfo.h"
#include "seis2ddata.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seiscbvs2dlinegetter.h"
#include "seispsioprov.h"
#include "seisread.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "ptrman.h"
#include "globexpr.h"
#include "iostrm.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "conn.h"
#include "linekey.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "bufstringset.h"
#include "linekey.h"
#include "keystrs.h"
#include "zdomain.h"

#define mGoToSeisDir() \
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) )

#define mGetLineSet(nm,rv) \
    if ( !isOK() || !is2D() || isPS() ) return rv; \
 \
    PtrMan<Seis2DDataSet> nm \
	= new Seis2DDataSet( *ioobj_ ); \
    if ( nm->nrLines() == 0 ) \
	return rv


SeisIOObjInfo::SeisIOObjInfo( const IOObj* ioobj )
	: ioobj_(ioobj ? ioobj->clone() : 0)		{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const IOObj& ioobj )
	: ioobj_(ioobj.clone())				{ setType(); }
SeisIOObjInfo::SeisIOObjInfo( const MultiID& id )
	: ioobj_(IOM().get(id))				{ setType(); }


SeisIOObjInfo::SeisIOObjInfo( const char* ioobjnm )
	: ioobj_(0)
{
    mGoToSeisDir();
    ioobj_ = IOM().getLocal( ioobjnm, 0 );
    setType();
}


SeisIOObjInfo::SeisIOObjInfo( const SeisIOObjInfo& sii )
	: geomtype_(sii.geomtype_)
	, bad_(sii.bad_)
{
    ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
}


SeisIOObjInfo::~SeisIOObjInfo()
{
    delete ioobj_;
}


SeisIOObjInfo& SeisIOObjInfo::operator =( const SeisIOObjInfo& sii )
{
    if ( &sii != this )
    {
	delete ioobj_;
	ioobj_ = sii.ioobj_ ? sii.ioobj_->clone() : 0;
	geomtype_ = sii.geomtype_;
	bad_ = sii.bad_;
    }
    return *this;
}


void SeisIOObjInfo::setType()
{
    bad_ = !ioobj_;
    if ( bad_ ) return;

    const BufferString trgrpnm( ioobj_->group() );
    bool isps = false;
    if ( SeisTrcTranslator::isPS(*ioobj_) )
	isps = true;
    ioobj_->pars().getYN( SeisTrcTranslator::sKeyIsPS(), isps );

    if ( !isps && ioobj_->group()!=mTranslGroupName(SeisTrc) )
	{ bad_ = true; return; }

    const bool is2d = SeisTrcTranslator::is2D( *ioobj_, false );
    geomtype_ = isps ? (is2d ? Seis::LinePS : Seis::VolPS)
		     : (is2d ? Seis::Line : Seis::Vol);
}


SeisIOObjInfo::SpaceInfo::SpaceInfo( int ns, int ntr, int bps )
	: expectednrsamps(ns)
	, expectednrtrcs(ntr)
	, maxbytespsamp(bps)
{
    if ( expectednrsamps < 0 )
	expectednrsamps = SI().zRange(false).nrSteps() + 1;
    if ( expectednrtrcs < 0 )
	expectednrtrcs = mCast( int, SI().sampling(false).hrg.totalNr() );
}


#define mChk(ret) if ( bad_ ) return ret

bool SeisIOObjInfo::getDefSpaceInfo( SpaceInfo& spinf ) const
{
    mChk(false);

    if ( Seis::isPS(geomtype_) )
	{ pErrMsg( "No space info for PS" ); return false; }

    if ( Seis::is2D(geomtype_) )
    {
	mGetLineSet(lset,false);
	StepInterval<int> trcrg; StepInterval<float> zrg;
	BufferStringSet seen;
	spinf.expectednrtrcs = 0;
	for ( int idx=0; idx<lset->nrLines(); idx++ )
	{
	    const char* lnm = lset->lineName( idx );
	    if ( !seen.isPresent(lnm) )
	    {
		seen.add( lnm );
		lset->getRanges( idx, trcrg, zrg );
		spinf.expectednrtrcs += trcrg.nrSteps() + 1;
	    }
	}
	spinf.expectednrsamps = zrg.nrSteps() + 1;
	spinf.maxbytespsamp = 4;
	return true;
    }

    CubeSampling cs;
    if ( !getRanges(cs) )
	return false;

    spinf.expectednrsamps = cs.zrg.nrSteps() + 1;
    spinf.expectednrtrcs = mCast( int, cs.hrg.totalNr() );
    getBPS( spinf.maxbytespsamp, -1 );
    return true;
}


bool SeisIOObjInfo::isTime() const
{
    const bool siistime = SI().zIsTime();
    mChk(siistime);
    return ZDomain::isTime( ioobj_->pars() );
}


bool SeisIOObjInfo::isDepth() const
{
    const bool siisdepth = !SI().zIsTime();
    mChk(siisdepth);
    return ZDomain::isDepth( ioobj_->pars() );
}


const ZDomain::Def& SeisIOObjInfo::zDomainDef() const
{
    mChk(ZDomain::SI());
    return ZDomain::Def::get( ioobj_->pars() );
}


int SeisIOObjInfo::expectedMBs( const SpaceInfo& si ) const
{
    mChk(-1);

    if ( isPS() )
    {
	pErrMsg("TODO: no space estimate for PS");
	return -1;
    }

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		    ioobj_->createTranslator() );
    if ( !sttr )
	{ pErrMsg("No Translator!"); return -1; }

    if ( si.expectednrtrcs < 0 || mIsUdf(si.expectednrtrcs) )
	return -1;

    int overhead = sttr->bytesOverheadPerTrace();

    double sz = si.expectednrsamps;
    sz *= si.maxbytespsamp;
    sz = (sz + overhead) * si.expectednrtrcs;

    const double bytes2mb = 9.53674e-7;
    return (int)((sz * bytes2mb) + .5);
}


bool SeisIOObjInfo::getRanges( CubeSampling& cs ) const
{
    mChk(false);
    mDynamicCastGet(IOStream*,iostrm,ioobj_)
    cs.init( true );
    if ( is2D() || (iostrm && iostrm->isMulti()) )
	return false;

    if ( !isPS() )
	return SeisTrcTranslator::getRanges( *ioobj_, cs );

    SeisPS3DReader* rdr = SPSIOPF().get3DReader( *ioobj_ );
    if ( !rdr )
	return false;

    const PosInfo::CubeData& cd = rdr->posData();
    StepInterval<int> rg;
    cd.getInlRange( rg ); cs.hrg.setInlRange( rg );
    cd.getCrlRange( rg ); cs.hrg.setCrlRange( rg );
    for ( int icrl=rg.start; icrl<rg.stop; icrl += rg.step )
    {
	SeisTrc* trc = rdr->getTrace( BinID(cs.hrg.start.inl(),icrl) );
	if ( trc )
	    { cs.zrg = trc->zRange(); delete trc; break; }
    }
    delete rdr;
    return true;
}


bool SeisIOObjInfo::getBPS( int& bps, int icomp ) const
{
    mChk(false);
    if ( is2D() )
	return 4;

    if ( isPS() )
    {
	pErrMsg("TODO: no BPS for PS");
	return false;
    }

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		 ioobj_->createTranslator() );
    if ( !sttr )
	{ pErrMsg("No Translator!"); return false; }

    Conn* conn = ioobj_->getConn( Conn::Read );
    bool isgood = sttr->initRead(conn);
    bps = 0;
    if ( isgood )
    {
	ObjectSet<SeisTrcTranslator::TargetComponentData>& comps
		= sttr->componentInfo();
	for ( int idx=0; idx<comps.size(); idx++ )
	{
	    int thisbps = (int)comps[idx]->datachar.nrBytes();
	    if ( icomp < 0 )
		bps += thisbps;
	    else if ( icomp == idx )
		bps = thisbps;
	}
    }

    if ( bps == 0 ) bps = 4;
    return isgood;
}


void SeisIOObjInfo::getDefKeys( BufferStringSet& bss, bool add ) const
{
    if ( !add ) bss.erase();
    if ( !isOK() ) return;

    BufferString key( ioobj_->key().buf() );
    if ( !is2D() )
	{ bss.add( key.buf() ); bss.sort(); return; }
    else if ( isPS() )
	{ pErrMsg("2D PS not supported getting def keys"); return; }

    PtrMan<Seis2DDataSet> dataset
	= new Seis2DDataSet( *ioobj_ );
    if ( dataset->nrLines() == 0 )
	return;

    for ( int idx=0; idx<dataset->nrLines(); idx++ )
	bss.add( dataset->lineName(idx) );

    bss.sort();
}


#define mGetZDomainGE \
    const GlobExpr zdomge( o2d.zdomky_.isEmpty() ? ZDomain::SI().key() \
						 : o2d.zdomky_.buf() )
#define mChkOpts \
   if ( o2d.steerpol_ != 2 ) \
    { \
	const char* lndt = lset->dataType(); \
	const char* attrnm = lset->name(); \
	const bool issteer = (lndt && sKey::Steering()==lndt) || \
				(!lndt && sKey::Steering()==attrnm); \
	if ( (o2d.steerpol_ == 0 && issteer) \
	  || (o2d.steerpol_ == 1 && !issteer) ) \
	    continue; \
    } \
    if ( !zdomge.matches(lset->zDomainKey(idx)) ) \
	continue


void SeisIOObjInfo::getNms( BufferStringSet& bss,
			    const SeisIOObjInfo::Opts2D& o2d, bool attr ) const
{
    if ( !isOK() )
	return;

    if ( isPS() )
    {
	SPSIOPF().getLineNames( *ioobj_, bss );
	return;
    }

   if ( !isOK() || !is2D() || isPS() ) return;
 
    PtrMan<Seis2DDataSet> dset 
	= new Seis2DDataSet( *ioobj_ ); 
    if ( dset->nrLines() == 0 ) 
	return;
    mGetZDomainGE;

    BufferStringSet rejected;
    for ( int idx=0; idx<dset->nrLines(); idx++ )
    {
	const char* nm = dset->lineName(idx);
	if ( bss.isPresent(nm) )
	    continue;

	if ( o2d.bvs_ )
	{
	    if ( rejected.isPresent(nm) )
		continue;
	}

	bss.add( nm );
    }

    bss.sort();
}


void SeisIOObjInfo::getNmsSubSel( const char* nm, BufferStringSet& bss,
			    const SeisIOObjInfo::Opts2D& o2d, bool l4a ) const
{
    if ( !nm || !*nm ) return;

    mGetLineSet(lset,);
    mGetZDomainGE;
    const BufferString target( nm );
    for ( int idx=0; idx<lset->nrLines(); idx++ )
    {
	const char* lnm = lset->lineName( idx );
	const char* requested = lnm;
	const char* listadd = lnm;

	if ( target == requested )
	{
	    mChkOpts;
	    bss.addIfNew( listadd );
	}
    }

    bss.sort();
}


bool SeisIOObjInfo::getRanges( const Pos::GeomID geomid, 
			       StepInterval<int>& trcrg,
			       StepInterval<float>& zrg ) const
{
    mChk(false);
    PtrMan<Seis2DDataSet> dataset = 
				new Seis2DDataSet( *ioobj_ );
    if ( dataset->nrLines() == 0 )
	return false;

    const int lidx = dataset->indexOf( geomid );
    if ( lidx < 0 )
	return false;

    return dataset->getRanges( lidx, trcrg, zrg );
}


BufferString SeisIOObjInfo::defKey2DispName( const char* defkey,
					     const char* ioobjnm )
{
    if ( !IOObj::isKey(defkey) )
	return BufferString( defkey );

    PtrMan<IOObj> ioobj = IOM().get( MultiID(defkey) );
    if ( !ioobj )
	return BufferString( "" );

    if ( SeisTrcTranslator::is2D(*ioobj,false) )
	return LineKey::defKey2DispName( defkey, ioobjnm );

    BufferString ret( "[",
		      ioobjnm && *ioobjnm ? ioobjnm : ioobj->name().buf(),
		      "]" );
    return ret;
}


BufferString SeisIOObjInfo::def3DDispName( const char* defkey,
					   const char* ioobjnm )
{
    if ( !IOObj::isKey(defkey) )
	return BufferString( defkey );

    BufferString attrnm( "[", ioobjnm, "]" );
    return attrnm;
}

static BufferStringSet& getTypes()
{
    mDefineStaticLocalObject( BufferStringSet, types, );
    return types;
}

static TypeSet<MultiID>& getIDs()
{
    mDefineStaticLocalObject( TypeSet<MultiID>, ids, );
    return ids;
}


void SeisIOObjInfo::initDefault( const char* typ )
{
    BufferStringSet& typs = getTypes();
    if ( typs.isPresent(typ) ) return;

    IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    ctxt.toselect.require_.set( sKey::Type(), typ );
    ctxt.toselect.allowtransls_ = CBVSSeisTrcTranslator::translKey();
    int nrpresent = 0;
    PtrMan<IOObj> ioobj = IOM().getFirst( ctxt, &nrpresent );
    if ( !ioobj || nrpresent > 1 )
	return;

    typs.add( typ );
    getIDs() += ioobj->key();
}


const MultiID& SeisIOObjInfo::getDefault( const char* typ )
{
    mDefineStaticLocalObject( const MultiID, noid, ("") );
    const int typidx = getTypes().indexOf( typ );
    return typidx < 0 ? noid : getIDs()[typidx];
}


void SeisIOObjInfo::setDefault( const MultiID& id, const char* typ )
{
    BufferStringSet& typs = getTypes();
    TypeSet<MultiID>& ids = getIDs();

    const int typidx = typs.indexOf( typ );
    if ( typidx > -1 )
	ids[typidx] = id;
    else
    {
	typs.add( typ );
	ids += id;
    }
}


int SeisIOObjInfo::nrComponents( Pos::GeomID geomid ) const
{
    return getComponentInfo( geomid, 0 );
}


void SeisIOObjInfo::getComponentNames( BufferStringSet& nms, 
				       Pos::GeomID geomid ) const
{
    getComponentInfo( geomid, &nms );
}


void SeisIOObjInfo::getCompNames( const MultiID& mid, BufferStringSet& nms )
{
    SeisIOObjInfo ioobjinf( mid );
    ioobjinf.getComponentNames( nms, Survey::GM().cUndefGeomID() );
}


int SeisIOObjInfo::getComponentInfo( Pos::GeomID geomid, 
				     BufferStringSet* nms ) const
{
    int ret = 0; if ( nms ) nms->erase();
    mChk(ret);
    if ( isPS() )
	return 0;

    if ( !is2D() )
    {
	mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> sttr,
		     ioobj_->createTranslator() );
	if ( !sttr )
	    { pErrMsg("No Translator!"); return 0; }
	Conn* conn = ioobj_->getConn( Conn::Read );
	if ( sttr->initRead(conn) )
	{
	    ret = sttr->componentInfo().size();
	    if ( nms )
	    {
		for ( int icomp=0; icomp<ret; icomp++ )
		    nms->add( sttr->componentInfo()[icomp]->name() );
	    }
	}
    }
    else
    {
	PtrMan<Seis2DDataSet> dataset = new Seis2DDataSet( *ioobj_ );
	if ( !dataset || dataset->nrLines() == 0 )
	    return 0;

	int lidx = dataset->indexOf( geomid );
	if ( lidx < 0 ) lidx = 0;
	SeisTrcBuf tbuf( true );
	Executor* ex = dataset->lineFetcher( lidx, tbuf, 1 );
	if ( ex ) ex->doStep();
	ret = tbuf.isEmpty() ? 0 : tbuf.get(0)->nrComponents();
	if ( nms )
	{
	    mDynamicCastGet(SeisCBVS2DLineGetter*,lg,ex)
	    if ( !lg )
	    {
		for ( int icomp=0; icomp<ret; icomp++ )
		    nms->add( BufferString("[",icomp+1,"]") );
	    }
	    else
	    {
		ret = lg->tr_ ? lg->tr_->componentInfo().size() : 0;
		if ( nms )
		{
		    for ( int icomp=0; icomp<ret; icomp++ )
			nms->add( lg->tr_->componentInfo()[icomp]->name() );
		}
	    }
	}
	delete ex;
    }

    return ret;
}


void SeisIOObjInfo::get2DDataSetInfo( BufferStringSet& datasets,
				      TypeSet<MultiID>* setids,
				      TypeSet<BufferStringSet>* linenames )
{
    datasets.erase();
    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Seis)->id );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( !(*ioobj.group() == 'T' || *ioobj.translator() == 'T')
	  || SeisTrcTranslator::isPS(ioobj) ) continue;

	datasets.add( ioobj.name() );
	if ( setids )
	    *setids += ioobj.key();

	if ( linenames )
	{
	    SeisIOObjInfo oinf( ioobj );
	    BufferStringSet lnms;
	    oinf.getLineNames( lnms );
	    *linenames += lnms;
	}
    }
}


void SeisIOObjInfo::get2DLineInfo( BufferStringSet& linesets,
				   TypeSet<MultiID>* setids,
				   TypeSet<BufferStringSet>* linenames )
{
    const MultiID mid ( IOObjContext::getStdDirData(IOObjContext::Seis)->id );
    const IODir iodir( mid );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( !(*ioobj.group() == '2' || *ioobj.translator() == '2')
	  || SeisTrcTranslator::isPS(ioobj) ) continue;

	linesets.add( ioobj.name() );
	if ( setids )
	    *setids += ioobj.key();

	if ( linenames )
	{
	    SeisIOObjInfo oinf( ioobj );
	    BufferStringSet lnms;
	    oinf.getLineNames( lnms );
	    *linenames += lnms;
	}
    }
}
