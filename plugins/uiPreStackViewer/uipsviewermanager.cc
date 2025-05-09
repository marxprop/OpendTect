/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipsviewermanager.h"

#include "bufstringset.h"
#include "ioman.h"
#include "ioobj.h"
#include "prestackprocessor.h"
#include "settings.h"
#include "survinfo.h"
#include "uidlggroup.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uipsviewershapetab.h"
#include "uipsviewerposdlg.h"
#include "uipsviewer2dposdlg.h"
#include "uipsviewersettingdlg.h"
#include "uipsviewer2dmainwin.h"
#include "uiseispartserv.h"
#include "ui3dviewer.h"
#include "uivispartserv.h"
#include "visflatviewer.h"
#include "visplanedatadisplay.h"
#include "visseis2ddisplay.h"
#include "vistransform.h"
#include "uiamplspectrum.h"


namespace PreStackView
{

uiViewer3DMgr::uiViewer3DMgr()
    : selectpsdatamenuitem_( tr("Display Prestack Data") )
    , positionmenuitem_( m3Dots(tr("Show position window")) )
    , proptymenuitem_( m3Dots(uiStrings::sProperties()) )
    , resolutionmenuitem_( tr("Resolution") )
    , viewermenuitem_( m3Dots(tr("View in 2D panel")) )
    , amplspectrumitem_( m3Dots(tr("Amplitude spectrum")) )
    , removemenuitem_( uiStrings::sRemove() )
    , visserv_( ODMainWin()->applMgr().visServer() )
{
    positionmenuitem_.iconfnm = "orientation64";
    proptymenuitem_.iconfnm = "tools";
    amplspectrumitem_.iconfnm = "amplspectrum";
    viewermenuitem_.iconfnm = "vd";
    removemenuitem_.iconfnm = "remove";

    posdialogs_.setNullAllowed();
    settingdlgs_.setNullAllowed();
    mAttachCB( visserv_->removeAllNotifier(), uiViewer3DMgr::removeAllCB );
    mAttachCB( visserv_->objectRemoved, uiViewer3DMgr::sceneChangeCB );

    mAttachCB( IOM().surveyToBeChanged, uiViewer3DMgr::surveyToBeChangedCB );
    mAttachCB( ODMainWin()->sessionSave, uiViewer3DMgr::sessionSaveCB );
    mAttachCB( ODMainWin()->sessionRestore, uiViewer3DMgr::sessionRestoreCB );

    RefMan<MenuHandler> menuhandler = visserv_->getMenuHandler();
    mAttachCB( menuhandler->createnotifier, uiViewer3DMgr::createMenuCB );
    mAttachCB( menuhandler->handlenotifier, uiViewer3DMgr::handleMenuCB );
}


uiViewer3DMgr::~uiViewer3DMgr()
{
    detachAllNotifiers();
    removeAllCB( nullptr );
}


void uiViewer3DMgr::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet( uiMenuHandler*, menu, cb );

