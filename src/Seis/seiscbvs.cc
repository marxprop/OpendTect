/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seiscbvs.h"

#include "cbvsreader.h"
#include "cbvsreadmgr.h"
#include "cbvswritemgr.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iostrm.h"
#include "oddirs.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "seistrc.h"
#include "separstr.h"
#include "strmprov.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"


const char* CBVSSeisTrcTranslator::sKeyDefExtension()	{ return "cbvs"; }

CBVSSeisTrcTranslator::CBVSSeisTrcTranslator( const char* nm, const char* unm )
    : SeisTrcTranslator(nm,unm)
    , coordpol_((int)CBVSIO::NotStored)
    , brickspec_(*new VBrickSpec)
    , auxinf_(false)
{
    if ( Survey::isValidGeomID(geomid_) )
	auxinf_.trckey_.setGeomID( geomid_ );
}


CBVSSeisTrcTranslator::~CBVSSeisTrcTranslator()
{
    cleanUp();
    delete &brickspec_;
}


CBVSSeisTrcTranslator* CBVSSeisTrcTranslator::make( const char* fnm,
	bool infoonly, bool is2d, uiString* msg, bool forceusecbvsinf )
{
    if ( !fnm || !*fnm )
    {
	if ( msg )
	    *msg = tr("Empty file name");

	return nullptr;
    }

    CBVSSeisTrcTranslator* tr = CBVSSeisTrcTranslator::instance();
    tr->set2D( is2d );
    tr->setForceUseCBVSInfo( forceusecbvsinf );
    if ( msg )
	*msg = uiString::emptyString();

    if ( !tr->initRead(new StreamConn(fnm,Conn::Read),
			infoonly ? Seis::PreScan : Seis::Prod) )
    {
	if ( msg )
	    *msg = tr->errMsg();

	deleteAndNullPtr( tr );
    }
    else if ( tr && tr->is2D() )
    {
	const Pos::GeomID gid( CBVSIOMgr::getFileNr(fnm) );
	if ( Survey::is2DGeom(gid) )
	    tr->setCurGeomID( gid );
    }

    return tr;
}


void CBVSSeisTrcTranslator::cleanUp()
{
    SeisTrcTranslator::cleanUp();

    donext_ =false;
    nrdone_ = 0;
    destroyVars( 0 );
}


void CBVSSeisTrcTranslator::destroyVars( int )
{
    deleteAndNullPtr( rdmgr_ );
    deleteAndNullPtr( wrmgr_ );
    deleteAndNullArrPtr( compsel_ );
}


bool CBVSSeisTrcTranslator::forRead() const
{
    return forread_;
}


void CBVSSeisTrcTranslator::setCoordPol( bool dowrite, bool intrailer )
{
    if ( !dowrite )
	coordpol_ = (int)CBVSIO::NotStored;
    else if ( intrailer )
	coordpol_ = (int)CBVSIO::InTrailer;
    else
	coordpol_ = (int)CBVSIO::InAux;
}


bool CBVSSeisTrcTranslator::is2D() const
{
    return auxinf_.is2D();
}


void CBVSSeisTrcTranslator::set2D( bool yn )
{
    setIs2D( yn );
    auxinf_.set2D( yn );
    if ( is2D() )
    {
	single_file_ = true;
	coordpol_ = (int)CBVSIO::InTrailer;
    }
}


void CBVSSeisTrcTranslator::setCurGeomID( Pos::GeomID gid )
{
    SeisTrcTranslator::setCurGeomID( gid );
    if ( Survey::isValidGeomID(curGeomID()) )
	auxinf_.trckey_.setGeomID( curGeomID() );
}


bool CBVSSeisTrcTranslator::getFileName( BufferString& fnm )
{
    if ( !conn_ )
    {
	errmsg_ = tr("Cannot open CBVS file");
	objstatus_ = IOObj::Status::ReadPermissionInvalid;
	return false;
    }

    PtrMan<IOObj> ioobj = IOM().get( conn_->linkedTo() );
    mDynamicCastGet(const IOStream*,iostrm,ioobj.ptr())
    if ( ioobj && !iostrm )
    {
	errmsg_ = tr("Object manager provides wrong type");
	objstatus_ = IOObj::Status::WrongObject;
	return false;
    }

    if ( !ioobj || iostrm->isMultiConn() )
    {
	mDynamicCastGet(StreamConn*,strmconn,conn_)
	if ( !strmconn )
	{
	    errmsg_ = tr("Wrong connection from Object Manager");
	    objstatus_ = IOObj::Status::WrongObject;
	    return false;
	}

	fnm = strmconn->fileName();
	return true;
    }

    // Catch the 'stdin' pretty name (currently "Std-IO")
    StreamProvider sp;
    fnm = iostrm->fullUserExpr(true);
    if ( fnm == sp.fileName() )
	fnm = StreamProvider::sStdIO();

    conn_->close();
    return true;
}


