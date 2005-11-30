/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jan 2005
 RCS:           $Id: uivisemobj.cc,v 1.35 2005-11-30 22:34:05 cvskris Exp $
________________________________________________________________________

-*/

#include "uivisemobj.h"

#include "attribsel.h"
#include "emhistory.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "survinfo.h"
#include "uiexecutor.h"
#include "uicursor.h"
#include "uigeninputdlg.h"
#include "uimenu.h"
#include "uimpe.h"
#include "uimsg.h"
#include "uimenuhandler.h"
#include "uivispartserv.h"
#include "visdataman.h"
#include "visemobjdisplay.h"
#include "vismpeeditor.h"
#include "vissurvobj.h"

const char* uiVisEMObject::trackingmenutxt = "Tracking";


uiVisEMObject::uiVisEMObject( uiParent* uip, int newid, uiVisPartServer* vps )
    : displayid(newid)
    , visserv(vps)
    , uiparent(uip)
    , nodemenu( *new uiMenuHandler(uip,-1) )
    , interactionlinemenu( *new uiMenuHandler(uip,-1) )
    , edgelinemenu( *new uiMenuHandler(uip,-1) )
    , showedtexture(true)
{
    nodemenu.ref();
    interactionlinemenu.ref();
    edgelinemenu.ref();

    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    uiCursorChanger cursorchanger( uiCursor::Wait );

    const MultiID mid = emod->getMultiID();
    EM::ObjectID emid = EM::EMM().getObjectID(mid);
    if ( !EM::EMM().getObject(emid) )
    {
	PtrMan<Executor> exec = 0;
	EM::SurfaceIOData sd;
	if ( !EM::EMM().getSurfaceData( mid, sd ) )
	{
	    EM::SurfaceIODataSelection sel( sd );
	    sel.setDefault();

	    const BufferStringSet sections = emod->displayedSections();

	    TypeSet<int> sectionidx;
	    for ( int idx=sections.size()-1; idx>=0; idx-- )
	    {
		const int idy = sel.sd.sections.indexOf( *sections[idx] );
		if ( idy!=-1 )
		    sectionidx += idy;
	    }

	    if ( sectionidx.size() )
		sel.selsections = sectionidx;

	    const StepInterval<int> rowrg = emod->displayedRowRange();
	    const StepInterval<int> colrg = emod->displayedColRange();
	    if ( rowrg.step!=-1 && colrg.step!=-1 )
	    {
		sel.rg.start.inl = rowrg.start;
		sel.rg.start.crl = colrg.start;
		sel.rg.stop.inl = rowrg.stop;
		sel.rg.step.crl = colrg.step;
		sel.rg.step.inl = rowrg.step;
		sel.rg.stop.crl = colrg.stop;
	    }

	    exec = EM::EMM().objectLoader( mid, &sel );
	}
	else
	    exec = EM::EMM().objectLoader( mid );

	if ( exec )
	{
	    uiExecutor dlg( uiparent, *exec );
	    if ( dlg.go() )
		emid = EM::EMM().getObjectID(mid);
	}
    }

    if ( !emod->setEMObject(emid) ) { emod->unRef(); return; }

    if ( emod->usesTexture() &&
	    emod->getSelSpec()->id()==Attrib::SelSpec::cNoAttrib() )
	setDepthAsAttrib();

    setUpConnections();
}


#define mRefUnrefRet { emod->ref(); emod->unRef(); return; }

uiVisEMObject::uiVisEMObject( uiParent* uip, const EM::ObjectID& emid,
			      int sceneid, uiVisPartServer* vps )
    : displayid(-1)
    , visserv( vps )
    , uiparent( uip )
    , nodemenu( *new uiMenuHandler(uip,-1) )
    , interactionlinemenu( *new uiMenuHandler(uip,-1) )
    , edgelinemenu( *new uiMenuHandler(uip,-1) )
    , showedtexture(true)
{
    nodemenu.ref();
    interactionlinemenu.ref();
    edgelinemenu.ref();

    visSurvey::EMObjectDisplay* emod = visSurvey::EMObjectDisplay::create();

    mDynamicCastGet(visSurvey::Scene*,scene,visBase::DM().getObject(sceneid))
    emod->setDisplayTransformation( scene->getUTM2DisplayTransform() );

    uiCursorChanger cursorchanger(uiCursor::Wait);
    if ( !emod->setEMObject(emid) ) mRefUnrefRet

    EM::EMManager& em = EM::EMM();
    const EM::EMObject* emobj = EM::EMM().getObject(emid);

    visserv->addObject( emod, sceneid, true );
    displayid = emod->id();
    setDepthAsAttrib();

    setUpConnections();
}


