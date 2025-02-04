#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "emposid.h"
#include "trckeyzsampling.h"

class BinIDValue;

namespace Attrib { class SelSpec; }
namespace EM { class EMObject; }

namespace MPE
{

class SectionSourceSelector;
class SectionExtender;
class SectionAdjuster;

/*!
\brief Tracks sections of EM::EMObject with ID EM::SectionID.
*/

mExpClass(MPEEngine) SectionTracker
{
public:
				SectionTracker(EM::EMObject&,
					       SectionSourceSelector* =nullptr,
					       SectionExtender* =nullptr,
					       SectionAdjuster* =nullptr);
    virtual			~SectionTracker();

    virtual bool		init();
    void			reset();

    ConstRefMan<EM::EMObject>	emObject() const;
    RefMan<EM::EMObject>	emObject();

    SectionSourceSelector*	selector();
    const SectionSourceSelector* selector() const;
    SectionExtender*		extender();
    const SectionExtender*	extender() const;
    SectionAdjuster*		adjuster();
    const SectionAdjuster*	adjuster() const;

    virtual bool		select();
    virtual bool		extend();
    virtual bool		adjust();
    const char*			errMsg() const;

    void			useAdjuster(bool yn);
    bool			adjusterUsed() const;

    void			setSetupID(const MultiID& id);
    const MultiID&		setupID() const;
    bool			hasInitializedSetup() const;

    void			setSeedOnlyPropagation(bool yn);
    bool			propagatingFromSeedOnly() const;

    const Attrib::SelSpec&	getDisplaySpec() const;
    void			setDisplaySpec(const Attrib::SelSpec&);

    void			getNeededAttribs(
					TypeSet<Attrib::SelSpec>&) const;
    virtual TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    mDeprecated("Use without SectionID")
				SectionTracker(EM::EMObject& emobj,
				       const EM::SectionID&,
				       SectionSourceSelector* =nullptr,
				       SectionExtender* =nullptr,
				       SectionAdjuster* =nullptr);

    mDeprecatedObs
    EM::SectionID		sectionID() const;

protected:

    void			getLockedSeeds(TypeSet<EM::SubID>& lockedseeds);

    WeakPtr<EM::EMObject>	emobject_;
    EM::SectionID		sid_		= EM::SectionID::def();

    BufferString		errmsg_;
    bool			useadjuster_		= true;
    MultiID			setupid_;
    Attrib::SelSpec&		displayas_;
    bool			seedonlypropagation_	= false;

    SectionSourceSelector*	selector_;
    SectionExtender*		extender_;
    SectionAdjuster*		adjuster_;

    static const char*		trackerstr;
    static const char*		useadjusterstr;
    static const char*		seedonlypropstr;
};

} // namespace MPE
