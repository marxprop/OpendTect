/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.40 2005-02-16 14:13:20 cvskris Exp $";

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emhistory.h"
#include "emsurfacetr.h"
#include "emhorizontaltube.h"
#include "emsticksettransl.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "errh.h"
#include "executor.h"
#include "iodir.h"
#include "ioman.h"
#include "ptrman.h"


EM::EMManager& EM::EMM()
{
    static PtrMan<EMManager> emm = 0;

    if ( !emm ) emm = new EM::EMManager;
    return *emm;
}

EM::EMManager::EMManager()
    : history_( *new EM::History(*this) )
{
    init();
}


EM::EMManager::~EMManager()
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	EMObjectCallbackData cbdata;
	cbdata.event = EMObjectCallbackData::Removal;

	const int oldsize = objects.size();
	objects[idx]->notifier.trigger(cbdata);
	if ( oldsize!=objects.size() ) idx--;
    }

    deepRef( objects );		//Removes all non-reffed 
    deepUnRef( objects );

    if ( objects.size() )
	pErrMsg( "All objects are not unreffed" );

    deepErase( objectfactories );
    delete &history_;
}


const EM::History& EM::EMManager::history() const
{ return history_; }


EM::History& EM::EMManager::history()
{ return history_; }


BufferString EM::EMManager::objectName(const EM::ObjectID& oid) const
{
    if ( getObject(oid) ) return getObject(oid)->name();
    MultiID mid = IOObjContext::getStdDirData(IOObjContext::Surf)->id;
    mid.add(oid);

    PtrMan<IOObj> ioobj = IOM().get( mid );
    BufferString res;
    if ( ioobj ) res = ioobj->name();
    return res;
}


EM::ObjectID EM::EMManager::findObject( const char* type,
					const char* name ) const
{
    const IOObjContext* context = getContext(type);
    if ( IOM().to(IOObjContext::getStdDirData(context->stdseltype)->id) )
    {
	PtrMan<IOObj> ioobj = IOM().getLocal( name );
	IOM().back();
	if ( ioobj ) return multiID2ObjectID(ioobj->key());
    }

    return -1;
}


const char* EM::EMManager::objectType(const EM::ObjectID& oid) const
{
    if ( getObject(oid) )
	return getObject(oid)->getTypeStr();

    MultiID mid = IOObjContext::getStdDirData(IOObjContext::Surf)->id;
    mid.add(oid);

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) 
	return 0;

    static BufferString objtype;
    objtype = ioobj->group();
    return objtype.buf();
}


void EM::EMManager::init()
{
    Horizon::initClass(*this);
    Fault::initClass(*this);
    HorizontalTube::initClass(*this);
    StickSet::initClass(*this);
} 


EM::ObjectID EM::EMManager::createObject( const char* type, const char* name )
{
    EM::EMObject* object = 0;
    for ( int idx=0; idx<objectfactories.size(); idx++ )
    {
	if ( !strcmp(type,objectfactories[idx]->typeStr()) )
	{
	    object = objectfactories[idx]->create( name, false, *this );
	    break;
	}
    }

    if ( !object )
	return -1;

    return object->id();
} 


EM::EMObject* EM::EMManager::getObject( const EM::ObjectID& id )
{
    const EM::ObjectID objid = multiID2ObjectID(id);
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==objid )
	    return objects[idx];
    }

    return 0;
}


const EM::EMObject* EM::EMManager::getObject( const EM::ObjectID& id ) const
{ return const_cast<EM::EMManager*>(this)->getObject(id); }


EM::ObjectID EM::EMManager::multiID2ObjectID( const MultiID& id )
{ return id.leafID(); }


void EM::EMManager::addObject( EM::EMObject* obj )
{
    if ( !obj ) return;
    objects += obj;
}


void EM::EMManager::removeObject( EM::EMObject* obj )
{
    objects -= obj;
}


EM::EMObject* EM::EMManager::createTempObject( const char* type )
{
    for ( int idx=0; idx<objectfactories.size(); idx++ )
    {
	if ( !strcmp(type,objectfactories[idx]->typeStr()) )
	    return objectfactories[idx]->create( 0, true, *this );
    }

    return 0;
}


EM::ObjectID EM::EMManager::objectID(int idx) const
{ return idx>=0 && idx<objects.size() ? objects[idx]->id() : -1; }


Executor* EM::EMManager::loadObject( const MultiID& mid,
				     const SurfaceIODataSelection* iosel )
{
    EM::ObjectID id = multiID2ObjectID(mid);
    EMObject* obj = getObject( id );
   
    if ( !obj )
    {
	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj ) return 0;

	obj = createTempObject( ioobj->group() );
	if ( obj )
	{ obj->setID(id); }
    }

    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( surface )
	return surface->geometry.loader(iosel);
    else if ( obj )
	return obj->loader();

    return 0;
}


const char* EM::EMManager::getSurfaceData( const MultiID& id,
					   EM::SurfaceIOData& sd )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
	return id == "" ? 0 : "Object Manager cannot find surface";

    const char* grpname = ioobj->group();
    if ( !strcmp(grpname,EMHorizonTranslatorGroup::keyword) ||
	    !strcmp(grpname,EMFaultTranslatorGroup::keyword) )
    {
	PtrMan<EMSurfaceTranslator> tr = 
	    		(EMSurfaceTranslator*)ioobj->getTranslator();
	if ( !tr )
	{ return "Cannot create translator"; }

	if ( !tr->startRead( *ioobj ) )
	{
	    static BufferString msg;
	    msg = tr->errMsg();
	    if ( msg == "" )
		{ msg = "Cannot read '"; msg += ioobj->name(); msg += "'"; }

	    return msg.buf();
	}

	const EM::SurfaceIOData& newsd = tr->selections().sd;
	sd.rg = newsd.rg;
	deepCopy( sd.sections, newsd.sections );
	deepCopy( sd.valnames, newsd.valnames );
	return 0;
    }

    pErrMsg("(read surface): unknown tr group");
    return 0;
}


void EM::EMManager::addFactory( ObjectFactory* fact )
{
    for ( int idx=0; idx<objectfactories.size(); idx++ )
    {
	if ( !strcmp(fact->typeStr(),objectfactories[idx]->typeStr()) )
	{
	    delete fact;
	    return;
	}
    }

    objectfactories += fact;
}


const IOObjContext* EM::EMManager::getContext( const char* type ) const
{
    for ( int idx=0; idx<objectfactories.size(); idx++ )
    {
	if ( !strcmp(type,objectfactories[idx]->typeStr()) )
	    return &objectfactories[idx]->ioContext();
    }

    return 0;
}

