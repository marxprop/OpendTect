/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribdescsetman.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "iopar.h"
#include "keystrs.h"

namespace Attrib
{

DescSetMan::DescSetMan( bool is2d, DescSet* ads, bool destr )
    : ads_(ads)
    , is2d_(is2d)
    , unsaved_(false)
    , destrondel_(destr)
    , inpselhist_(*new IOPar( "Input Attribute history" ))
    , steerselhist_(*new IOPar( "Steering selection history" ))
{
    if ( !ads_ )
	ads_ = new DescSet(is2d_);
    else
	fillHist();
}


DescSetMan::~DescSetMan()
{
    if ( destrondel_ ) delete ads_;
    delete &inpselhist_;
    delete &steerselhist_;
}


void DescSetMan::setDescSet( DescSet* newads )
{
    if ( newads == ads_ )
	return;
    if ( !ads_ )
	{ ads_ = newads; return; }
    if ( !newads )
	{ inpselhist_.setEmpty(); ads_ = newads; return; }

    // Remove invalid entries
    fillHist();
    cleanHist( inpselhist_, *newads );
    cleanHist( steerselhist_, *newads );

    ads_ = newads;
}


void DescSetMan::cleanHist( IOPar& selhist, const DescSet& newads )
{
    IOParIterator iter( selhist );
    BufferString key, val;
    while ( iter.next(key,val) )
    {
	const int id = val.toInt();
	if ( id < 0 ) continue;

	ConstRefMan<Desc> desc = ads_->getDesc( DescID(id,false) );
	bool keep = false;
	if ( desc )
	{
	    if ( newads.getID(desc->userRef(),true).asInt() >= 0 )
		keep = true;
	}

	if ( !keep )
	    selhist.removeWithKey( key );
    }
}


void DescSetMan::fillHist()
{
    inpselhist_.setEmpty();

    // First add one empty
    inpselhist_.set( IOPar::compKey(sKey::IOSelection(),1), -1 );

    int nr = 1;
    TypeSet<DescID> attribids;
    ads_->getIds( attribids );
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	RefMan<Desc> ad = ads_->getDesc( attribids[idx] );
	if ( !ad || ad->isHidden() || ad->isStored() ) continue;

	const BufferString key( "", attribids[idx].asInt() );
	if ( inpselhist_.hasKey(key) )
	    continue;

	nr++;
	inpselhist_.set( IOPar::compKey(sKey::IOSelection(),nr), key );
    }
}

} // namespace Attrib