uiVisEMObject::~uiVisEMObject()
{
    uiMenuHandler* menu = visserv->getMenu(displayid,false);
    if ( menu )
    {
	menu->createnotifier.remove( mCB(this,uiVisEMObject,createMenuCB) );
	menu->handlenotifier.remove( mCB(this,uiVisEMObject,handleMenuCB) );
    }

    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( emod && emod->getEditor() )
    {
	emod->getEditor()->noderightclick.remove(
		mCB(this,uiVisEMObject,nodeRightClick) );
	emod->getEditor()->interactionlinerightclick.remove(
		mCB(this,uiVisEMObject,interactionLineRightClick) );
    }

    nodemenu.createnotifier.remove( mCB(this,uiVisEMObject,createNodeMenuCB) );
    nodemenu.handlenotifier.remove( mCB(this,uiVisEMObject,handleNodeMenuCB) );
    nodemenu.unRef();

    interactionlinemenu.createnotifier.remove(
	    mCB(this,uiVisEMObject,createInteractionLineMenuCB) );
    interactionlinemenu.handlenotifier.remove(
	    mCB(this,uiVisEMObject,handleInteractionLineMenuCB) );
    interactionlinemenu.unRef();

    edgelinemenu.createnotifier.remove(
	    mCB(this,uiVisEMObject,createEdgeLineMenuCB) );
    edgelinemenu.handlenotifier.remove(
	    mCB(this,uiVisEMObject,handleEdgeLineMenuCB) );
    edgelinemenu.unRef();
}


bool uiVisEMObject::isOK() const
{
    return getDisplay();
}


void uiVisEMObject::prepareForShutdown()
{
    const MultiID mid = visserv->getMultiID( displayid );
    if ( mid==-1 ) return;

    const EM::ObjectID emid = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject(emid);
    if ( !emobj || !emobj->isChanged(-1) )
	return;

    BufferString msg( emobj->getTypeStr() );
    msg += " '";
    msg += emobj->name(); msg += "' has changed.\nDo you want to save it?";
    if ( uiMSG().notSaved( msg,0,false) )
    {
	PtrMan<Executor> saver = emobj->saver();
	uiCursorChanger uicursor( uiCursor::Wait );
	if ( saver ) saver->execute();
    }
}


void uiVisEMObject::setUpConnections()
{
    singlecolmnuitem.text = "Use single color";
    trackmenuitem.text = uiVisEMObject::trackingmenutxt;
    showseedsmnuitem.text = "Show seeds";
    wireframemnuitem.text = "Wireframe";
    editmnuitem.text = "Edit";
    shiftmnuitem.text = "Shift ...";
    fillholesitem.text = "Fill holes";
    showseedsmnuitem.text = "Show seeds";
    removesectionmnuitem.text ="Remove section";
    makepermnodemnuitem.text = "Make control permanent";
    removecontrolnodemnuitem.text = "Remove control";
    showonlyatsectionsmnuitem.text = "Display only at sections";
    changesectionnamemnuitem.text = "Change section's name";

    uiMenuHandler* menu = visserv->getMenu(displayid,true);
    menu->createnotifier.notify( mCB(this,uiVisEMObject,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiVisEMObject,handleMenuCB) );
    nodemenu.createnotifier.notify( mCB(this,uiVisEMObject,createNodeMenuCB) );
    nodemenu.handlenotifier.notify( mCB(this,uiVisEMObject,handleNodeMenuCB) );
    interactionlinemenu.createnotifier.notify(
	    mCB(this,uiVisEMObject,createInteractionLineMenuCB) );
    interactionlinemenu.handlenotifier.notify(
	    mCB(this,uiVisEMObject,handleInteractionLineMenuCB) );
    edgelinemenu.createnotifier.notify(
	    mCB(this,uiVisEMObject,createEdgeLineMenuCB) );
    edgelinemenu.handlenotifier.notify(
	    mCB(this,uiVisEMObject,handleEdgeLineMenuCB) );

    connectEditor();
}