int CBVSSeisTrcTranslator::bytesOverheadPerTrace() const
{
    return rdmgr_ ? rdmgr_->bytesOverheadPerTrace()
		  : CBVSReader::defHeaderSize();
}


int CBVSSeisTrcTranslator::estimatedNrTraces() const
{
    return rdmgr_ ? rdmgr_->estimatedNrTraces()
		  : SeisTrcTranslator::estimatedNrTraces();
}


od_int64 CBVSSeisTrcTranslator::getFileSize( const IOObj* ) const
{
    if ( !rdmgr_ )
	return -1;

    const BufferString filenm = rdmgr_->baseFileName();
    if ( filenm.isEmpty() )
	return -1;

    if ( !File::isDirectory(filenm) && File::isEmpty(filenm) )
	return -1;

    od_int64 totalsz = 0;
    if ( File::isDirectory(filenm) )
    {
	const DirList dl( filenm.buf(), File::DirListType::FilesInDir );
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const FilePath filepath = dl.fullPath( idx );
	    const StringView ext = filepath.extension();
	    if ( ext != "cbvs" )
		continue;

	    totalsz += File::getFileSize( filepath.fullPath() );
	}
    }
    else
    {
	int nrfiles = 0;
	while ( true )
	{
	    const BufferString fullnm( CBVSIOMgr::getFileName(filenm,nrfiles) );
	    if ( !File::exists(fullnm) )
		break;

	    totalsz += File::getFileSize( fullnm );
	    nrfiles++;
	}
    }

    BufferStringSet filenames;
    SeisTrcTranslator::getAllFileNames( filenames );
    for ( const auto* nm : filenames )
	totalsz += File::getFileSize( nm->buf() );

    return totalsz;
}


void CBVSSeisTrcTranslator::getAllFileNames( BufferStringSet& filenames,
					     bool forui ) const
{
    if ( !rdmgr_ )
	return;

    const BufferString basefname = rdmgr_->baseFileName();
    if ( !basefname.isEmpty() )
	filenames.addIfNew( basefname );

    SeisTrcTranslator::getAllFileNames( filenames, forui );
}


void CBVSSeisTrcTranslator::implFileNames( const IOObj* obj,
					   BufferStringSet& filenames ) const
{
    if ( !obj )
	return;

    filenames.add( obj->fullUserExpr() );
}


bool CBVSSeisTrcTranslator::haveAux( const char* ext ) const
{
    if ( !rdmgr_ )
	return false;

    const StringView filenm = rdmgr_->baseFileName();
    if ( filenm.isEmpty() )
	return false;

    FilePath fp( filenm );
    fp.setExtension( ext );
    if ( !File::isDirectory(filenm) )
	return File::exists( fp.fullPath() );

    const DirList dl( filenm.buf(), File::DirListType::FilesInDir );
    return dl.isPresent( fp.fullPath() );
}


BufferString CBVSSeisTrcTranslator::getAuxFileName( const char* ext ) const
{
    if ( !rdmgr_ )
	return BufferString::empty();

    if ( !haveAux(ext) )
	return BufferString::empty();

    FilePath fp( rdmgr_->baseFileName() );
    fp.setExtension( ext );
    return fp.fullPath();
}


