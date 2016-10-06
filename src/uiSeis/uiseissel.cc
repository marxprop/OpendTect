/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2001
________________________________________________________________________

-*/

#include "uiseissel.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjselwritetransl.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uistrings.h"

#include "zdomain.h"
#include "ctxtioobj.h"
#include "trckeyzsampling.h"
#include "dbdir.h"
#include "iostrm.h"
#include "dbman.h"
#include "iopar.h"
#include "keystrs.h"
#include "seis2dlineio.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "seistype.h"
#include "separstr.h"
#include "survinfo.h"
#include "seiscbvs.h"
#include "seispsioprov.h"


uiString uiSeisSelDlg::gtSelTxt( const uiSeisSel::Setup& setup, bool forread )
{
    if ( !setup.seltxt_.isEmpty() )
	return setup.seltxt_;

    uiString datatype = Seis::dataName(setup.geom_, setup.explprepost_ );

    return forread
	? uiStrings::phrInput( datatype )
	: uiStrings::phrOutput( datatype );
}


static IOObjContext adaptCtxt4Steering( const IOObjContext& ct,
				const uiSeisSel::Setup& su )
{
    IOObjContext ctxt( ct );
    if ( su.steerpol_ == uiSeisSel::Setup::NoSteering )
	ctxt.toselect_.dontallow_.set( sKey::Type(), sKey::Steering() );
    else if ( su.steerpol_ == uiSeisSel::Setup::OnlySteering )
    {
	ctxt.toselect_.require_.set( sKey::Type(), sKey::Steering() );
	ctxt.fixTranslator(
		Seis::is2D(su.geom_) ? CBVSSeisTrc2DTranslator::translKey()
				     : CBVSSeisTrcTranslator::translKey() );
    }

    return ctxt;
}


static CtxtIOObj adaptCtio4Steering( const CtxtIOObj& ct,
				const uiSeisSel::Setup& su )
{
    CtxtIOObj ctio( ct );
    ctio.ctxt_ = adaptCtxt4Steering( ctio.ctxt_, su );
    return ctio;
}


static uiIOObjSelDlg::Setup getSelDlgSU( const uiSeisSel::Setup& sssu )
{
    uiIOObjSelDlg::Setup sdsu;
    sdsu.allowsetsurvdefault( sssu.allowsetsurvdefault_ );
    sdsu.withwriteopts( sssu.withwriteopts_ );
    return sdsu;
}


uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const uiSeisSel::Setup& sssu )
    : uiIOObjSelDlg(p,getSelDlgSU(sssu),adaptCtio4Steering(c,sssu))
    , compfld_(0)
    , steerpol_(sssu.steerpol_)
    , zdomainkey_(sssu.zdomkey_)
{
    setSurveyDefaultSubsel( sssu.survdefsubsel_ );

    const bool is2d = Seis::is2D( sssu.geom_ );
    const bool isps = Seis::isPS( sssu.geom_ );

    if ( is2d )
    {
	if ( selgrp_->getTopGroup() )
	    selgrp_->getTopGroup()->display( true, true );
	if ( selgrp_->getNameField() )
	    selgrp_->getNameField()->display( true, true );
	if ( selgrp_->getListField() )
	    selgrp_->getListField()->display( true, true );

	if ( c.ioobj_)
	{
	    DBKeySet selmids;
	    selmids += c.ioobj_->key();
	    selgrp_->setChosen( selmids );
	}
    }

    uiString titletxt( tr("Select %1") );
    if ( !sssu.seltxt_.isEmpty() )
	titletxt = titletxt.arg( sssu.seltxt_ );
    else
	titletxt = titletxt.arg( isps
		? tr("Data Store")
		: (is2d ? tr("Dataset") : uiStrings::sVolume()) );
    setTitleText( titletxt );

    uiGroup* topgrp = selgrp_->getTopGroup();
    selgrp_->getListField()->selectionChanged.notify(
					    mCB(this,uiSeisSelDlg,entrySel) );

    if ( !selgrp_->getCtxtIOObj().ctxt_.forread_ && Seis::is2D(sssu.geom_) )
	selgrp_->setConfirmOverwrite( false );

    if ( selgrp_->getCtxtIOObj().ctxt_.forread_ && sssu.selectcomp_ )
    {
	compfld_ = new uiLabeledComboBox( selgrp_, uiStrings::sComponent(),
					  "Compfld" );
	compfld_->attach( alignedBelow, topgrp );
	entrySel(0);
    }
}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    if ( !compfld_ )
	return;

    const IOObjContext& ctxt = selgrp_->getCtxtIOObj().ctxt_;
    if ( !ctxt.forread_ )
	return;

    const IOObj* ioobj = ioObj(); // do NOT call this function when for write
    if ( !ioobj )
	return;

    compfld_->box()->setCurrentItem(0);
    SeisTrcReader rdr( ioobj );
    if ( !rdr.prepareWork(Seis::PreScan) ) return;
    SeisTrcTranslator* transl = rdr.seisTranslator();
    if ( !transl ) return;
    BufferStringSet compnms;
    transl->getComponentNames( compnms );
    compfld_->box()->setEmpty();
    compfld_->box()->addItems( compnms );
    compfld_->display( transl->componentInfo().size()>1 );
}


