/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodvw2dvariabledensity.h"

#include "uiattribpartserv.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatviewcoltabed.h"
#include "uiflatviewstdcontrol.h"
#include "uimenuhandler.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uitaskrunner.h"
#include "uitreeview.h"

#include "attribdatapackzaxistransformer.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "filepath.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "view2dseismic.h"
#include "view2ddataman.h"
#include "zaxistransform.h"


uiODVW2DVariableDensityTreeItem::uiODVW2DVariableDensityTreeItem()
    : uiODVw2DTreeItem( "VD" )
    , dummyview_(0)
    , menu_(0)
    , coltabinitialized_(false)
    , selattrmnuitem_(tr("Select Attribute"))
{}


uiODVW2DVariableDensityTreeItem::~uiODVW2DVariableDensityTreeItem()
{
    detachAllNotifiers();
    if ( menu_ )
	menu_->unRef();

    viewer2D()->dataMgr()->removeObject( dummyview_ );
}


bool uiODVW2DVariableDensityTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    mAttachCB( vwr.dataChanged,uiODVW2DVariableDensityTreeItem::dataChangedCB );

    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( true );
    uitreeviewitem_->setChecked( ddp.vd_.show_ );
    mAttachCB( checkStatusChange(), uiODVW2DVariableDensityTreeItem::checkCB );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );
    mAttachCB( dummyview_->deSelection(),
	       uiODVW2DVariableDensityTreeItem::deSelectCB );

    if ( vwr.hasPack(false) )
    {
	if ( !vwr.control() )
	    displayMiniCtab(0);

	ColTab::Sequence seq( vwr.appearance().ddpars_.vd_.ctab_ );
	displayMiniCtab( &seq );
    }

    return true;
}


void uiODVW2DVariableDensityTreeItem::initColTab()
{
    if ( coltabinitialized_ ) return;
    mAttachCB( viewer2D()->viewControl()->colTabEd()->colTabChgd,
	       uiODVW2DVariableDensityTreeItem::colTabChgCB );
    uitreeviewitem_->setSelected( true );
    select(); coltabinitialized_ = true;
}


bool uiODVW2DVariableDensityTreeItem::select()
{
    if ( !uitreeviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( dummyview_ );
    uiFlatViewColTabEd* coltabed = viewer2D()->viewControl()->colTabEd();
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    coltabed->setColTab( vwr.appearance().ddpars_.vd_ );
    return true;
}


void uiODVW2DVariableDensityTreeItem::deSelectCB( CallBacker* )
{
    viewer2D()->viewControl()->colTabEd()->setSensitive( false );
}


void uiODVW2DVariableDensityTreeItem::checkCB( CallBacker* )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    if ( !vwr.hasPack(false) )
    {
	if ( !isChecked() ) return;
	const DataPack::ID dpid = viewer2D()->getDataPackID( false );
	if ( dpid != DataPack::cNoID() ) viewer2D()->setUpView( dpid, false );
	return;
    }

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	viewer2D()->viewwin()->viewer(ivwr).setVisible( false, isChecked() );
}


void uiODVW2DVariableDensityTreeItem::colTabChgCB( CallBacker* cb )
{
    if ( viewer2D()->dataMgr()->selectedID() != dummyview_->id() )
	return;

    mDynamicCastGet(uiFlatViewColTabEd*,coltabed,cb);
    if ( !coltabed ) return;

    const FlatView::DataDispPars::VD& vdpars = coltabed->getDisplayPars();
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(ivwr);
	vwr.appearance().ddpars_.vd_ = vdpars;
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }
}


void uiODVW2DVariableDensityTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    if ( !vwr.hasPack(false) || !vwr.control() )
    {
	displayMiniCtab( 0 );
	return;
    }

    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( vwr.isVisible(true) &&
				   viewer2D()->selSpec(false).id().isValid() );
    uitreeviewitem_->setChecked( ddp.vd_.show_ );

    if ( !coltabinitialized_ ) initColTab();

    if ( viewer2D()->dataMgr()->selectedID() == dummyview_->id() )
	viewer2D()->viewControl()->colTabEd()->setColTab( ddp.vd_ );

    ColTab::Sequence seq( ddp.vd_.ctab_ );
    displayMiniCtab( &seq );
}


