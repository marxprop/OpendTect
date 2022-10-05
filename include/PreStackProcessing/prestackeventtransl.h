#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "transl.h"

class Executor;
class IOObj;
class BinIDValueSet;
class TrcKeySampling;

namespace PreStack { class EventManager; }

/*!
\brief TranslatorGroup for PreStack Event.
*/

mExpClass(PreStackProcessing) PSEventTranslatorGroup : public TranslatorGroup
{ isTranslatorGroup(PSEvent);
public:
				PSEventTranslatorGroup();

    const char*			defExtension() const override
				{ return sDefExtension(); }
    static const char*		sDefExtension()	     { return "psevent"; }
};


/*!
\brief Translator for PreStack Event.
*/

mExpClass(PreStackProcessing) PSEventTranslator : public Translator
{
public:
			~PSEventTranslator();

    virtual Executor*	createReader(PreStack::EventManager&,
				     const BinIDValueSet*,
				     const TrcKeySampling*,IOObj*,
				     bool trigger)	= 0;
    virtual Executor*	createWriter(PreStack::EventManager&,IOObj*) = 0;
    virtual Executor*	createSaveAs(PreStack::EventManager&,IOObj*)	= 0;
    virtual Executor*	createOptimizer(IOObj*)				= 0;

    static Executor*	reader(PreStack::EventManager&, const BinIDValueSet*,
			       const TrcKeySampling*, IOObj*, bool trigger );
    static Executor*	writer(PreStack::EventManager&,IOObj*);
    static Executor*	writeAs(PreStack::EventManager&,IOObj*);

protected:
			PSEventTranslator(const char* nm,const char* unm);
};


/*!
\brief dgb PSEventTranslator.
*/

mExpClass(PreStackProcessing) dgbPSEventTranslator : public PSEventTranslator
{ isTranslator(dgb,PSEvent)
public:
			dgbPSEventTranslator(const char* nm,const char* unm);

    Executor*		createReader(PreStack::EventManager&,
				 const BinIDValueSet*,
				 const TrcKeySampling*,IOObj*,bool) override;
    Executor*		createWriter(PreStack::EventManager&,IOObj*) override;
    Executor*		createSaveAs(PreStack::EventManager&,IOObj*) override;
    Executor*		createOptimizer(IOObj*) override { return 0; }

};
