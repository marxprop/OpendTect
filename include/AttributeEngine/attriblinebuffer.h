#ifndef attribbuffer_h
#define attribbuffer_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attriblinebuffer.h,v 1.6 2009-01-05 09:49:43 cvsranojay Exp $
________________________________________________________________________

-*/

#include "sets.h"

class BinID;


namespace Attrib
{

class DataHolder;

mClass DataHolderLineBuffer
{
public:
    			DataHolderLineBuffer();
			~DataHolderLineBuffer();

    DataHolder*		createDataHolder( const BinID&, int t0, int nrsamples );
    void		removeDataHolder(const BinID&);
    void		removeBefore( const BinID&, const BinID& );
    void		removeAllExcept( const BinID& );

    DataHolder*		getDataHolder( const BinID& b )
			{ return gtDataHolder(b); }
    const DataHolder*	getDataHolder( const BinID& b ) const
			{ return gtDataHolder(b); }

protected:
    void		removeInline( int lineidx );

    TypeSet<int>			inlines;	
    ObjectSet<ObjectSet<DataHolder> >	inlinedata;
    ObjectSet<TypeSet<int> >		crossliness;
    DataHolder*		gtDataHolder(const BinID&) const;
};

}; //Namespace


#endif
