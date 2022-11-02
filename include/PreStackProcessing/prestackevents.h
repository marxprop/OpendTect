#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"
#include "sharedobject.h"

#include "bufstringset.h"
#include "color.h"
#include "multidimstorage.h"
#include "multiid.h"
#include "offsetazimuth.h"
#include "position.h"
#include "threadlock.h"
#include "undo.h"
#include "valseriesevent.h"

class BinIDValueSet;
class Executor;
class OffsetAzimuth;
class SeisTrcReader;
class TrcKeySampling;

namespace EM { class Horizon3D; }

namespace PreStack
{

class CDPGeometrySet;
class GatherEvents;
class VelocityPicks;

/*!
\brief A Event is a set of picks on an event on a single PreStack gather.
*/

mExpClass(PreStackProcessing) Event
{
public:
			Event(int sz,bool quality);
			Event(const Event&);
			~Event();

    Event&		operator=(const Event&);
    void		setSize(int sz,bool quality);
    void		removePick(int);
    void		addPick();
    void		insertPick(int);
    int			indexOf(const OffsetAzimuth&) const;

    int			sz_;
    float*		pick_ = nullptr;
    OffsetAzimuth*	offsetazimuth_ = nullptr;

    static unsigned char cBestQuality()		{ return 254; }
    static unsigned char cManPickQuality()	{ return 255; }
    static unsigned char cDefaultQuality()	{ return 0; }
    unsigned char*	pickquality_ = 0;
			//255	= manually picked
			//0-254 = tracked

    unsigned char	quality_ = 255;
    short		horid_ = -1;
    VSEvent::Type	eventtype_ = VSEvent::None;
};


/*!
\brief A EventSet is a set of Events on a single PreStack gather.
*/

mExpClass(PreStackProcessing) EventSet : public ReferencedObject
{
public:
			EventSet();
			EventSet(const EventSet&);

    EventSet&		operator=(const EventSet&);

    int			indexOf(int horid) const;

    ObjectSet<Event>	events_;
    bool		ischanged_ = false;

protected:
    virtual		~EventSet();
};


/*!
\brief A EventManager is a set of EventsSet on multiple PreStack gathers, and
are identified under the same MultiID.
*/

mExpClass(PreStackProcessing) EventManager : public SharedObject
{
public:
    mStruct(PreStackProcessing) DipSource
    {
			DipSource();
			~DipSource();

	enum Type	{ None, Horizon, SteeringVolume };
			mDeclareEnumUtils(Type);

	bool		operator==(const DipSource&) const;

	Type		type_ = None;
	MultiID		mid_;

	void		fill(BufferString&) const;
	bool		use(const char*);
    };
				EventManager();

    const TypeSet<int>&		getHorizonIDs() const { return horids_; }
    int				addHorizon(int id=-1);
				//!<\returns horid
				//!<\note id argument is only for internal use
				//!<	  i.e. the reader
    bool			removeHorizon(int id);
    const MultiID&		horizonEMReference(int id) const;
    void			setHorizonEMReference(int id,const MultiID&);
    int				nextHorizonID(bool usethis);
    void			setNextHorizonID(int);

    const OD::Color&		getColor() const	{ return color_; }
    void			setColor(const OD::Color&);

    void			setDipSource(const DipSource&,bool primary);
    const DipSource&		getDipSource(bool primary) const;

    Executor*			setStorageID(const MultiID& mid,
					     bool reload );
				/*!<Sets the storage id.
				  \param mid Database Object key
				  \param reload if true,
					all data will be removed, forceReload
					will trigger, and data at the reload
					positions will be loaded from the new
					data.
				   \note Note that no data on disk is
				    changed/duplicated. That can be done with
				    the translator. */
    const MultiID&		getStorageID() const;

    bool			getHorRanges(TrcKeySampling&) const;
    bool			getLocations(BinIDValueSet&) const;

    Undo&			undo()		{ return undo_; }
    const Undo&			undo() const	{ return undo_; }


    Executor*			commitChanges();
    Executor*			load(const BinIDValueSet&,bool trigger);

    bool			isChanged() const;
    void			resetChangedFlag(bool onlyhorflag);
    Notifier<EventManager>	resetChangeStatus;
				//!<Triggers when the chang flags are reseted

    EventSet*			getEvents(const BinID&,bool load,bool create);
    const EventSet*		getEvents(const BinID&,
				    bool load=false,bool create=false) const;

    void			cleanUp(bool keepchanged);

    MultiDimStorage<EventSet*>& getStorage() { return events_; }

    Notifier<EventManager>	change;
				/*!<\note Dont enable/disable,
					  use blockChange if needed. */
    const BinID&		changeBid() const  { return changebid_;}
				/*!<Can be -1 if general
				   (name/color/horid) change. */
    void			blockChange(bool yn,bool sendall);
				/*!<Turns off notification, but
				    class will record changes. If
				    sendall is on when turning off the
				    block, all recorded changes will
				    be triggered. */

    Notifier<EventManager>	forceReload;
				/*!<When triggered, all
				    EventSets must be
				    unreffed. Eventual load requirements
				    should be added. */
    void			addReloadPositions(
						const BinIDValueSet&);
    void			addReloadPosition(const BinID&);

    void			reportChange(const BinID& bid=BinID::udf());

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);