void uiVisEMObject::connectEditor()
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( emod && emod->getEditor() )
    {
	emod->getEditor()->noderightclick.notifyIfNotNotified(
		mCB(this,uiVisEMObject,nodeRightClick) );

	emod->getEditor()->interactionlinerightclick.notifyIfNotNotified(
		mCB(this,uiVisEMObject,interactionLineRightClick) );

	//interactionlinemenu.setID( emod->getEditor()->lineID() );
	//edgelinemenu.setID( emod->getEditor()->lineID() );
    }
}

const char* uiVisEMObject::getObjectType( int id )
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,obj,visBase::DM().getObject(id))
    return obj ? EM::EMM().objectType( obj->getMultiID() ) : 0;
}


void uiVisEMObject::setDepthAsAttrib()
{
    uiCursorChanger cursorchanger( uiCursor::Wait );
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( emod ) emod->setDepthAsAttrib();
}


void uiVisEMObject::readAuxData()
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( emod ) emod->readAuxData();
}


int uiVisEMObject::nrSections() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return 0;

    EM::ObjectID emid = emod->getObjectID();
    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    return emobj ? emobj->nrSections() : 0;
}


EM::SectionID uiVisEMObject::getSectionID( int idx ) const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return -1;

    EM::ObjectID emid = emod->getObjectID();
    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    return emobj ? emobj->sectionID( idx ) : -1;
}


EM::SectionID uiVisEMObject::getSectionID( const TypeSet<int>* path ) const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    return path && emod ? emod->getSectionID( path ) : -1;
}


void uiVisEMObject::checkTrackingStatus()
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;
    emod->updateFromMPE();
}


float uiVisEMObject::getShift() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    return emod ? emod->getTranslation().z : 0;
}


void uiVisEMObject::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb)

    visSurvey::EMObjectDisplay* emod = getDisplay();

    const EM::ObjectID emid = emod->getObjectID();
    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    const EM::SectionID sid = emod->getSectionID(menu->getPath());

    mAddMenuItem( menu, &singlecolmnuitem, !emod->getOnlyAtSectionsDisplay(),
	  	  !emod->usesTexture() );
    mAddMenuItem( menu, &showonlyatsectionsmnuitem, true,
	          emod->getOnlyAtSectionsDisplay() );
    mAddMenuItem( menu, &changesectionnamemnuitem, 
	          emobj->canSetSectionName() && sid!=-1, false );
    mAddMenuItem( menu, &shiftmnuitem,
	    !strcmp(getObjectType(displayid),EM::Horizon::typeStr()), false );
    mAddMenuItem( menu, &fillholesitem,
	    !strcmp(getObjectType(displayid),EM::Horizon::typeStr()), false );

    mAddMenuItem(&trackmenuitem,&editmnuitem,true,emod->isEditingEnabled());
    mAddMenuItem(&trackmenuitem,&wireframemnuitem,true, emod->usesWireframe());
   
    const TypeSet<EM::PosID>* seeds =
			emobj->getPosAttribList(EM::EMObject::sSeedNode);
    mAddMenuItem(&trackmenuitem,&showseedsmnuitem,
	    	 seeds && seeds->size(),
		 emod->showsPosAttrib(EM::EMObject::sSeedNode));

    mAddMenuItem(menu,&trackmenuitem,trackmenuitem.nrItems(),false);

    mAddMenuItem( menu, &removesectionmnuitem, false, false );
    if ( emobj->nrSections()>1 && sid!=-1 )
	removesectionmnuitem.enabled = true;
}


