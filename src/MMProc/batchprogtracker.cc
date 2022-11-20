/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprogtracker.h"

#include "clientservicebase.h"
#include "file.h"
#include "ioman.h"


BatchProgramTracker& eBPT()
{
    mDefineStaticLocalObject(PtrMan<BatchProgramTracker>,
						    theinstance, = nullptr);
    return *theinstance.createIfNull();
}


const BatchProgramTracker& BPT()
{
    return eBPT();
}



BatchProgramTracker::BatchProgramTracker()
{
    BatchServiceClientMgr& mgr = BatchServiceClientMgr::getMgr();
    mAttachCB( mgr.batchStarted, BatchProgramTracker::registerProcess );
    mAttachCB( mgr.batchEnded, BatchProgramTracker::unregisterProcess );
    mAttachCB( mgr.batchFinished, BatchProgramTracker::unregisterProcess );
    mAttachCB( mgr.batchKilled, BatchProgramTracker::unregisterProcess );
}


BatchProgramTracker::~BatchProgramTracker()
{
    detachAllNotifiers();
}


void BatchProgramTracker::registerProcess( CallBacker* cb )
{
    if ( !cb )
	return;

    BatchServiceClientMgr& mgr = BatchServiceClientMgr::getMgr();
    mCBCapsuleUnpack(Network::Service::ID, servid, cb);
    if ( !mgr.isPresent(servid) )
	return;

    serviceids_.add( servid );
}


void BatchProgramTracker::unregisterProcess( CallBacker* cb )
{
    if ( !cb )
	return;

    BatchServiceClientMgr& mgr = BatchServiceClientMgr::getMgr();
    mCBCapsuleUnpack(Network::Service::ID, servid, cb);
    const int idx = serviceids_.indexOf( servid );
    if ( idx < 0 )
	return;

    File::remove(mgr.getLockFileFP(servid));
    serviceids_.removeSingle(idx);
}


const TypeSet<Network::Service::ID>& BatchProgramTracker::getServiceIDs() const
{
    return serviceids_;
}


void BatchProgramTracker::cleanAll()
{
    BatchServiceClientMgr& mgr = BatchServiceClientMgr::getMgr();
    for (const auto& servid : serviceids_)
	File::remove( mgr.getLockFileFP(servid) );
}


void BatchProgramTracker::cleanProcess( const Network::Service::ID& servid )
{
    File::remove( BatchServiceClientMgr::getMgr().getLockFileFP( servid ) );
}