const char* uiSeisSelDlg::getDataType()
{
    if ( steerpol_ )
	return steerpol_ == uiSeisSel::Setup::NoSteering
			  ? 0 : sKey::Steering().str();
    const IOObj* ioobj = ioObj();
    if ( !ioobj ) return 0;
    const char* res = ioobj->pars().find( sKey::Type() );
    return res;
}


void uiSeisSelDlg::fillPar( IOPar& iopar ) const
{
    uiIOObjSelDlg::fillPar( iopar );
    const IOObj* ioobj = ioObj();
    if ( !ioobj )
	return;

    SeisIOObjInfo oinf( *ioobj );
    if ( compfld_ )
    {
	BufferStringSet compnms;
	getComponentNames( compnms );
	const int compnr = compnms.indexOf( compfld_->box()->text() );
	if ( compnr>=0 )
	    iopar.set( sKey::Component(), compnr );
    }
}


void uiSeisSelDlg::usePar( const IOPar& iopar )
{
    uiIOObjSelDlg::usePar( iopar );

    if ( compfld_ )
	entrySel(0);

    if ( compfld_ )
    {
	int selcompnr = mUdf(int);
	if ( iopar.get( sKey::Component(), selcompnr ) && !mIsUdf( selcompnr) )
	{
	    BufferStringSet compnms;
	    getComponentNames( compnms );
	    if ( selcompnr >= compnms.size() ) return;
	    compfld_->box()->setText( compnms.get(selcompnr).buf() );
	}
    }
}


void uiSeisSelDlg::getComponentNames( BufferStringSet& compnms ) const
{
    compnms.erase();
    const IOObj* ioobj = ioObj();
    if ( !ioobj ) return;
    SeisTrcReader rdr( ioobj );
    if ( !rdr.prepareWork(Seis::PreScan) ) return;
    SeisTrcTranslator* transl = rdr.seisTranslator();
    if ( !transl ) return;
    transl->getComponentNames( compnms );
}


static IOObjContext getIOObjCtxt( const IOObjContext& c,
				  const uiSeisSel::Setup& s )
{
    return adaptCtxt4Steering( c, s );
}


uiSeisSel::uiSeisSel( uiParent* p, const IOObjContext& ctxt,
		      const uiSeisSel::Setup& su )
	: uiIOObjSel(p,getIOObjCtxt(ctxt,su),mkSetup(su,ctxt.forread_))
	, seissetup_(mkSetup(su,ctxt.forread_))
	, othdombox_(0)
        , compnr_(0)
{
    workctio_.ctxt_ = inctio_.ctxt_;
    if ( !ctxt.forread_ && Seis::is2D(seissetup_.geom_) )
	seissetup_.confirmoverwr_ = setup_.confirmoverwr_ = false;

    mkOthDomBox();
}


void uiSeisSel::mkOthDomBox()
{
    if ( !inctio_.ctxt_.forread_ && seissetup_.enabotherdomain_ )
    {
	othdombox_ = new uiCheckBox( this, SI().zIsTime() ? uiStrings::sDepth()
                                                          : uiStrings::sTime());
	othdombox_->attach( rightOf, endObj(false) );
    }
}


uiSeisSel::~uiSeisSel()
{
}


uiSeisSel::Setup uiSeisSel::mkSetup( const uiSeisSel::Setup& su, bool forread )
{
    uiSeisSel::Setup ret( su );
    ret.seltxt_ = uiSeisSelDlg::gtSelTxt( su, forread );
    ret.filldef( su.allowsetdefault_ );
    return ret;
}


const char* uiSeisSel::getDefaultKey( Seis::GeomType gt ) const
{
    const bool is2d = Seis::is2D( gt );
    return IOPar::compKey( sKey::Default(),
	is2d ? SeisTrcTranslatorGroup::sKeyDefault2D()
	     : SeisTrcTranslatorGroup::sKeyDefault3D() );
}


void uiSeisSel::fillDefault()
{
    if ( !setup_.filldef_ || workctio_.ioobj_ || !workctio_.ctxt_.forread_ )
	return;

    workctio_.destroyAll();
    if ( Seis::isPS(seissetup_.geom_) )
	workctio_.fillDefault();
    else
	workctio_.fillDefaultWithKey( getDefaultKey(seissetup_.geom_) );
}


IOObjContext uiSeisSel::ioContext( Seis::GeomType gt, bool forread )
{
    PtrMan<IOObjContext> newctxt = Seis::getIOObjContext( gt, forread );
    return IOObjContext( *newctxt );
}


