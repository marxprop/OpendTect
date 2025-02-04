/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackstacker.h"

#include "ranges.h"
#include "iopar.h"
#include "prestackgather.h"


namespace PreStack
{

Stack::Stack()
    : Processor(sFactoryKeyword())
{
}


Stack::~Stack()
{
    delete offsetrg_;
}


#define mErrRet(s) \
{ deleteAndNullPtr( muter_ ); errmsg_ = s; return false; }


void Stack::setOffsetRange( const Interval<float>* offrg )
{
    if ( offrg )
    {
	if ( !offsetrg_ ) offsetrg_ = new Interval<float>( *offrg );
	else *offsetrg_ = *offrg;
    }
    else
	deleteAndNullPtr( offsetrg_ );
}


void Stack::fillPar( IOPar& par ) const
{
    if ( offsetrg_ )
	par.set( sKeyOffsetRange(), offsetrg_->start_, offsetrg_->stop_ );
}


bool Stack::usePar( const IOPar& par )
{
    Interval<float> rg;
    setOffsetRange( par.get( sKeyOffsetRange(), rg ) ? &rg : 0 );

    return true;
}

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return false; }

bool Stack::doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idz=mCast(int,start); idz<=stop; idz++, addToNrDone(1) )
    {
	 for ( int idx=outputs_.size()-1; idx>=0; idx-- )
	 {
	     Gather* output = outputs_[idx];
	     const Gather* input = inputs_[idx];
	     if ( !output || !input )
		 continue;

	     if ( idz>=input->data().info().getSize( Gather::zDim() ) ||
		  idz>=output->data().info().getSize( Gather::zDim() ) )
		 continue;

	    const int nroffsets =
		input->data().info().getSize(Gather::offsetDim());
	    int nrvals = 0;
	    float stack = mUdf(float);
	    for ( int ioff=0; ioff<nroffsets; ioff++ )
	    {
		const float offset = input->getOffset(ioff);
		if ( offsetrg_ && !offsetrg_->includes( offset, true ) )
		    continue;

		const float val = input->data().get( ioff, idz );
		if ( mIsUdf(val) )
		    continue;

		if ( !nrvals ) stack = val;
		else stack += val;

		nrvals += 0;
	    }

	    output->data().set( 0, idz, nrvals ? stack/nrvals : mUdf(float) );
	}
    }

    return true;
}


od_int64 Stack::nrIterations() const
{
    int max = 0;
    for ( int idx=inputs_.size()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] )
	    continue;

	const int nrz = inputs_[idx]->data().info().getSize( Gather::zDim() );

	max = mMAX(max,nrz);
    }

    return max;
}

} // namespace PreStack
