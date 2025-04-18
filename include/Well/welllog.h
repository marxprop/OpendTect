#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "welldahobj.h"

#include "iopar.h"
#include "mnemonics.h"
#include "ranges.h"
#include "unitofmeasure.h"
#include "wellid.h"

namespace Well
{

/*!
\brief Well log

  No regular sampling required, as in all DahObjs.

  Logs can contain undefined points and sections (washouts etc.) - they will
  never have undefined dah values though. If you can't handle undef values,
  use getValue( dah, true ).

  Log are imported 'as is', making this one of the rare non-SI value
  objects in OpendTect. The unit of measure label you get may be able to
  uncover what the actual unit of measure is.

  The IOPar pars() will be retrieved and stored with each log; it is not
  really used; moreover, it's intended for plugins to dump their extra info
  about this log.
*/

mExpClass(Well) Log : public DahObj
{
public:
			Log(const char* nm=nullptr);
			Log(const Log&);
			~Log();

    Log&		operator =(const Log&);
    Log*		clone() const;

    bool		isLoaded() const;

    float		value( int idx ) const override { return vals_[idx]; }
    void		setValue(int idx,float val);
    void		setValueRange(const Interval<float>&);

    float		getValue(float md,bool noudfs=false) const;
    void		addValue(float md,float val);
			//!< addition must always ascend or descend
    void		updateAfterValueChanges();
			//!< call it upon any change of value(s)
    void		prepareForDisplay();
			//!<call it when plotting this log, preferably on a copy
    void		ensureAscZ();
			// Do this after adding values when Z may be reversed
    bool		insertAtDah(float md,float val) override;
    void		removeTopBottomUdfs();

    mDeprecated("Use setValueRange")
    Interval<float>&	valueRange()			{ return range_; }
    const Interval<float>& valueRange() const		{ return range_; }

    const char*		mnemonicLabel() const;
    const Mnemonic*	mnemonic(bool setifnull=false) const;
    bool		haveMnemonic() const;
    void		setMnemonic(const Mnemonic&);
    void		setMnemonicLabel(const char*,bool setifnull=false);

    const char*		unitMeasLabel() const		{ return unitmeaslbl_;}
    const UnitOfMeasure* unitOfMeasure() const		{ return uom_; }
    bool		haveUnit() const{ return !unitmeaslbl_.isEmpty(); }
    void		setUnitOfMeasure(const UnitOfMeasure*);
    void		setUnitMeasLabel(const char*,bool converttosymbol=true);
    void		convertTo(const UnitOfMeasure*);
    Mnemonic::StdType	propType() const;
    bool		isCode() const			{ return iscode_; }
			//!< log values are all integers stored as floats

    Log*		cleanUdfs() const;
    Log*		upScaleLog(const StepInterval<float>&) const;
    Log*		sampleLog(const StepInterval<float>&) const;
    static Log*		createSampledLog(const StepInterval<float>&,
					 float val=mUdf(float));

    static const char*	sKeyMnemLbl();
    static const char*	sKeyUnitLbl();
    static const char*	sKeyHdrInfo();
    static const char*	sKeyStorage();
    static const char*	sKeyDahRange();
    static const char*	sKeyLogRange();
    static const char*	sKeyDefMnemLbl();

    float*		valArr()			{ return vals_.arr(); }
    const float*	valArr() const			{ return vals_.arr(); }

    IOPar&		pars()				{ return pars_; }
    const IOPar&	pars() const			{ return pars_; }

    void		setID( const LogID& id )	{ id_ = id; }
    const LogID&	ID() const			{ return id_; }

private:

    TypeSet<float>	vals_;
    Interval<float>	range_;
    const Mnemonic*	mn_				= nullptr;
    const UnitOfMeasure* uom_				= nullptr;
    BufferString	mnemlbl_;
    BufferString	unitmeaslbl_;
    bool		iscode_				= false;
    IOPar		pars_;
    LogID		id_;

    void		setMnemonicNullCB(CallBacker*);

    const Mnemonic*	guessMnemonic();

    void		removeAux( int idx ) override
			{ vals_.removeSingle(idx); }

    void		eraseAux() override		{ vals_.erase(); }
    float		gtVal(float,int&) const;

public:

    mDeprecated("Use mnemonicLabel")
    const char*		mnemLabel() const	{ return mnemonicLabel(); }

    mDeprecated("Use setMnemonicLabel")
    void		setMnemLabel( const char* str )
			{ setMnemonicLabel( str ); }

};


/*!\brief Well Log iterator. */

mExpClass(Well) LogIter : public DahObjIter
{
public:
			LogIter(const Log&,bool start_at_end=false);
			~LogIter();

    const Log&		log() const;
};

} // namespace Well