    RefMan<visBase::DataObject> dataobj =
		visserv_->getObject( VisID(menu->menuID()) );

    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );
    mDynamicCastGet( visSurvey::Seis2DDisplay*, s2d, dataobj.ptr() );
    if ( (pdd && pdd->getOrientation()!=OD::SliceType::Z) || s2d )
    {
	uiSeisPartServer* seisserv = ODMainWin()->applMgr().seisServer();

	BufferStringSet gnms; seisserv->getStoredGathersList(!s2d,gnms);
	if ( gnms.size() )
	{
	    selectpsdatamenuitem_.removeItems();
	    selectpsdatamenuitem_.createItems(gnms);

	    mAddMenuItem( menu, &selectpsdatamenuitem_, true, false );
	}
	else
	    mResetMenuItem( &selectpsdatamenuitem_ );
    }
    else
	mResetMenuItem( &selectpsdatamenuitem_ );


    mDynamicCastGet( visSurvey::PreStackDisplay*, psv, dataobj.ptr() );
    resolutionmenuitem_.id = -1;
    resolutionmenuitem_.removeItems();

    if ( psv && psv->flatViewer() )
    {
	const int nrres = psv->flatViewer()->nrResolutions();
	BufferStringSet resolutions;
	for ( int idx=0; idx<nrres; idx++ )
	    resolutions.add( psv->flatViewer()->getResolutionName(idx) );

	resolutionmenuitem_.createItems( resolutions );
	for ( int idx=0; idx<resolutionmenuitem_.nrItems(); idx++ )
	    resolutionmenuitem_.getItem(idx)->checkable = true;

	resolutionmenuitem_.getItem(
		psv->flatViewer()->getResolution() )->checked = true;
    }



    viewermenuitem_.removeItems();

    const int idxof = psv ? viewers3d_.indexOf(psv) : -1;
    if ( idxof < 0  )
    {
	mResetMenuItem( &proptymenuitem_ );
	mResetMenuItem( &resolutionmenuitem_ );
	mResetMenuItem( &viewermenuitem_ );
	mResetMenuItem( &amplspectrumitem_ );
	mResetMenuItem( &positionmenuitem_ );
	mResetMenuItem( &removemenuitem_ );
    }
    else
    {
	mAddMenuItem( menu, &proptymenuitem_, true, false );
	mAddMenuItem( menu, &resolutionmenuitem_, true, false );
	mAddMenuItem( menu, &viewermenuitem_, true, false );
	mAddMenuItem( menu, &amplspectrumitem_, true, false );
	if ( !posdialogs_[idxof] || posdialogs_[idxof]->isHidden() )
	    mAddMenuItem( menu, &positionmenuitem_, true, false )
	else
	    mResetMenuItem( &positionmenuitem_ )

	mAddMenuItem( menu, &removemenuitem_, true, false );
    }
}


#define mComma ,
static void setDlgPos( uiMainWin* mw, int idx )
{
    if ( !mw ) return;
    mDefineStaticLocalObject( uiMainWin::PopupArea, puas, [] =
	{ uiMainWin::BottomRight mComma uiMainWin::BottomLeft mComma
	     uiMainWin::TopRight mComma uiMainWin::TopLeft } );
    mw->setPopupArea( puas[ idx % 4 ] );
}


void uiViewer3DMgr::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);

    if ( menu->isHandled() )
	return;

    const SceneID sceneid = getSceneID( VisID(menu->menuID()) );
    if ( !sceneid.isValid() )
	return;

    const int mnuidx = selectpsdatamenuitem_.itemIndex( mnuid );
    RefMan<visBase::DataObject> dataobj =
			visserv_->getObject( VisID(menu->menuID()) );
    mDynamicCastGet(visSurvey::PreStackDisplay*,psv,dataobj.ptr())
    if ( mnuidx < 0 && !psv )
	return;

    if ( mnuidx>=0 )
    {
	menu->setIsHandled( true );
	if ( !add3DViewer(menu,sceneid,mnuidx) )
	    return;
    }
    else if ( mnuid==removemenuitem_.id )
    {
	menu->setIsHandled( true );
	visserv_->removeObject( psv, sceneid );
	const int idx = viewers3d_.indexOf( psv );
	delete posdialogs_.removeSingle( idx );
	viewers3d_.removeSingle( idx );
	delete settingdlgs_.removeSingle( idx );
    }
    else if ( mnuid==proptymenuitem_.id )
    {
	menu->setIsHandled( true );
	const int sdidx = viewers3d_.indexOf( psv );
	uiViewer3DSettingDlg* dlg = settingdlgs_[sdidx];
	if ( !dlg )
	{
	    dlg = new uiViewer3DSettingDlg( menu->getParent(),*psv, *this );
	    settingdlgs_.replace( sdidx, dlg );
	}

	dlg->go();
    }
    else if ( mnuid==positionmenuitem_.id )
    {
	menu->setIsHandled( true );
	const int idx = viewers3d_.indexOf( psv );
	if ( idx >= 0 )
	{
	    uiViewer3DPositionDlg* dlg = posdialogs_[idx];
	    if ( !dlg )
		dlg = mkNewPosDialog( menu, *psv );
	    if ( dlg )
		dlg->show();
	}
    }
    else if ( resolutionmenuitem_.id!=-1 &&
	      resolutionmenuitem_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled( true );
	if ( psv->flatViewer() )
	{
	    psv->flatViewer()->setResolution(
					resolutionmenuitem_.itemIndex(mnuid) );
	}
    }
    else if ( mnuid == viewermenuitem_.id )
    {
	menu->setIsHandled( true );
	uiMainWin* viewwin = createMultiGather2DViewer( *psv );
	if ( viewwin )
	{
	    viewers2d_ += viewwin;
	    viewwin->show();
	}
    }
    else if ( mnuid==amplspectrumitem_.id )
    {
	menu->setIsHandled( true );
	auto* asd = new uiAmplSpectrum( menu->getParent() );
	asd->setDeleteOnClose( true );
	ConstRefMan<FlatDataPack> gatherdp = psv->getFlatDataPack();
	if ( !gatherdp )
	    return;

	asd->setDataPack( *gatherdp.ptr() );
	const uiString pos = psv->is3DSeis()
	    ? toUiString( "%1/%2").arg(psv->getPosition().inl())
				  .arg( psv->getPosition().crl() )
	    : toUiString( psv->getPosition().crl() );

	const uiString capt = tr("Amplitude spectrum for %1 at %2")
	    .arg( psv->getObjectName() )
	    .arg( pos );
	asd->setCaption( capt );
	asd->show();
    }
}