bool CBVSSeisTrcTranslator::initRead_()
{
    forread_ = true;
    BufferString fnm;
    if ( !getFileName(fnm) )
	return false;

    rdmgr_ = new CBVSReadMgr( fnm, nullptr, single_file_,
			read_mode == Seis::PreScan, forceusecbvsinfo_ );
    if ( rdmgr_->failed() )
    {
	errmsg_ = toUiString( rdmgr_->errMsg() );
	objstatus_ = rdmgr_->objStatus();
	deleteAndNullPtr( rdmgr_ );
	return false;
    }

    if ( is2D() )
	rdmgr_->setSingleLineMode( true );

    if ( !File::isDirectory(fnm) )
    {
	const int nrfiles = CBVSIOMgr::nrFiles( fnm );
	if ( nrfiles == 1 && !File::exists(fnm) )
	{
	    errmsg_ = tr("%1 does not exist").arg( fnm );
	    objstatus_ = IOObj::Status::FileNotPresent;
	    return false;
	}

	const FilePath basefp( fnm );
	BufferString filemask( "*", basefp.baseName().buf(), "^*.cbvs");
	const DirList dl( basefp.pathOnly().buf(),
			  File::DirListType::FilesInDir, filemask.buf());
	if ( dl.size() != nrfiles-1 )
	{
	    objstatus_ = IOObj::Status::FileNotPresent;
	    return false;
	}

	for ( int idx=0; idx<nrfiles; idx++ )
	{
	    const BufferString fullnm( CBVSIOMgr::getFileName(fnm,idx) );
	    if ( !File::exists(fullnm.buf()) )
	    {
		errmsg_ = tr( "Some Z-optimized slices are missing" );
		objstatus_ = IOObj::Status::FileDataCorrupt;
		return false;
	    }
	}
    }

    const int nrcomp = rdmgr_->nrComponents();
    const CBVSInfo& info = rdmgr_->info();
    insd_ = info.sd_;
    innrsamples_ = info.nrsamples_;
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	const BasicComponentInfo& cinf = *info.compinfo_[idx];
	addComp( cinf.datachar_, cinf.name(), cinf.datatype_ );
    }

    pinfo_.usrinfo_ = info.usertext_;
    pinfo_.stdinfo_ = info.stdtext_;
    pinfo_.nr_ = info.seqnr_;
    pinfo_.fullyrectandreg_ = info.geom_.fullyrectandreg_;
    pinfo_.inlrg_.start_ = info.geom_.start_.inl();
    pinfo_.inlrg_.stop_ = info.geom_.stop_.inl();
    pinfo_.inlrg_.step_ = abs(info.geom_.step_.inl());
    pinfo_.inlrg_.sort();
    pinfo_.crlrg_.start_ = info.geom_.start_.crl();
    pinfo_.crlrg_.stop_ = info.geom_.stop_.crl();
    pinfo_.crlrg_.step_ = abs(info.geom_.step_.crl());
    pinfo_.crlrg_.sort();
    if ( !pinfo_.fullyrectandreg_ )
	pinfo_.cubedata_ = &info.geom_.cubedata_;

    rdmgr_->getIsRev( pinfo_.inlrev_, pinfo_.crlrev_ );
    return true;
}


bool CBVSSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    if ( !trc.data().nrComponents() )
	return false;

    forread_ = false;

    for ( int idx=0; idx<trc.data().nrComponents(); idx++ )
    {
	DataCharacteristics dc(trc.data().getInterpreter(idx)->dataChar());
	addComp( dc, 0 );
	if ( preseldatatype_ )
	    tarcds_[idx]->datachar_ = DataCharacteristics(
			(DataCharacteristics::UserType)preseldatatype_ );
    }

    return true;
}


bool CBVSSeisTrcTranslator::commitSelections_()
{
    if ( forread_ && !rdmgr_ )
	return false;

    if ( forread_ && seldata_ && !seldata_->isAll() )
    {
	const Interval<int> inlrg = seldata_->inlRange();
	const Interval<int> crlrg = seldata_->crlRange();
	TrcKeyZSampling tkzs;
	if ( is2D() )
	{
	    tkzs.hsamp_.start_.inl() = rdmgr_->info().geom_.start_.inl();
	    tkzs.hsamp_.stop_.inl() = rdmgr_->info().geom_.start_.inl();
	    // CBVSInfo does not know about 2D
	}
	else
	{
	    tkzs.hsamp_.start_.inl() = inlrg.start_;
	    tkzs.hsamp_.stop_.inl() = inlrg.stop_;
	}

	tkzs.hsamp_.start_.crl() = crlrg.start_;
	tkzs.hsamp_.stop_.crl() = crlrg.stop_;
	tkzs.zsamp_.start_ = outsd_.start_;
	tkzs.zsamp_.step_ = outsd_.step_;
	tkzs.zsamp_.stop_ = outsd_.start_ + (outnrsamples_-1) * outsd_.step_;

	if ( rdmgr_->pruneReaders(tkzs) == 0 )
	{
	    errmsg_ = tr("Input contains no relevant data");
	    return false;
	}
    }

    delete [] compsel_;
    compsel_ = new bool [tarcds_.size()];
    for ( int idx=0; idx<tarcds_.size(); idx++ )
	compsel_[idx] = tarcds_[idx]->destidx >= 0;

    if ( !forread_ )
	return startWrite();

    if ( selRes(rdmgr_->binID()) )
	return toNext();

    return true;
}