void uiODVW2DVariableDensityTreeItem::dataTransformCB( CallBacker* )
{
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(ivwr);
	const TypeSet<DataPack::ID> ids = vwr.availablePacks();
	for ( int idx=0; idx<ids.size(); idx++ )
	    if ( ids[idx]!=vwr.packID(false) && ids[idx]!=vwr.packID(true) )
		vwr.removePack( ids[idx] );
    }

    Attrib::SelSpec& selspec = viewer2D()->selSpec( false );
    if ( selspec.isZTransformed() ) return;

    const DataPack::ID dpid = createDataPack( selspec );
    if ( dpid != DataPack::cNoID() )
	viewer2D()->setUpView( dpid, false );
}


void uiODVW2DVariableDensityTreeItem::displayMiniCtab(
						const ColTab::Sequence* seq )
{
    if ( !seq )
    {
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
	return;
    }

    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *seq );
}


bool uiODVW2DVariableDensityTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	mAttachCB( menu_->createnotifier,
		   uiODVW2DVariableDensityTreeItem::createMenuCB );
	mAttachCB( menu_->handlenotifier,
		   uiODVW2DVariableDensityTreeItem::handleMenuCB );
    }
    return menu_->executeMenu(uiMenuHandler::fromTree());
}


void uiODVW2DVariableDensityTreeItem::createMenuCB( CallBacker* cb )
{
    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_ );

    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( selattrmnuitem_.nrItems() )
	mAddMenuItem( menu, &selattrmnuitem_, true, false );
}


void uiODVW2DVariableDensityTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( handleSelMenu(mnuid) )
	menu->setIsHandled(true);
}


void uiODVW2DVariableDensityTreeItem::createSelMenu( MenuItem& mnu )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstDataPackRef<FlatDataPack> dp = vwr.obtainPack( false, true );
    if ( !dp ) return;

    const Attrib::SelSpec& as = viewer2D()->selSpec( false );
    MenuItem* subitem = 0;
    applMgr()->attrServer()->resetMenuItems();

    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp.ptr());
    if ( dp2ddh )
    {
//	BufferString ln;
//	dp2ddh->getLineName( ln );
// TODO: Use lnm to get attributes for this line only
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,true,false);
    }
    else
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,false,false);
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
    subitem = applMgr()->attrServer()->calcAttribMenuItem( as, dp2ddh, true );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
    if ( dp2ddh )
    {
//	BufferString ln;
//	dp2ddh->getLineName( ln );
// TODO: Use lnm to get attributes for this line only
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,true,true);
    }
    else
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,false,true );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
}


bool uiODVW2DVariableDensityTreeItem::handleSelMenu( int mnuid )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstDataPackRef<FlatDataPack> dp = vwr.obtainPack( false, true );
    if ( !dp ) return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    bool dousemulticomp, stored, steering;
    dousemulticomp = stored = steering = false;

    BufferString attrbnm;
    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp.ptr());
    if ( dp2ddh )
	attrserv->info2DAttribSubMenu( mnuid, attrbnm, steering, stored );

    Attrib::SelSpec selas = viewer2D()->selSpec( false );
    if ( !stored && !attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
	return false;

    const DataPack::ID dpid =
	createDataPack( selas, attrbnm.buf(), steering, stored );
    if ( dpid == DataPack::cNoID() ) return false;

    useStoredDispPars( selas );
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	FlatView::DataDispPars& ddpars =
	    viewer2D()->viewwin()->viewer(ivwr).appearance().ddpars_;
	ddpars.vd_.show_ = true;
    }

    viewer2D()->setSelSpec( &selas, false );
    viewer2D()->setUpView( dpid, false );
    return true;
}