SceneID uiViewer3DMgr::getSceneID( const VisID& visid ) const
{
    TypeSet<SceneID> sceneids;
    visserv_->getSceneIds( sceneids );
    for ( const auto& sceneid : sceneids )
    {
	TypeSet<VisID> scenechildren;
	visserv_->getSceneChildIds( sceneid, scenechildren );
	if ( scenechildren.isPresent(visid) )
	    return sceneid;
    }

    return SceneID::udf();
}


#define mErrReturn(msg) { uiMSG().error(msg); return false; }

bool uiViewer3DMgr::add3DViewer( const uiMenuHandler* menu,
				 const SceneID& sceneid, int mnuidx )
{
    if ( !menu )
	return false;

    IOM().to( IOObjContext::Seis );
    PtrMan<IOObj> ioobj = IOM().getLocal(
	    selectpsdatamenuitem_.getItem(mnuidx)->text.getFullString(), 0 );
    if ( !ioobj )
	mErrReturn( tr("No object selected") )

    RefMan<visBase::DataObject> dataobj =
	visserv_->getObject( VisID(menu->menuID()) );

    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );
    mDynamicCastGet( visSurvey::Seis2DDisplay*, s2d, dataobj.ptr() );
    if ( !pdd && !s2d )
	mErrReturn( tr("Display panel is not set.") )

    RefMan<visSurvey::PreStackDisplay> viewer = new visSurvey::PreStackDisplay;
    viewer->setMultiID( ioobj->key() );
    visserv_->addObject( viewer.ptr(), sceneid, true );

    const Coord3 pickedpos = menu->getPickedPos();

    //set viewer position
    bool settingok = true;
    if ( pdd )
    {
	viewer->setSectionDisplay( pdd );
	BinID bid;
	if (  menu->getMenuType() != uiMenuHandler::fromScene() )
	{
	    TrcKeySampling hrg = pdd->getTrcKeyZSampling().hsamp_;
	    bid = SI().transform((SI().transform(hrg.start_)
				 +SI().transform(hrg.stop_))/2);
	}
	else bid = SI().transform( pickedpos );

	settingok = viewer->setPosition( TrcKey(bid) );
    }
    else if ( s2d )
    {
	int trcnr;
	if ( menu->getMenuType() != uiMenuHandler::fromScene() )
	    trcnr = s2d->getTraceNrRange().center();
	else
	    trcnr = s2d->getNearestTraceNr( pickedpos );

	settingok = viewer->setSeis2DDisplay( s2d, trcnr );
    }

    if ( !settingok )
    {
	visserv_->removeObject( viewer.ptr(), sceneid );
	return false;
    }

    const int res = pdd ? pdd->getResolution() : s2d->getResolution();
    if ( viewer->flatViewer() )
	viewer->flatViewer()->setResolution( res );

    //set viewer angle.
    const ui3DViewer*  sovwr = ODMainWin()->sceneMgr().get3DViewer( sceneid );
    Coord3 campos = sovwr->getCameraPosition();
	viewer->getScene()->getUTM2DisplayTransform()->transformBack( campos );
    const BinID dir0 = SI().transform(campos)-SI().transform(pickedpos);
    const Coord dir( dir0.inl(), dir0.crl() );
    viewer->displaysOnPositiveSide( viewer->getBaseDirection().dot(dir)>0 );

    //Read defaults
    const Settings& settings = Settings::fetch(uiViewer3DMgr::sSettings3DKey());
    bool autoview;
    if ( settings.getYN(visSurvey::PreStackDisplay::sKeyAutoWidth(), autoview) )
	viewer->displaysAutoWidth( autoview );

    float factor;
    if ( settings.get( visSurvey::PreStackDisplay::sKeyFactor(), factor ) )
	viewer->setFactor( factor );

    float width;
    if ( settings.get( visSurvey::PreStackDisplay::sKeyWidth(), width ) )
	viewer->setWidth( width );

    IOPar* flatviewpar = settings.subselect( sKeyFlatviewPars() );
    bool showx1 = viewer->flatViewer()->appearance().annot_.x1_.showgridlines_;
    bool showx2 = viewer->flatViewer()->appearance().annot_.x2_.showgridlines_;
    if ( flatviewpar )
    {
	viewer->flatViewer()->appearance().ddpars_.usePar( *flatviewpar );
	flatviewpar->getYN( IOPar::compKey( FlatView::Annotation::sKeyAxes(),
		    FlatView::Annotation::sKeyShwGridLines()), showx1, showx2 );
	viewer->flatViewer()->appearance().annot_.x1_.showgridlines_ = showx1;
	viewer->flatViewer()->appearance().annot_.x2_.showgridlines_ = showx2;
    }

    viewer->flatViewer()->handleChange( FlatView::Viewer::DisplayPars );
    viewer->flatViewer()->turnOnGridLines( showx1, showx2 );

    if ( viewer->getScene() )
	viewer->getScene()->change.notifyIfNotNotified(
		mCB( this, uiViewer3DMgr, sceneChangeCB ) );

    viewers3d_ += viewer.ptr();
    posdialogs_ += nullptr;
    mkNewPosDialog( menu, *viewer );
    settingdlgs_ += nullptr;
    return true;
}