bool CBVSSeisTrcTranslator::inactiveSelData() const
{
    return isEmpty( seldata_ );
}


int CBVSSeisTrcTranslator::selRes( const BinID& bid ) const
{
    if ( inactiveSelData() )
	return 0;

    // Table for 2D: can't select because inl/crl in file is not 'true'
    if ( is2D() && seldata_->type() == Seis::Table )
	return 0;

    return is2D() ? seldata_->selRes( curGeomID(), bid.trcNr() )
		  : seldata_->selRes( bid );
}


bool CBVSSeisTrcTranslator::toNext()
{
    if ( inactiveSelData() )
	return rdmgr_->toNext();

    const CBVSInfo& info = rdmgr_->info();
    if ( info.nrtrcsperposn_ > 1 )
    {
	if ( !rdmgr_->toNext() )
	    return false;
	else if ( !selRes(rdmgr_->binID()) )
	    return true;
    }

    BinID nextbid = rdmgr_->nextBinID();
    if ( nextbid.isUdf() )
	return false;

    if ( !selRes(nextbid) )
	return rdmgr_->toNext();

    // find next requested BinID
    while ( true )
    {
	while ( true )
	{
	    int res = selRes( nextbid );
	    if ( !res ) break;

	    if ( res%256 == 2 )
	    {
		if ( !info.geom_.moveToNextInline(nextbid) )
		    return false;
	    }
	    else if ( !info.geom_.moveToNextPos(nextbid) )
		return false;
	}

	if ( goTo(nextbid) )
	    break;
	else if ( !info.geom_.moveToNextPos(nextbid) )
	    return false;
    }

    return true;
}


bool CBVSSeisTrcTranslator::toStart()
{
    if ( rdmgr_ && rdmgr_->toStart() )
    {
	headerdonenew_ = donext_ = false;
	return true;
    }

    return false;
}


bool CBVSSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( rdmgr_ && rdmgr_->goTo(bid) )
    {
	headerdonenew_ = donext_ = false;
	return true;
    }

    return false;
}


bool CBVSSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !commitSelections_() )
	return false;

    if ( headerdonenew_ )
	return true;

    donext_ = donext_ || selRes( rdmgr_->binID() );

    if ( donext_ && !toNext() )
	return false;

    donext_ = true;

    if ( !rdmgr_->getAuxInfo(auxinf_) )
	return false;

    ti.getFrom( auxinf_ );
    if ( ti.is2D() && !forceusecbvsinfo_ )
    {
	float spnr = mUdf(float);
	if ( ti.trcKey().geometry().as2D()->getPosByTrcNr(ti.trcNr(),
							  ti.coord_,spnr)
	     && !mIsUdf(spnr) )
	    ti.refnr_ = spnr;
    }

    ti.sampling_.start_ = outsd_.start_;
    ti.sampling_.step_ = outsd_.step_;
    ti.seqnr_ = ++nrdone_;

    if ( ti.lineNr() == 0 && ti.trcNr() == 0 )
	ti.setPos( SI().transform(ti.coord_) );

    if ( is_2d )
    {
	if ( geomid_.isValid() )
	    ti.setGeomID( geomid_ );
    }

    return (headerdonenew_ = true);
}


bool CBVSSeisTrcTranslator::readDataPack( RegularSeisDataPack& rsdp,
					  TaskRunner* )
{
    const bool res = GetEnvVarYN("OD_ENABLE_TRANSLATOR_DATAPACK",false);
    if ( !res )
	return false;

    if ( !storbuf_ && !commitSelections() )
	return false;

    TrcKeyZSampling tkzs;
    tkzs.hsamp_.setInlRange( seldata_->inlRange() );
    tkzs.hsamp_.setCrlRange( seldata_->crlRange() );
    tkzs.zsamp_.setInterval( seldata_->zRange() );
    rsdp.setSampling( tkzs );
    rsdp.addComponent( "" );

    while ( true )
    {
	const BinID bid = rdmgr_->binID();
	const int inlidx = tkzs.hsamp_.lineIdx( bid.inl() );
	const int crlidx = tkzs.hsamp_.trcIdx( bid.crl() );
	const int zidx = 0;

	if ( !rdmgr_->fetch(*storbuf_,compsel_,&samprg_) )
	{
	    errmsg_ = toUiString( rdmgr_->errMsg() );
	    return false;
	}

	const float val = storbuf_->getValue( 0, 0 );
	rsdp.data(0).set( inlidx, crlidx, zidx, val );
	if ( !toNext() )
	    break;
    }

    return true;
}


