#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          March 2018
________________________________________________________________________

-*/

#include "hdf5access.h"


namespace HDF5
{

mExpClass(General) Reader : public Access
{
public:

    virtual void		getGroups(BufferStringSet&) const	= 0;
				//!< All groups with full names (recursive)
    virtual void		getDataSets(const char*,
					    BufferStringSet&) const	= 0;
				//!< Pass a full group name
    virtual ArrayNDInfo*	getDataSizes(const DataSetKey&) const	= 0;
				//!< Can return null when:
				//!< - there is no such group
				//!< - the group has an empty dataset

};

} // namespace HDF5