uiViewer3DPositionDlg*
    uiViewer3DMgr::mkNewPosDialog( const uiMenuHandler* menu,
				   visSurvey::PreStackDisplay& vwr )
{
    mDeclareAndTryAlloc( uiViewer3DPositionDlg*, dlg,
	    uiViewer3DPositionDlg( menu->getParent(), vwr ) );
    if ( dlg )
    {
	const int newidx = posdialogs_.size() - 1;
	setDlgPos( dlg, newidx );
	posdialogs_.replace( newidx, dlg );
	dlg->show();
    }

    return dlg;
}


#define mErrRes(msg) { uiMSG().error(msg); return nullptr; }

uiMainWin* uiViewer3DMgr::create2DViewer( const uiString& title,
					  const PreStack::Gather& gather )
{
    auto* viewwin = new uiFlatViewMainWin( ODMainWin(),
					   uiFlatViewMainWin::Setup(title) );
    viewwin->setWinTitle( title );
    viewwin->setDarkBG( false );

    uiFlatViewer& vwr = viewwin->viewer();
    vwr.appearance().annot_.setAxesAnnot( true );
    vwr.appearance().setGeoDefaults( true );
    vwr.appearance().ddpars_.show( false, true );
    vwr.appearance().ddpars_.wva_.overlap_ = 1;

    vwr.setPack( FlatView::Viewer::VD, const_cast<PreStack::Gather*>(&gather) );
    int pw = 400 + 5 * gather.data().getSize( PreStack::Gather::offsetDim() );
    if ( pw > 800 )
	pw = 800;

    vwr.setInitialSize( uiSize(pw,600) );
    viewwin->addControl( new uiFlatViewStdControl( vwr,
					     uiFlatViewStdControl::Setup() ) );
    mAttachCB( viewwin->windowClosed, uiViewer3DMgr::viewer2DClosedCB );
    return viewwin->dockParent();
}