void uiSeisSel::newSelection( uiIOObjRetDlg* dlg )
{
    ((uiSeisSelDlg*)dlg)->fillPar( dlgiopar_ );
    if ( seissetup_.selectcomp_ && !dlgiopar_.get(sKey::Component(), compnr_) )
	setCompNr( compnr_ );
}


const char* uiSeisSel::userNameFromKey( const char* txt ) const
{
    if ( !txt || !*txt ) return "";

    StringPair strpair( txt );
    curusrnm_ = uiIOObjSel::userNameFromKey( strpair.first() );
    return curusrnm_.buf();
}


void uiSeisSel::setCompNr( int nr )
{
    compnr_ = nr;
    if ( mIsUdf(compnr_) )
	dlgiopar_.removeWithKey( sKey::Component() );
    else
	dlgiopar_.set( sKey::Component(), nr );
    updateInput();
}


const char* uiSeisSel::compNameFromKey( const char* txt ) const
{
    if ( !txt || !*txt ) return "";

    StringPair strpair( txt );
    return uiIOObjSel::userNameFromKey( strpair.second() );
}


bool uiSeisSel::existingTyped() const
{
    bool containscompnm = false;
    const char* ptr = "";
    ptr = firstOcc( getInput(), '|' );
    if ( ptr )
	containscompnm = true;
    return (!is2D() && !containscompnm) || isPS() ? uiIOObjSel::existingTyped()
	 : existingUsrName( StringPair(getInput()).first() );
}


bool uiSeisSel::fillPar( IOPar& iop ) const
{
    iop.merge( dlgiopar_ );
    return uiIOObjSel::fillPar( iop );
}


void uiSeisSel::usePar( const IOPar& iop )
{
    uiIOObjSel::usePar( iop );
    dlgiopar_.merge( iop );

    if ( seissetup_.selectcomp_ && !iop.get(sKey::Component(), compnr_) )
	compnr_ = 0;
}


void uiSeisSel::updateInput()
{
    BufferString ioobjkey;
    if ( workctio_.ioobj_ )
	ioobjkey = workctio_.ioobj_->key();

    if ( !ioobjkey.isEmpty() )
	uiIOSelect::setInput( ioobjkey );

    if ( seissetup_.selectcomp_ && !mIsUdf( compnr_ ) )
    {
	SeisTrcReader rdr( workctio_.ioobj_ );
	if ( !rdr.prepareWork(Seis::PreScan) ) return;
	SeisTrcTranslator* transl = rdr.seisTranslator();
	if ( !transl ) return;
	BufferStringSet compnms;
	transl->getComponentNames( compnms );
	if ( compnr_ >= compnms.size() || compnms.size()<2 ) return;

	BufferString text = userNameFromKey( ioobjkey );
	text += "|";
	text += compnms.get( compnr_ );
	uiIOSelect::setInputText( text.buf() );
    }
}


void uiSeisSel::commitSucceeded()
{
    if ( !othdombox_ || !othdombox_->isChecked() ) return;

    const ZDomain::Def* def = SI().zIsTime() ? &ZDomain::Depth()
					     : &ZDomain::Time();
    def->set( dlgiopar_ );
    if ( inctio_.ioobj_ )
    {
	def->set( inctio_.ioobj_->pars() );
	DBM().setEntry( *inctio_.ioobj_ );
    }
}


void uiSeisSel::processInput()
{
    obtainIOObj();
    if ( !workctio_.ioobj_ && !workctio_.ctxt_.forread_ )
	return;

    uiIOObjSel::fillPar( dlgiopar_ );
    updateInput();
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, workctio_, seissetup_ );
    dlg->usePar( dlgiopar_ );
    uiIOObjSelGrp* selgrp = dlg->selGrp();
    if ( selgrp )
    {
	selgrp->setConfirmOverwrite( false );
	if ( wrtrselfld_ )
	    selgrp->setDefTranslator( wrtrselfld_->selectedTranslator() );
    }

    return dlg;
}

// uiSteerCubeSel

static uiSeisSel::Setup mkSeisSelSetupForSteering( bool is2d, bool forread,
						   const uiString& txt )
{
    uiSeisSel::Setup sssu( is2d, false );
    sssu.wantSteering().seltxt( txt );
    sssu.withwriteopts( false );
    return sssu;
}


uiSteerCubeSel::uiSteerCubeSel( uiParent* p, bool is2d, bool forread,
				const uiString& txt )
	: uiSeisSel(p,uiSeisSel::ioContext(is2d?Seis::Line:Seis::Vol,forread),
		    mkSeisSelSetupForSteering(is2d,forread,txt))
{
}


const char* uiSteerCubeSel::getDefaultKey( Seis::GeomType gt ) const
{
    BufferString defkey = uiSeisSel::getDefaultKey( gt );
    return IOPar::compKey( defkey, sKey::Steering() );
}
