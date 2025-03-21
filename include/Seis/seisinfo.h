#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "enums.h"
#include "position.h"
#include "ranges.h"
#include "samplingdata.h"
#include "seisposkey.h"

class PosAuxInfo;
class SeisTrc;
class UnitOfMeasure;


/*!\brief Information for a seismic trace, AKA trace header info */

mExpClass(Seis) SeisTrcInfo
{
public:

    typedef IdxPair::IdxType	IdxType;

			SeisTrcInfo();
			~SeisTrcInfo();
			SeisTrcInfo(const SeisTrcInfo&);
    SeisTrcInfo&	operator =(const SeisTrcInfo&);

    Coord		coord_;
    SamplingData<float> sampling_;
    float		offset_		= 0.f;
    float		azimuth_	= 0.f;
    float		refnr_		= mUdf(float);
    float		pick_		= mUdf(float);
    int			seqnr_		= 0;

    bool		is2D() const;
    bool		is3D() const;
    bool		isSynthetic() const;
    OD::GeomSystem	geomSystem() const;
    BinID		binID() const;
    Pos::IdxPair	idxPair() const;
    IdxType		inl() const;
    IdxType		crl() const;
    IdxType		lineNr() const;
    IdxType		trcNr() const;
    Pos::GeomID		geomID() const;
    const TrcKey&	trcKey() const		{ return trckey_; }

    SeisTrcInfo&	setGeomSystem(OD::GeomSystem);
    SeisTrcInfo&	setPos(const BinID&); //3D
    SeisTrcInfo&	setPos(Pos::GeomID,IdxType); //2D
    SeisTrcInfo&	setGeomID(Pos::GeomID);
    SeisTrcInfo&	setLineNr(IdxType);
    SeisTrcInfo&	setTrcNr(IdxType);
    SeisTrcInfo&	setTrcKey(const TrcKey&);
    SeisTrcInfo&	calcCoord(); //!< from current TrcKey position

    int			nearestSample(float pos) const;
    float		samplePos( int idx ) const
    { return sampling_.atIndex( idx ); }
    SampleGate		sampleGate(const Interval<float>&) const;
    bool		dataPresent(float pos,int trcsize) const;

    enum Fld		{ TrcNr=0, Pick, RefNr,
			  CoordX, CoordY, BinIDInl, BinIDCrl,
			  Offset, Azimuth, SeqNr, GeomID };
			mDeclareEnumUtils(Fld)
    static uiString	getUnitLbl(const Fld&,bool display,bool abbrevated=true,
				   bool withparentheses=true);
    static const UnitOfMeasure* getUnit(const Fld&,bool display);
    double		getValue(const Fld&) const;
    static void		getAxisCandidates(Seis::GeomType,TypeSet<Fld>&);
    Fld			getDefaultAxisFld(Seis::GeomType,
					  const SeisTrcInfo* next,
					  const SeisTrcInfo* last) const;
    void		setPSFlds(const Coord& rcvpos,const Coord& srcpos,
				  bool setpos=false);

    static const char*	sSamplingInfo();
    static const char*	sNrSamples();
    static float	defaultSampleInterval(bool forcetime=false);

    Seis::PosKey	posKey(Seis::GeomType) const;
    void		setPosKey(const Seis::PosKey&);
    void		putTo(PosAuxInfo&) const;
    void		getFrom(const PosAuxInfo&);
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    float		zref_		= 0.f; // not stored
    bool		new_packet_	= false; // not stored

private:

    TrcKey&		trckey_;

public:

    mDeprecated("Use setPos")
    SeisTrcInfo&	setBinID( const BinID& bid )
						{ return setPos(bid); }
    mDeprecated("Use setLineNr")
    SeisTrcInfo&	setInl( IdxType inl )	{ return setLineNr(inl); }
    mDeprecated("Use setTrcNr")
    SeisTrcInfo&	setCrl( IdxType crl )	{ return setTrcNr(crl); }

    mDeprecated("Use coord_")
    Coord&		coord;
    mDeprecated("Use sampling_")
    SamplingData<float>& sampling;
    mDeprecated("Use offset_")
    float&		offset;
    mDeprecated("Use azimuth_")
    float&		azimuth;
    mDeprecated("Use refnr_")
    float&		refnr;
    mDeprecated("Use pick_")
    float&		pick;
    mDeprecated("Use zref_")
    float&		zref;
    mDeprecated("Use new_packet_")
    bool&		new_packet;

    mDeprecated("Use BinID or setPos")
    BinID&		binid;
    mDeprecated("Use seqnr_ or setTrcNr")
    int&		nr;

};