uiMainWin* uiViewer3DMgr::createMultiGather2DViewer(
				    const visSurvey::PreStackDisplay& psv )
{
    const MultiID mid = psv.getMultiID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
       return nullptr;

    const bool is2d = !psv.is3DSeis();
    BufferString title = "Gathers from [";
    if ( is2d )
	title += psv.lineName();
    else
	title += ioobj->name();

    title += "]";

    auto* viewwin = new uiStoredViewer2DMainWin( ODMainWin(), title, is2d );
    const StepInterval<int>& trcrg = psv.getTraceRange( psv.getBinID() );
    const TrcKey tk = psv.getTrcKey();
    viewwin->init( mid, tk,  is2d ? true : psv.isOrientationInline(), trcrg );
    viewwin->setDarkBG( false );
    viewwin->setAppearance( psv.flatViewer()->appearance() );
    mAttachCB( viewwin->seldatacalled_, uiViewer3DMgr::viewer2DSelDataCB );
    mAttachCB( viewwin->windowClosed, uiViewer3DMgr::viewer2DClosedCB );
    return viewwin->dockParent();
}


const uiFlatViewer* uiViewer3DMgr::getViewer( int idx ) const
{
    if ( !viewers2d_.validIdx(idx) )
	return nullptr;

    mDynamicCastGet(const uiFlatViewWin*,uifvw,viewers2d_.get(idx));
    return uifvw ? &uifvw->viewer() : nullptr;
}


void uiViewer3DMgr::viewer2DSelDataCB( CallBacker* cb )
{
    mDynamicCastGet( uiStoredViewer2DMainWin*, win, cb )
    if ( !win )
	{ pErrMsg( "Can not find viewer" ); return; }

    uiSeisPartServer* seisserv = ODMainWin()->applMgr().seisServer();
    BufferStringSet selgnms, allgnms; TypeSet<MultiID> selids;
    win->getIDs( selids );
    for( int idx=0; idx<selids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( selids[idx] );
	if ( ioobj )
	    selgnms.addIfNew( ioobj->name() );
    }

    seisserv->getStoredGathersList(!win->is2D(),allgnms);
    if ( allgnms.isEmpty() )
	return;

    for ( int idx=0; idx<selgnms.size(); idx++ )
    {
	if ( allgnms.isPresent( selgnms.get( idx ).buf() ) )
	    allgnms.removeSingle(allgnms.indexOf( selgnms.get( idx ).buf() ) );
    }

    selids.erase();
    uiViewer2DSelDataDlg dlg( win, allgnms, selgnms );
    if ( dlg.go() )
    {
	for( int idx=0; idx<selgnms.size(); idx++ )
	{
	    IOObj* ioobj = IOM().getLocal( selgnms[idx]->buf(), 0 );
	    if ( ioobj )
		selids += ioobj->key();
	    else
	    {
		uiString msg = tr("Can not find %1").arg(selgnms[idx]->buf());
		uiMSG().error( msg );
	    }
	}
	if ( selids.isEmpty() )
	    { uiMSG().error(tr("No data found")); return; }

	win->setIDs( selids );
    }
}


void uiViewer3DMgr::viewer2DClosedCB( CallBacker* cb )
{
    mDynamicCastGet(uiViewer2DMainWin*,uivwr2dmw,cb)
    mDynamicCastGet(uiFlatViewMainWin*,uifvmw,cb)
    uiMainWin* mainwin = nullptr;
    if ( uivwr2dmw )
	mainwin = uivwr2dmw->dockParent();
    else if ( uifvmw )
	mainwin = uifvmw->dockParent();
    else
	return;

    const int idx = viewers2d_.indexOf( mainwin );
    if ( idx==-1 )
	return;

    mDetachCB( viewers2d_.get(idx)->windowClosed,
	       uiViewer3DMgr::viewer2DClosedCB );
    viewers2d_.removeSingle( idx );
}