void uiVisEMObject::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod || mnuid==-1 || menu->isHandled() )
	return;

    const EM::ObjectID emid = emod->getObjectID();
    EM::EMObject* emobj = EM::EMM().getObject(emid);
    const EM::SectionID sid = emod->getSectionID(menu->getPath());

    if ( mnuid==singlecolmnuitem.id )
    {
	emod->useTexture( !emod->usesTexture() );
	menu->setIsHandled(true);
    }
    else if ( mnuid==showonlyatsectionsmnuitem.id )
    {
	const bool turnon = !emod->getOnlyAtSectionsDisplay();
	setOnlyAtSectionsDisplay( turnon );
	menu->setIsHandled(true);
    }
    else if ( mnuid==changesectionnamemnuitem.id )
    {
	StringInpSpec* spec = new StringInpSpec( emobj->sectionName(sid) );
	uiGenInputDlg dlg(uiparent,"Change section-name", "Name", spec);
	while ( dlg.go() )
	{
	    if ( emobj->setSectionName(sid,dlg.text(), true ) )
	    {
		EM::History& history = EM::EMM().history();
		const int currentevent = history.currentEventNr();
		history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
		break;
	    }
	}

	menu->setIsHandled(true);
    }
    else if ( mnuid==wireframemnuitem.id )
    {
	menu->setIsHandled(true);
	emod->useWireframe( !emod->usesWireframe() );
    }
    else if ( mnuid==showseedsmnuitem.id )
    {
	menu->setIsHandled(true);
	emod->showPosAttrib( EM::EMObject::sSeedNode,
			     !emod->showsPosAttrib(EM::EMObject::sSeedNode),
			     Color( 200, 200, 200 ));
    }
    else if ( mnuid==editmnuitem.id )
    {
	bool turnon = !emod->isEditingEnabled();
	emod->enableEditing(turnon);
	if ( turnon ) connectEditor();
	menu->setIsHandled(true);
    }
    else if ( mnuid==shiftmnuitem.id )
    {
	menu->setIsHandled(true);
	Coord3 shift = emod->getTranslation();
	BufferString lbl( "Shift " ); lbl += SI().getZUnit();
	DataInpSpec* inpspec = new FloatInpSpec( shift.z );
	uiGenInputDlg dlg( uiparent,"Specify horizon shift", lbl, inpspec );
	if ( !dlg.go() ) return;

	double newshift = dlg.getfValue();
	if ( shift.z == newshift ) return;

	shift.z = newshift;
	emod->setTranslation( shift );
	if ( !emod->hasStoredAttrib() )
	    visserv->calculateAttrib( displayid, false );
	else
	{
	    uiMSG().error( "Cannot calculate this attribute on new location"
		           "\nDepth will be displayed instead" );
	    emod->setDepthAsAttrib();
	}
	visserv->triggerTreeUpdate();
    }
    else if ( mnuid==fillholesitem.id )
    {
	mDynamicCastGet(EM::Horizon*,emhzn,emobj);
	if ( !emhzn ) return;

	menu->setIsHandled(true);
	BufferString lbl( "Aperture: " );
	DataInpSpec* inpspec = new IntInpSpec( 5 );
	uiGenInputDlg dlg( uiparent, "Interpolation aperture size ", 
			   lbl, inpspec );
	if ( !dlg.go() ) return;

	const int aperture = dlg.getIntValue();
	if ( aperture<1 )
	    uiMSG().error( "Aperture size must be greater than 0" );
	else
	    emhzn->interpolateHoles(aperture);
    }
    else if ( mnuid==removesectionmnuitem.id )
    {
	if ( !emobj )
	    return;

	emobj->removeSection(sid, true );

	EM::History& history = EM::EMM().history();
	const int currentevent = history.currentEventNr();
	history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
    }
}


void uiVisEMObject::setOnlyAtSectionsDisplay( bool yn )
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    bool usetexture = false;
    if ( yn )
	showedtexture = emod->usesTexture();
    else 
	usetexture = showedtexture;

    emod->useTexture( usetexture );
    emod->setOnlyAtSectionsDisplay( yn );
}

#define mMakePerm	0
#define mRemoveCtrl	1
#define mRemoveNode	2


void uiVisEMObject::interactionLineRightClick( CallBacker* )
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    PtrMan<MPE::uiEMEditor> uimpeeditor =
	MPE::uiMPE().editorfact.create(uiparent,
				       emod->getEditor()->getMPEEditor());
    if ( !uimpeeditor ) return;

    interactionlinemenu.createnotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createInteractionLineMenus));
    interactionlinemenu.handlenotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleInteractionLineMenus));

    interactionlinemenu.executeMenu(uiMenuHandler::fromScene);

    interactionlinemenu.createnotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createInteractionLineMenus));
    interactionlinemenu.handlenotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleInteractionLineMenus));
}