    bool			getDip(const BinIDValue&,int horid,
				       float& inldip, float& crldip );

protected:
    virtual			~EventManager();

    static const char*		sKeyStorageID() { return "PS Picks"; }
    bool			getDip(const BinIDValue&,int horid,bool primary,
				       float& inldip,float& crldip);

    MultiDimStorage<EventSet*>	events_;
    Threads::Lock		eventlock_;
    MultiID			storageid_;
    VelocityPicks*		velpicks_;

    OD::Color			color_;

    TypeSet<int>		horids_;
    TypeSet<MultiID>		horrefs_;
    ObjectSet<EM::Horizon3D>	emhorizons_;

    BinID			changebid_;
    Threads::Lock		changebidlock_;
    BinIDValueSet*		notificationqueue_;
    BinIDValueSet*		reloadbids_;

    int				nexthorid_ = 0;
    bool			auxdatachanged_ = false;

    DipSource			primarydipsource_;
    DipSource			secondarydipsource_;

    SeisTrcReader*		primarydipreader_ = nullptr;
    SeisTrcReader*		secondarydipreader_ = nullptr;

    Undo			undo_;
};


/*!
\brief BinIDUndoEvent for PreStack pick.
*/

mExpClass(PreStackProcessing) SetPickUndo : public BinIDUndoEvent
{
public:
			SetPickUndo(EventManager&,const BinID&,int horidx,
				    const OffsetAzimuth&,float depth,
				    unsigned char pickquality);
			~SetPickUndo();

    const char*		getStandardDesc() const override
			{ return "prestack pick"; }

    const BinID&	getBinID() const override	{ return bid_; }

    bool		unDo() override;
    bool		reDo() override;

protected:

    bool		doWork(float,unsigned char);

    EventManager&	manager_;
    const BinID		bid_;
    const int		horidx_;
    const OffsetAzimuth oa_;
    const float		olddepth_;
    const unsigned char oldquality_;
    float		newdepth_;
    unsigned char	newquality_;
};


/*!
\brief UndoEvent for PreStack pick.
*/

mExpClass(PreStackProcessing) SetEventUndo : public UndoEvent
{
public:
			SetEventUndo(EventManager&,const BinID&,int horidx,
				    short horid,VSEvent::Type,
				    unsigned char pickquality);
			SetEventUndo(EventManager&,const BinID&,int horidx);
			~SetEventUndo();

    const char*		getStandardDesc() const override
			{ return "prestack pick"; }

    const BinID&	getBinID() const	{ return bid_; }

    bool		unDo() override;
    bool		reDo() override;

protected:

    bool		addEvent();
    bool		removeEvent();

    EventManager&	manager_;
    const BinID		bid_;
    const int		horidx_;

    unsigned char	quality_;
    short		horid_;
    VSEvent::Type	eventtype_;

    bool		isremove_;
};

} // namespace PreStack