void uiViewer3DMgr::sceneChangeCB( CallBacker* )
{
    for ( int idx=0; idx<viewers3d_.size(); idx++ )
    {
	visSurvey::PreStackDisplay* psv = viewers3d_[idx];
	const visSurvey::Scene* scene = psv->getScene();
	ConstRefMan<PreStack::Gather> gatherdp = psv->getGather();
	const visSurvey::PlaneDataDisplay* pdd = psv->getSectionDisplay();
	const visSurvey::Seis2DDisplay*    s2d = psv->getSeis2DDisplay();
	if ( gatherdp && pdd && (!scene || scene->getFirstIdx( pdd )==-1 ) )
	{
	    removeViewWin( *gatherdp.ptr() );
	    viewers3d_.removeSingle( idx );
	    delete posdialogs_.removeSingle( idx );
	    delete settingdlgs_.removeSingle( idx );
	    if ( scene )
		visserv_->removeObject( psv, psv->getSceneID() );

	    idx--;
	}

	if ( gatherdp && s2d && (!scene || scene->getFirstIdx( s2d )==-1 ) )
	{
	    removeViewWin( *gatherdp.ptr() );
	    viewers3d_.removeSingle( idx );
	    delete posdialogs_.removeSingle( idx );
	    delete settingdlgs_.removeSingle( idx );
	    if ( scene )
		visserv_->removeObject( psv, psv->getSceneID() );

	    idx--;
	}
    }
}


void uiViewer3DMgr::removeViewWin( const PreStack::Gather& gather )
{
    for ( int idx=viewers2d_.size()-1; idx>=0; idx-- )
    {
	const uiFlatViewer* viewer = getViewer( idx );
	if ( !viewer )
	    continue;

	WeakPtr<FlatDataPack> pack = viewer->getPack( false );
	ConstRefMan<FlatDataPack> fdp = pack.get();
	mDynamicCastGet(const PreStack::Gather*,viewgather,fdp.ptr());
	if ( !viewgather || viewgather != &gather )
	    continue;

	delete viewers2d_.removeSingle( idx );
    }
}


void uiViewer3DMgr::sessionRestoreCB( CallBacker* )
{
    deepErase( viewers2d_ );

    TypeSet<VisID> vispsviewids;
    visserv_->findObject( typeid(visSurvey::PreStackDisplay), vispsviewids );

    for ( int idx=0; idx<vispsviewids.size(); idx++ )
    {
	mDynamicCastGet( visSurvey::PreStackDisplay*, psv,
			 visserv_->getObject(vispsviewids[idx]) );
	if ( !psv )
	    continue;

	if ( psv->getScene() )
	    psv->getScene()->change.notifyIfNotNotified(
		    mCB( this, uiViewer3DMgr, sceneChangeCB ) );
	viewers3d_ += psv;
	posdialogs_ += nullptr;
	settingdlgs_ += nullptr;
    }

    PtrMan<IOPar> allwindowspar = ODMainWin()->sessionPars().subselect(
				  sKey2DViewers() );
    if ( !allwindowspar )
	allwindowspar =
	    ODMainWin()->sessionPars().subselect( "PreStack 2D Viewers" );

    int nrwindows;
    if ( !allwindowspar || !allwindowspar->get(sKeyNrWindows(), nrwindows) )
	return;

    sessiongathers_ = new PreStack::GatherSetDataPack( nullptr );
    for ( int idx=0; idx<nrwindows; idx++ )
    {
	BufferString key = sKeyViewerPrefix();
	key += idx;
	PtrMan<IOPar> viewerpar = allwindowspar->subselect( key.buf() );
	if ( !viewerpar )
	    continue;

	MultiID mid;
	bool is3d;
	int trcnr;
	BinID bid;
	BufferString name2d;
	if ( !viewerpar->get( sKeyMultiID(), mid ) ||
	     !viewerpar->get( sKeyBinID(), bid ) ||
	     !viewerpar->get( sKeyTraceNr(), trcnr ) ||
	     !viewerpar->get( sKeyLineName(), name2d ) ||
	     !viewerpar->getYN( sKeyIsVolumeData(), is3d ) )
	{
	    if ( !viewerpar->get( "uiFlatViewWin MultiID", mid ) ||
		 !viewerpar->get( "uiFlatViewWin binid", bid ) ||
		 !viewerpar->get( "Seis2D TraceNr", trcnr ) ||
		 !viewerpar->get( "Seis2D Name", name2d ) ||
		 !viewerpar->getYN( "Seis3D display", is3d ) )
		continue;
	}

	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj )
	    continue;

	RefMan<PreStack::Gather> gather = new PreStack::Gather;
	DataPackID dpid;
	TrcKey tk;
	if ( is3d )
	    tk.setPosition( bid );
	else
	{
	    const Pos::GeomID gid = Survey::GM().getGeomID( name2d );
	    tk.setGeomID( gid ).setTrcNr( trcnr );
	}

	if ( gather->readFrom(*ioobj,tk) )
	    dpid = gather->id();
	else
	    continue;

	sessiongathers_->addGather( *gather.ptr() );
//	DPM(DataPackMgr::FlatID()).add( gather );

	uiString title;
	if ( is3d )
	    getSeis3DTitle( bid, ioobj->name(), title );
	else
	    getSeis2DTitle( trcnr, name2d, title );

	uiMainWin* viewwin = create2DViewer( title, *gather.ptr() );
	if ( !viewwin )
	    continue;

	viewers2d_ += viewwin;
	viewwin->show();
    }

    sessiongathers_->finalize();
    for ( int idx=0; idx<viewers3d_.size(); idx++ )
    {
	viewers3d_[idx]->setProcPar( *allwindowspar );
	viewers3d_[idx]->updateDisplay();
    }
}