bool CBVSSeisTrcTranslator::readData( TraceData* extbuf )
{
    if ( !storbuf_ && !commitSelections() )
	return false;

    TraceData& tdata = extbuf ? *extbuf : *storbuf_;
    if ( !rdmgr_->fetch(tdata,compsel_,&samprg_) )
    {
	errmsg_ = toUiString( rdmgr_->errMsg() );
	return false;
    }

    headerdonenew_ = false;
    return (datareaddone_ = true );
}


bool CBVSSeisTrcTranslator::skip( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
	if ( !rdmgr_->toNext() )
	    return false;

    donext_ = headerdonenew_ = false;
    return true;
}


Pos::IdxPair2Coord CBVSSeisTrcTranslator::getTransform() const
{
    if ( !rdmgr_ || !rdmgr_->nrReaders() )
	return SI().binID2Coord();

    return rdmgr_->info().geom_.b2c_;
}


bool CBVSSeisTrcTranslator::getGeometryInfo( PosInfo::CubeData& cd ) const
{
    if ( !rdmgr_ )
	return false;

    cd = rdmgr_->info().geom_.cubedata_;
    return true;
}


bool CBVSSeisTrcTranslator::startWrite()
{
    BufferString fnm;
    if ( !getFileName(fnm) )
	return false;

    CBVSInfo info;
    info.auxinfosel_.setAll( true );
    info.geom_.fullyrectandreg_ = false;
    info.geom_.b2c_ = SI().binID2Coord();
    info.stdtext_ = pinfo_.stdinfo_;
    info.usertext_ = pinfo_.usrinfo_;
    for ( int idx=0; idx<nrSelComps(); idx++ )
	info.compinfo_ += new BasicComponentInfo(*outcds_[idx]);
    info.sd_ = insd_;
    info.nrsamples_ = innrsamples_;

    wrmgr_ = new CBVSWriteMgr( fnm, info, &auxinf_, &brickspec_, single_file_,
				(CBVSIO::CoordPol)coordpol_ );
    if ( wrmgr_->failed() )
    {
	errmsg_ = toUiString( wrmgr_->errMsg() );
	return false;
    }

    if ( is2D() )
	wrmgr_->setForceTrailers( true );
    return true;
}


bool CBVSSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    if ( !wrmgr_ )
    {
	pErrMsg("initWrite not done or failed");
	return false;
    }

    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	const int icomp = selComp(iselc);
	for ( int isamp=samprg_.start_; isamp<=samprg_.stop_; isamp++ )
	{
	    //Parallel !!!
	    storbuf_->setValue(isamp-samprg_.start_,trc.get(isamp,icomp),iselc);
	}
    }

    trc.info().putTo( auxinf_ );
    if ( !wrmgr_->put(*storbuf_) )
    {
	errmsg_ = mToUiStringTodo(wrmgr_->errMsg());
	return false;
    }

    return true;
}


void CBVSSeisTrcTranslator::blockDumped( int nrtrcs )
{
    if ( nrtrcs > 1 && wrmgr_ )
	wrmgr_->ensureConsistent();
}


void CBVSSeisTrcTranslator::usePar( const IOPar& iopar )
{
    SeisTrcTranslator::usePar( iopar );
    DataCharacteristics::UserType usrtyp;
    if ( DataCharacteristics::getUserTypeFromPar(iopar,usrtyp) )
	preseldatatype_ = int( usrtyp );

    const BufferString res = iopar.find( sKeyOptDir() );
    if ( !res.isEmpty() )
    {
	const char* operstr = res.str();
	brickspec_.setStd( *operstr == 'H' );
	if ( *operstr == 'H' && *operstr && *(operstr +1) == '`' )
	{
	    FileMultiString fms( operstr + 2 );
	    const int sz = fms.size();
	    int tmp = fms.getIValue( 0 );
	    if ( tmp > 0 )
		brickspec_.nrsamplesperslab = tmp < 100000 ? tmp : 100000;

	    if ( sz > 1 )
	    {
		tmp = fms.getIValue( 1 );
		if ( tmp > 0 )
		    brickspec_.maxnrslabs = tmp;
	    }
	}
    }
}



