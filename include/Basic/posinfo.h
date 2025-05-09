#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"
#include "indexinfo.h"
#include "manobjectset.h"
#include "od_iosfwd.h"
#include "typeset.h"
#include "trckeyzsampling.h"

namespace OD { enum class SliceType; }

/*!\brief Position info, often segmented

In data cubes with gaps and other irregulaities, a complete description
of the positions present can be done by describing the regular segments
per inline. No sorting of inlines is required.

The crossline segments are assumed to be sorted, i.e.:
[1-3,1] [5-9,2] : OK
[9-5,-1] [3-1,-1] : OK
[5-9,2] [1-3,1] : Not OK

Note that the LineData class is also interesting for 2D lines with trace
numbers.

*/

namespace PosInfo
{

/*!
\brief Position info for a line - in a 3D cube, that would be an inline.
Stored as (crossline-)number segments.
*/

mExpClass(Basic) LineData
{
public:
    typedef StepInterval<int>	Segment;

				LineData( int i ) : linenr_(i)	{}

    const int			linenr_;
    TypeSet<Segment>		segments_;

    int				size() const;
    int				segmentOf(int) const;
    Interval<int>		range() const;
    void			merge(const LineData&,bool incl);
				//!< incl=union, !incl=intersection

    int				nearestSegment(double) const;

};


/*!
\brief Position in a CubeData.
*/

mExpClass(Basic) CubeDataPos
{
public:
		CubeDataPos( int iln=0, int isn=0, int sidx=-1 )
		    : lidx_(iln), segnr_(isn), sidx_(sidx)	{}

    int		lidx_;
    int		segnr_;
    int		sidx_;

    void	toPreStart()	{ lidx_ = segnr_ = 0; sidx_ = -1; }
    void	toStart()	{ lidx_ = segnr_ = sidx_ = 0; }
    bool	isValid() const	{ return lidx_>=0 && segnr_>=0 && sidx_>=0; }

};


/*!
\brief Position info for an entire 3D cube.
The LineData's are not sorted.
*/

mExpClass(Basic) CubeData : public ManagedObjectSet<LineData>
{
public:
			CubeData();
			CubeData(const BinID& start,const BinID& stop,
				 const BinID& step);
			CubeData(const TrcKeySampling&);
			CubeData(const CubeData&);
			~CubeData();

    CubeData&		operator =( const CubeData& cd );

    int			totalSize() const;
    int			totalSizeInside(const TrcKeySampling& hrg) const;
			/*!<Only take positions that are inside hrg. */

    virtual int		indexOf(int inl,int* newidx=0) const;
			//!< newidx only filled if not null and -1 is returned
    bool		includes(const BinID&) const;
    bool		includes(int inl,int crl) const;
    void		getRanges(Interval<int>& inl,Interval<int>& crl) const;
    bool		getInlRange(StepInterval<int>&,bool sorted=true) const;
			//!< Returns whether fully regular.
    bool		getCrlRange(StepInterval<int>&,bool sorted=true) const;
			//!< Returns whether fully regular.

    bool		isValid(const CubeDataPos&) const;
    bool		isValid(const BinID&) const;
    bool		isValid(od_int64 globalidx,const TrcKeySampling&) const;

    bool		toNext(CubeDataPos&) const;
    BinID		binID(const CubeDataPos&) const;
    CubeDataPos		cubeDataPos(const BinID&) const;

    bool		haveInlStepInfo() const		{ return size() > 1; }
    bool		haveCrlStepInfo() const;
    bool		isFullyRectAndReg() const;
    bool		isCrlReversed() const;

    void		limitTo(const TrcKeySampling&);
    void		merge(const CubeData&,bool incl);
				//!< incl=union, !incl=intersection
    void		generate(BinID start,BinID stop,BinID step,
				 bool allowreversed=false);

    bool		read(od_istream&,bool asc);
    bool		write(od_ostream&,bool asc) const;

    int			indexOf( const LineData* l ) const override
			{ return ObjectSet<LineData>::indexOf( l ); }

protected:

    void		copyContents(const CubeData&);

};


/*!
\brief Position info for an entire 3D cube.
The LineData's are sorted.
*/

mExpClass(Basic) SortedCubeData : public CubeData
{
public:
			SortedCubeData();
			SortedCubeData(const BinID& start,const BinID& stop,
				       const BinID& step);
			SortedCubeData(const SortedCubeData&);
			SortedCubeData(const CubeData&);
			~SortedCubeData();

    SortedCubeData&	operator =(const SortedCubeData&);
    SortedCubeData&	operator =(const CubeData&);

    int			indexOf(int inl,int* newidx=0) const override;
			//!< newidx only filled if not null and -1 is returned

    SortedCubeData&	add(LineData*);

    int			indexOf( const LineData* l ) const override
			{ return CubeData::indexOf( l ); }

protected:

    CubeData&		doAdd(LineData*) override;

};


/*!
\brief Fills CubeData object. Requires inline- and crossline-sorting.
*/

mExpClass(Basic) CubeDataFiller
{
public:
			CubeDataFiller(CubeData&);
			~CubeDataFiller();

    void		add(const BinID&);
    void		finish(); // automatically called on delete

protected:

    CubeData&		cd_;
    LineData*		ld_;
    LineData::Segment	seg_;
    int			prevcrl;

    void		initLine();
    void		finishLine();
    LineData*		findLine(int);

};


/*!
\brief Iterates through CubeData.
*/

mExpClass(Basic) CubeDataIterator
{
public:

			CubeDataIterator( const CubeData& cd )
			    : cd_(cd)	{}

    inline bool		next( BinID& bid )
			{
			    const bool rv = cd_.toNext( cdp_ );
			    bid = binID(); return rv;
			}
    inline void		reset()		{ cdp_.toPreStart(); }
    inline BinID	binID() const	{ return cd_.binID( cdp_ ); }

    const CubeData&	cd_;
    CubeDataPos		cdp_;

};


/*!
\brief A class to hold a sorted set of Inline, Crossline and Z slices from a
given TrcKeyZSampling. Each slice is defined by its relative position in tkzs_.
*/

mExpClass(Basic) CubeSliceSet
{
			CubeSliceSet(const TrcKeyZSampling&);
    virtual		~CubeSliceSet();

    bool		isEmpty() const;
    void		setEmpty();

    int			nrSlices() const;
    int			nrInlines() const;
    int			nrCrosslines() const;
    int			nrZSlices() const;
    bool		hasInline(int inl) const;
    bool		hasCrossline(int crl) const;
    bool		hasZSlice(float z) const;

    bool		getSliceAtIndex(int index,OD::SliceType,
					TrcKeyZSampling&) const;
    bool		getInline(int inl,TrcKeyZSampling&) const;
    bool		getCrossline(int crl,TrcKeyZSampling&) const;
    bool		getZSlice(float z,TrcKeyZSampling&) const;

    bool		addSlice(const TrcKeyZSampling& tkzs);
			/*<! tkzs should be flat */

    bool		addInline(int inl);
    bool		addCrossline(int crl);
    bool		addZSlice(float z);

    bool		removeInline(int inl);
    bool		removeCrossline(int crl);
    bool		removeZSlice(float z);

protected:

    const TrcKeyZSampling	tkzs_;		// Reference TrcKeyZSampling

			// Sorted
    TypeSet<int>	inlidxs_;
    TypeSet<int>	crlidxs_;
    TypeSet<int>	zidxs_;

    bool		getTKZSForInline(int inl,TrcKeyZSampling&) const;
    bool		getTKZSForCrossline(int crl,TrcKeyZSampling&) const;
    bool		getTKZSForZSlice(float z,TrcKeyZSampling&) const;
};

} // namespace PosInfo