void uiVisEMObject::nodeRightClick( CallBacker* )
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    PtrMan<MPE::uiEMEditor> uimpeeditor =
	MPE::uiMPE().editorfact.create(uiparent,
				       emod->getEditor()->getMPEEditor());
    if ( !uimpeeditor ) return;
    const EM::PosID empid = emod->getEditor()->getNodePosID(
				emod->getEditor()->getRightClickNode());
    if ( empid.objectID()==-1 )
	return;

    uimpeeditor->setActiveNode( empid );

    nodemenu.createnotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createNodeMenus));
    nodemenu.handlenotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleNodeMenus));

    nodemenu.executeMenu(uiMenuHandler::fromScene);

    nodemenu.createnotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createNodeMenus));
    nodemenu.handlenotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleNodeMenus));
}


void uiVisEMObject::edgeLineRightClick( CallBacker* cb )
{
    /*
    mCBCapsuleUnpack(const visSurvey::EdgeLineSetDisplay*,edgelinedisplay,cb);
    if ( !edgelinedisplay ) return;

    edgelinemenu.setID(edgelinedisplay->id());
    edgelinemenu.executeMenu(uiMenuHandler::fromScene);
    */
}


void uiVisEMObject::createNodeMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    visSurvey::EMObjectDisplay* emod = getDisplay();
    const EM::PosID empid = emod->getEditor()->getNodePosID(
				emod->getEditor()->getRightClickNode());
    if ( empid.objectID()==-1 )
	return;

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.getObject(empid.objectID());

    mAddMenuItem( menu, &makepermnodemnuitem,
	          emobj->isPosAttrib(empid,EM::EMObject::sTemporaryControlNode),
		  false );

    mAddMenuItem( menu, &removecontrolnodemnuitem,
	emobj->isPosAttrib(empid,EM::EMObject::sPermanentControlNode),
	true);
/*
    removenodenodemnuitem = emobj->isDefined(*empid)
        ? menu->addItem( new uiMenuItem("Remove node") )
	: -1;

    uiMenuItem* snapitem = new uiMenuItem("Snap after edit");
    tooglesnappingnodemnuitem = menu->addItem(snapitem);
    snapitem->setChecked(emod->getEditor()->snapAfterEdit());
*/
}


EM::ObjectID uiVisEMObject::getObjectID() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return -1;

    return emod->getObjectID();
}



void uiVisEMObject::handleNodeMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiMenuHandler*,menu,caller)
    if ( mnuid==-1 || menu->isHandled() )
	return;

    visSurvey::EMObjectDisplay* emod = getDisplay();
    const EM::PosID empid = emod->getEditor()->getNodePosID(
				emod->getEditor()->getRightClickNode());

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.getObject(empid.objectID());

    if ( mnuid==makepermnodemnuitem.id )
    {
	menu->setIsHandled(true);
        emobj->setPosAttrib(empid,EM::EMObject::sPermanentControlNode,true);
	emobj->setPosAttrib(empid,EM::EMObject::sTemporaryControlNode,false);
	emobj->setPosAttrib(empid,EM::EMObject::sEdgeControlNode,false);
    }
    else if ( mnuid==removecontrolnodemnuitem.id )
    {
	menu->setIsHandled(true);
        emobj->setPosAttrib(empid,EM::EMObject::sPermanentControlNode,false);
	emobj->setPosAttrib(empid,EM::EMObject::sTemporaryControlNode,false);
	emobj->setPosAttrib(empid,EM::EMObject::sEdgeControlNode,false);
    }
}


visSurvey::EMObjectDisplay* uiVisEMObject::getDisplay()
{
    mDynamicCastGet( visSurvey::EMObjectDisplay*, emod,
		     visserv->getObject(displayid));
    return emod;
}


const visSurvey::EMObjectDisplay* uiVisEMObject::getDisplay() const
{
    return const_cast<uiVisEMObject*>(this)->getDisplay();
}


void uiVisEMObject::createEdgeLineMenuCB( CallBacker* cb )
{
}


void uiVisEMObject::handleEdgeLineMenuCB( CallBacker* cb )
{
}