void uiViewer3DMgr::getSeis2DTitle( int tracenr, const char* nm,
				    uiString& title )
{
    title = tr("Gather from [%1] at trace %2").arg( nm ).arg( tracenr );
}


void uiViewer3DMgr::getSeis3DTitle( const BinID& bid, const char* name,
				    uiString& title )
{
    title = tr("Gather from [%1] at %2" ).arg( name ).arg( bid.toString() );
}


void uiViewer3DMgr::sessionSaveCB( CallBacker* )
{
    IOPar allwindowpar;
    int nrsaved = 0;
    for ( int idx=0; idx<viewers2d_.size(); idx++ )
    {
	const uiFlatViewer* viewer = getViewer( idx );
	if ( !viewer )
	    continue;

	ConstRefMan<PreStack::Gather> gather = viewer->getPack( false ).get();
	if ( !gather )
	    continue;

	IOPar viewerpar;
	viewer->fillAppearancePar( viewerpar );
	viewerpar.set( sKeyBinID(), gather->getBinID() );
	viewerpar.set( sKeyMultiID(), gather->getStorageID() );
	viewerpar.set( sKeyTraceNr(), gather->getSeis2DTraceNr() );
	viewerpar.set( sKeyLineName(), gather->getSeis2DName() );
	viewerpar.setYN( sKeyIsVolumeData(), gather->is3D() );

	BufferString key = sKeyViewerPrefix();
	key += nrsaved;
	nrsaved++;

	allwindowpar.mergeComp( viewerpar, key );
    }

    if ( !viewers3d_.isEmpty() )
	viewers3d_[0]->getProcPar( allwindowpar );

    allwindowpar.set( sKeyNrWindows(), nrsaved );
    ODMainWin()->sessionPars().mergeComp( allwindowpar, sKey2DViewers() );
}


void uiViewer3DMgr::removeAllCB( CallBacker* )
{
    viewers3d_.erase();
    deepErase( posdialogs_ );
    deepErase( settingdlgs_ );
    deepErase( viewers2d_ );
}


void uiViewer3DMgr::surveyToBeChangedCB( CallBacker* )
{
    deepErase( settingdlgs_ );
    deepErase( posdialogs_ );
    deepErase( viewers2d_ );
    deepUnRef( viewers3d_ );
}


uiMainWin* uiViewer3DMgr::create2DViewer( const uiString& title,
					  const DataPackID& dpid )
{
    ConstRefMan<FlatDataPack> fdp =
			DPM(DataPackMgr::FlatID()).get<FlatDataPack>( dpid );
    mDynamicCastGet(const PreStack::Gather*,gather,fdp.ptr());
    if ( !gather )
	return nullptr;

    return create2DViewer( title, *gather );
}


void uiViewer3DMgr::removeViewWin( const DataPackID& dpid )
{
    ConstRefMan<FlatDataPack> fdp =
			DPM(DataPackMgr::FlatID()).get<FlatDataPack>( dpid );
    mDynamicCastGet(const PreStack::Gather*,gather,fdp.ptr());
    if ( gather )
	removeViewWin( *gather );
}

} // namespace PreStackView