static StreamProvider* getStrmProv( const IOObj* ioobj, const char* ext )
{
    FilePath fp( ioobj->fullUserExpr(true) );
    fp.setExtension( ext );
    StreamProvider* sp = new StreamProvider( fp.fullPath() );
    if ( !sp->exists(true) )
	deleteAndNullPtr( sp );

    return sp;
}


static void removeAuxFile( const IOObj* ioobj, const char* ext )
{
    PtrMan<StreamProvider> sp = getStrmProv( ioobj, ext );
    if ( sp )
	sp->remove(false);
}


static void renameAuxFile( const IOObj* ioobj, const char* newnm,
			   const char* ext )
{
    PtrMan<StreamProvider> sp = getStrmProv( ioobj, ext );
    if ( !sp )
	return;

    FilePath fpnew( newnm );
    fpnew.setExtension( ext );
    sp->rename( fpnew.fullPath() );
}


#define mImplStart(fn) \
    if ( !ioobj || ioobj->translator()!="CBVS" ) return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
    if ( iostrm->isMulti() ) \
	return const_cast<IOStream*>(iostrm)->fn; \
 \
    BufferString pathnm = iostrm->fileSpec().fullDirName(); \
    BufferString basenm = iostrm->fileSpec().fileName()

#define mImplLoopStart \
	StreamProvider sp( CBVSIOMgr::getFileName(basenm,nr) ); \
	sp.addPathIfNecessary( pathnm ); \
	if ( !sp.exists(true) ) \
	    return true;


bool CBVSSeisTrcTranslator::implRemove( const IOObj* ioobj, bool deep ) const
{
    if ( ioobj && implIsLink(ioobj) && !deep )
	return true;

    mImplStart( implRemove() );

    removeAuxFile( ioobj, "par" );
    removeAuxFile( ioobj, "proc" );

    bool rv = true;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.remove(false) )
	{
	    rv = nr ? true : false;
	    break;
	}
    }

    return rv;
}


bool CBVSSeisTrcTranslator::implRename( const IOObj* ioobj,
					const char* newnm ) const
{
    if ( implIsLink(ioobj) )
	return true;

    mImplStart( implRename(newnm) );

    renameAuxFile( ioobj, newnm, "par" );
    renameAuxFile( ioobj, newnm, "proc" );

    bool rv = true;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	StreamProvider spnew( CBVSIOMgr::getFileName(newnm,nr) );
	spnew.addPathIfNecessary( pathnm );
	if ( !sp.rename(spnew.fileName()) )
	{
	    rv = false;
	    break;
	}
    }

    return rv;
}


bool CBVSSeisTrcTranslator::implSetReadOnly( const IOObj* ioobj, bool yn ) const
{
    mImplStart( implSetReadOnly(yn) );

    bool rv = true;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.setReadOnly(yn) )
	{
	    rv = false;
	    break;
	}
    }

    return rv;
}


bool CBVSSeisTrcTranslator::implIsLink( const IOObj* ioobj ) const
{
    if ( !ioobj || !ioobj->implExists(true) )
	return false;

    FilePath cbvsfp( ioobj->mainFileName() );
    cbvsfp.makeCanonical();
    FilePath survfp( GetDataDir() );
    survfp.makeCanonical();
    return !cbvsfp.isSubDirOf( survfp );
}


bool CBVSSeisTrcTranslator::getConfirmRemoveMsg( const IOObj* ioobj,
				uiString& msg, uiString& canceltxt,
				uiString& deepremovetxt,
				uiString& shallowremovetxt ) const
{
    if ( !ioobj || !ioobj->implExists(true) )
	return false;

    if ( !implIsLink(ioobj) )
	return Translator::getConfirmRemoveMsg( ioobj, msg, canceltxt,
					deepremovetxt, shallowremovetxt );

    FilePath cbvsfp( ioobj->mainFileName() );
    cbvsfp.makeCanonical();
    msg = tr("This is a link to a CBVS file outside your current survey:\n"
	    "'%1'\nDo you want to delete the actual CBVS file?")
	    .arg(cbvsfp.fullPath());
    canceltxt = uiStrings::sCancel();
    deepremovetxt = tr("Delete CBVS file");
    shallowremovetxt = tr("Delete link only");
    return true;
}
