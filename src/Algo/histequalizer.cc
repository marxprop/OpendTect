/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "histequalizer.h"
#include "dataclipper.h"
#include "typeset.h"
#include "statrand.h"

HistEqualizer::HistEqualizer( const int nrseg )
    : datapts_(*new LargeValVec<float>() )
    , nrseg_(nrseg)
    , gen_(*new Stats::RandGen())
{
}


HistEqualizer::~HistEqualizer()
{
    delete &gen_;
}


void HistEqualizer::setData( const LargeValVec<float>& datapts )
{
    datapts_ = datapts;
    update();
}


void HistEqualizer::setRawData( const TypeSet<float>& datapts )
{
    DataClipper clipper;
    clipper.putData( datapts.arr(), datapts.size() );
    clipper.fullSort();
    setData( clipper.statPts() );
}


void HistEqualizer::update()
{
    const int datasz = mCast( int, datapts_.size() );
    if ( histeqdatarg_ )
	delete histeqdatarg_;
    histeqdatarg_ = new TypeSet< Interval<float> >();
    int index = 0;
    TypeSet<int> segszs;
    getSegmentSizes( segszs );
    for ( int idx = 0; idx < segszs.size(); idx++ )
    {
	int startidx = index;
	int stopindex = startidx + segszs[idx] > datasz - 1 ?
			datasz - 1 : startidx + segszs[idx];
	*histeqdatarg_ += Interval<float> ( datapts_[startidx],
					    datapts_[stopindex]);
	index = stopindex;
    }
    TypeSet< Interval<float> > testprint( *histeqdatarg_ );
}


float HistEqualizer::position( float val ) const
{
    int start = 0;
    int end = nrseg_ -1;
    int midval = ( nrseg_ -1 )/2;
    float ret = 0;
    Interval<float> startrg = (*histeqdatarg_)[start];
    Interval<float> stoprg = (*histeqdatarg_)[end];
    if (  val < startrg.stop_ )
	return 0;
    if (  val > stoprg.start_ )
	return 1;
    while ( end-start > 1 )
    {
	Interval<float> midvalrg = (*histeqdatarg_)[midval];
	if ( midvalrg.start_ <= val && midvalrg.stop_ >= val )
	{
	    ret = (float)midval/(float)nrseg_;
	    if ( ret > 1 ) ret = 1;
	    else if ( ret < 0 ) ret = 0;
	    return ret;
	}
	if ( midvalrg.stop_ < val )
	{
	    start = midval;
	    midval = start + ( end - start ) / 2;
	}
	else if ( midvalrg.start_ > val )
	{
	    end = midval;
	    midval = end - ( end - start ) / 2;
	}
    }
    return ret;
}


void HistEqualizer::getSegmentSizes( TypeSet<int>& segszs )
{
    const int datasz = mCast( int, datapts_.size() );
    const int aindexlength = (int)(datasz/nrseg_);
    const int bindexlength = aindexlength+1;
    const int numberofa = bindexlength*nrseg_ - datasz;
    const int numberofb = nrseg_ - numberofa;

    segszs.setSize( nrseg_, aindexlength );
    int count = 0;
    while ( true )
    {
	if ( numberofb == 0 )
	    break;
	int idx = gen_.getIndex( nrseg_ );
	if ( segszs[idx] == bindexlength )
	    continue;

	segszs[idx] = bindexlength;
	count++;
	if ( count == numberofb )
	    break;
    }
}
