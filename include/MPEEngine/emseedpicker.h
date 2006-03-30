#ifndef emseedpicker_h
#define emseedpicker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: emseedpicker.h,v 1.16 2006-03-30 15:53:32 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emtracker.h"
#include "position.h"
#include "sets.h"

namespace MPE
{

/*!
handles adding of seeds and retracking of events based on new seeds.

An instance of the class is usually avaiable from the each EMTracker.
*/

class EMSeedPicker
{
public:
    virtual		~EMSeedPicker() {}


    virtual bool	canSetSectionID() const			{ return false;}
    virtual bool	setSectionID(const EM::SectionID&)	{ return false;}
				
    virtual EM::SectionID getSectionID() const			{ return -1; }

    virtual bool	startSeedPick()				{ return false;}
    			/*!<Should be set when seedpicking
			    is about to start. */

    virtual bool	canAddSeed() const			{ return false;}
    virtual bool	addSeed(const Coord3&)			{ return false;}
    virtual bool	canRemoveSeed() const			{ return false;}
    virtual bool	removeSeed(const EM::PosID&,
	    			   bool retrack=true)		{ return false;}
    virtual bool	reTrack()				{ return false;}
    virtual int		nrSeeds() const				{ return 0; }

    virtual int		isMinimumNrOfSeeds() const		{ return 1; }
    			/*<!\returns the number of seeds that
				the user has to add before we
				can use the results. */
    virtual bool	stopSeedPick(bool iscancel=false)	{ return true; }

    virtual void	setSeedConnectMode(int)			{ return; }
    virtual int		getSeedConnectMode() const		{ return -1; }
    virtual void	blockSeedPick(bool)			{ return; }
    virtual bool	isSeedPickBlocked() const		{ return false;}
    virtual bool        doesModeUseVolume() const		{ return true; }
    virtual bool	doesModeUseSetup() const		{ return true;}

    virtual const char*	errMsg() const				{ return 0; }

};


};

#endif
