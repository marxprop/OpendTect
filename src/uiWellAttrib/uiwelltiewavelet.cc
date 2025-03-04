/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelltiewavelet.h"

#include "arrayndimpl.h"
#include "flatposdata.h"
#include "survinfo.h"
#include "wavelet.h"

#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiseiswvltattr.h"
#include "uitoolbutton.h"


#define mErrRet(msg,act) { uiMSG().error(msg); act; }

// WellTie::uiWaveletView

WellTie::uiWaveletView::uiWaveletView( uiParent* p, ObjectSet<Wavelet>& wvs )
    : uiGroup(p)
    , activeWvltChgd(this)
    , wvltset_(wvs)
{
    createWaveletFields( this );
    for ( int idx=0; idx<wvs.size(); idx++ )
    {
	uiwvlts_ += new uiWavelet( this, wvs[idx], idx==0 );
	uiwvlts_[idx]->attach( ensureBelow, activewvltfld_ );
	if ( idx ) uiwvlts_[idx]->attach( rightOf, uiwvlts_[idx-1] );
	mAttachCB( uiwvlts_[idx]->wvltChged, uiWaveletView::activeWvltChanged );
    }
}


WellTie::uiWaveletView::~uiWaveletView()
{
    detachAllNotifiers();
    deepErase( uiwvlts_ );
}


void WellTie::uiWaveletView::createWaveletFields( uiGroup* grp )
{
    grp->setHSpacing( 40 );

    uiString initwnm = tr("Initial");
    uiString estwnm = tr("Deterministic");

    uiLabel* wvltlbl = new uiLabel( this, tr("Set active Wavelet : "));
    activewvltfld_ = new uiGenInput( this, uiString::emptyString(),
				     BoolInpSpec(true,initwnm,estwnm));
    wvltlbl->attach( alignedAbove, activewvltfld_ );
    activewvltfld_->valueChanged.notify(
			 mCB(this, uiWaveletView, activeWvltChanged) );
    setVSpacing ( 0 );
}


void WellTie::uiWaveletView::redrawWavelets()
{
    for ( int idx=0; idx<uiwvlts_.size(); idx++ )
	uiwvlts_[idx]->drawWavelet();
}


void WellTie::uiWaveletView::activeWvltChanged( CallBacker* )
{
    const bool isinitactive = activewvltfld_->getBoolValue();
    uiwvlts_[0]->setAsActive( isinitactive );
    uiwvlts_[1]->setAsActive( !isinitactive );
    CBCapsule<bool> caps( isinitactive, this );
    activeWvltChgd.trigger( &caps );
}


void WellTie::uiWaveletView::setActiveWavelet( bool initial )
{
    if ( !activewvltfld_ )
	return;

    activewvltfld_->setValue( initial );
}


bool WellTie::uiWaveletView::isInitialWvltActive() const
{
    if ( !activewvltfld_ )
	return false;

    return activewvltfld_->getBoolValue();
}


// WellTie::uiWavelet

WellTie::uiWavelet::uiWavelet( uiParent* p, Wavelet* wvlt, bool isactive )
    : uiGroup(p)
    , wvltChged(this)
    , isactive_(isactive)
    , wvlt_(wvlt)
{
    viewer_ = new uiFlatViewer( this );

    wvltbuts_ += new uiToolButton( this, "info", uiStrings::sProperties(),
	    mCB(this,uiWavelet,dispProperties) );
    wvltbuts_[0]->attach( alignedBelow, viewer_ );

    wvltbuts_ += new uiToolButton( this, "phase", tr("Rotate phase"),
	    mCB(this,uiWavelet,rotatePhase) );
    wvltbuts_[1]->attach( rightOf, wvltbuts_[0] );

    wvltbuts_ += new uiToolButton( this, "wavelet_taper", tr("Taper Wavelet"),
	    mCB(this,uiWavelet,taper) );
    wvltbuts_[2]->attach( rightOf, wvltbuts_[1] );

    initWaveletViewer();
    drawWavelet();
    setVSpacing ( 0 );
}


WellTie::uiWavelet::~uiWavelet()
{
}


void WellTie::uiWavelet::initWaveletViewer()
{
    FlatView::Appearance& app = viewer_->appearance();
    app.annot_.setAxesAnnot( false );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.mappersetup_.cliprate_.set(0,0);
    app.ddpars_.wva_.refline_ = OD::Color::Black();
    app.ddpars_.wva_.mappersetup_.symmidval_ = 0;
    app.setDarkBG( false );
    app.annot_.x1_.hasannot_ = false;
    app.annot_.x2_.hasannot_ = false;
    viewer_->setInitialSize( uiSize(80,100) );
    viewer_->setStretch( 1, 2 );
}


void WellTie::uiWavelet::rotatePhase( CallBacker* )
{
    auto* orgwvlt = new Wavelet( *wvlt_ );
    uiSeisWvltRotDlg dlg( this, *wvlt_ );
    dlg.acting.notify( mCB(this,uiWavelet,wvltChanged) );
    if ( !dlg.go() )
    {
	*wvlt_ = *orgwvlt;
	wvltChanged( nullptr );
    }
    delete orgwvlt;
}


void WellTie::uiWavelet::taper( CallBacker* )
{
    auto* orgwvlt = new Wavelet( *wvlt_ );
    uiSeisWvltTaperDlg dlg( this, *wvlt_ );
    dlg.acting.notify( mCB(this,uiWavelet,wvltChanged) );
    if ( !dlg.go() )
    {
	*wvlt_ = *orgwvlt;
	wvltChanged( nullptr );
    }
    delete orgwvlt;
}


void WellTie::uiWavelet::wvltChanged( CallBacker* )
{
    drawWavelet();
    if ( isactive_ )
	wvltChged.trigger();
}


void WellTie::uiWavelet::dispProperties( CallBacker* )
{
    delete wvltpropdlg_;
    wvltpropdlg_ = new uiWaveletDispPropDlg( this, *wvlt_ );
    wvltpropdlg_ ->go();
}


void WellTie::uiWavelet::setAsActive( bool isactive )
{
    isactive_ = isactive;
}


void WellTie::uiWavelet::drawWavelet()
{
    if ( !wvlt_ )
	return;

    Wavelet wvlt( *wvlt_ );
    wvlt.normalize();
    const int wvltsz = wvlt.size();
    //TODO Update
    //const ZDomain::Info& zdomain = wvlt.zDomain();
    const ZDomain::Info& zdomain = SI().zDomainInfo();
    Array2D<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
    OD::memCopy( fva2d->getData(), wvlt.samples(), wvltsz * sizeof(float) );
    RefMan<FlatDataPack> dp = new FlatDataPack( "Wavelet", fva2d );
    dp->setName( wvlt.name() );
    fdp_ =  dp;

    const bool canupdate = viewer_->enableChange( false );
    viewer_->clearAllPacks();
    viewer_->setPack( FlatView::Viewer::WVA, dp.ptr(), false);
    viewer_->appearance().ddpars_.wva_.mappersetup_.setAutoScale( true );
    StepInterval<double> posns;
    posns.setFrom( wvlt.samplePositions() );
    posns.scale( zdomain.userFactor() );

    dp->posData().setRange( false, posns );
    dp->setZDomain( zdomain );
    viewer_->setViewToBoundingBox();
    viewer_->enableChange( canupdate );
    viewer_->handleChange( sCast(od_uint32,uiFlatViewer::All) );
}