DataPack::ID uiODVW2DVariableDensityTreeItem::createDataPack(
			Attrib::SelSpec& selas, const BufferString& attrbnm,
			const bool steering, const bool stored )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstDataPackRef<FlatDataPack> dp = vwr.obtainPack( false, true );
    if ( !dp ) return DataPack::cNoID();

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    attrserv->setTargetSelSpec( selas );

    mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,dp.ptr());
    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp.ptr());

    DataPack::ID newid = DataPack::cNoID();
    RefMan<ZAxisTransform> zat = viewer2D()->getZAxisTransform();
    if ( dp2ddh )
    {
	if ( stored )
	{
	    const SeisIOObjInfo objinfo( attrbnm );
	    if ( !objinfo.ioObj() )
		return DataPack::cNoID();

	    Attrib::DescID attribid = attrserv->getStoredID(
			    objinfo.ioObj()->key(), true, steering ? 1 : 0 );
	    selas.set( attrbnm, attribid, false, 0 );
	    selas.set2DFlag();

	    const Attrib::DescSet* ds = Attrib::DSHolder().getDescSet( true,
								       true );
	    if ( !ds ) return DataPack::cNoID();

	    selas.setRefFromID( *ds );
	    selas.setUserRef( attrbnm );

	    const Attrib::Desc* targetdesc = ds->getDesc( attribid );
	    if ( !targetdesc )
		return DataPack::cNoID();

	    BufferString defstring;
	    targetdesc->getDefStr( defstring );
	    selas.setDefString( defstring );
	    attrserv->setTargetSelSpec( selas );
	}

	uiTaskRunner uitr( &viewer2D()->viewwin()->viewer() );
	TrcKeyZSampling tkzs = dp2ddh->getTrcKeyZSampling();
	if ( zat )
	{
	    tkzs.zsamp_.setFrom( zat->getZInterval(true) );
	    tkzs.zsamp_.step = SI().zStep();
	}

	newid = attrserv->create2DOutput( tkzs, dp2ddh->getGeomID(), uitr );
    }
    else if ( dprdm )
    {
	attrserv->setTargetSelSpec( selas );
	const Interval<float> zrg = zat ? zat->getZInterval(true) :
		Interval<float>((float)dprdm->posData().range(false).start,
				(float)dprdm->posData().range(false).stop);

	TypeSet<BinID> bids;
	if ( dprdm->pathBIDs() )
	    bids = *dprdm->pathBIDs();
	newid = attrserv->createRdmTrcsOutput( zrg, &bids, &bids );
    }
    else
    {
	newid = viewer2D()->createDataPack( selas );
    }

    if ( zat && !selas.isZTransformed() && (dp2ddh || dprdm) )
    {
	ConstDataPackRef<FlatDataPack> newdp =
		DPM(DataPackMgr::FlatID()).obtain( newid );
	Attrib::FlatDataPackZAxisTransformer transformer( *zat );
	transformer.setInput( newdp.ptr() );
	transformer.setOutput( newid );
	transformer.execute();
    }

    return newid;
}


void uiODVW2DVariableDensityTreeItem::useStoredDispPars(
				const Attrib::SelSpec& selspec )
{
    PtrMan<IOObj> ioobj = applMgr()->attrServer()->getIOObj( selspec );
    if ( ioobj )
    {
	FilePath fp( ioobj->fullUserExpr(true) );
	fp.setExtension( "par" );
	IOPar iop;
	if ( iop.read(fp.fullPath(),sKey::Pars()) && !iop.isEmpty() )
	{
	    ColTab::Sequence seq( 0 );
	    const char* ctname = iop.find( sKey::Name() );
	    seq = ColTab::Sequence( ctname );
	    displayMiniCtab( &seq );

	    ColTab::MapperSetup mapper;
	    mapper.usePar( iop );

	    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	    {
		FlatView::DataDispPars& ddp =
		    viewer2D()->viewwin()->viewer(ivwr).appearance().ddpars_;
		ddp.vd_.ctab_ = ctname;
		ddp.vd_.mappersetup_ = mapper;
	    }
	}
    }
}


uiTreeItem* uiODVW2DVariableDensityTreeItemFactory::createForVis(
					const uiODViewer2D&,int id )const
{
    return 0;
}

