#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "datapackbase.h"
#include "integerid.h"
#include "seisinfo.h"
#include "trckeyzsampling.h"

class BinIDValueSet;
class DataCharacteristics;
class TraceData;
namespace PosInfo { class CubeData; }


/*!\brief Seis Volume DataPack base class. */

mExpClass(Seis) SeisVolumeDataPack : public VolumeDataPack
{
public:

    bool		isFullyCompat(const ZSampling&,
				      const DataCharacteristics&) const;

			// following will just fill with all available data
    void		fillTrace(const TrcKey&,SeisTrc&) const;
    void		fillTraceInfo(const TrcKey&,SeisTrcInfo&) const;
    void		fillTraceData(const TrcKey&,TraceData&) const;

protected:
			SeisVolumeDataPack(const char* cat,const BinDataDesc*);
			~SeisVolumeDataPack();

};

/*!
\brief SeisVolumeDataPack for 2D and 3D seismic data.
*/

mExpClass(Seis) RegularSeisDataPack : public SeisVolumeDataPack
{
public:
				RegularSeisDataPack(const char* cat,
						   const BinDataDesc* =nullptr);

    bool			is2D() const override;
    bool			isRegular() const override	{ return true; }

    RegularSeisDataPack*	clone() const;
    RegularSeisDataPack*	getSimilar() const;
    bool			copyFrom(const RegularSeisDataPack&);

    void			setSampling( const TrcKeyZSampling& tkzs )
				{ sampling_ = tkzs; }
    const TrcKeyZSampling&	sampling() const
				{ return sampling_; }
    ZSampling			zRange() const override
				{ return sampling_.zsamp_; }

    void			setTrcsSampling(PosInfo::CubeData*);
				//!<Becomes mine
    const PosInfo::CubeData*	getTrcsSampling() const;
				//!<Only for 3D

    bool			addComponent(const char* nm) override;
    bool			addComponentNoInit(const char* nm);

    int				nrTrcs() const override
				{ return (int)sampling_.hsamp_.totalNr(); }
    TrcKey			getTrcKey(int globaltrcidx) const override;
    int				getGlobalIdx(const TrcKey&) const override;

    void			dumpInfo(StringPairSet&) const override;

    static RefMan<RegularSeisDataPack> createDataPackForZSliceRM(
					    const BinIDValueSet*,
					    const TrcKeyZSampling&,
					    const ZDomain::Info&,
					    const BufferStringSet* nms=nullptr);
				/*!< Creates RegularSeisDataPack from
				BinIDValueSet for z-slices in z-axis transformed
				domain. nrComponents() in the created datapack
				will be one less than BinIDValueSet::nrVals(),
				as the	z-component is not used. \param nms is
				for passing component names. */

protected:
				~RegularSeisDataPack();

    TrcKeyZSampling		sampling_;
    PosInfo::CubeData*		rgldpckposinfo_ = nullptr;

public:

    mDeprecated("Use createDataPackForZSliceRM")
    static DataPackID		createDataPackForZSlice(const BinIDValueSet*,
						const TrcKeyZSampling&,
						const ZDomain::Info&,
					    const BufferStringSet* nms=nullptr);
				/*!< Creates RegularSeisDataPack from
				BinIDValueSet for z-slices in z-axis transformed
				domain. nrComponents() in the created datapack
				will be one less than BinIDValueSet::nrVals(),
				as the	z-component is not used. \param nms is
				for passing component names. */

};


/*!
\brief SeisVolumeDataPack for random lines.
*/

mExpClass(Seis) RandomSeisDataPack : public SeisVolumeDataPack
{
public:
				RandomSeisDataPack(const char* cat,
						   const BinDataDesc* =nullptr);

    bool			is2D() const override	{ return false; }
    bool			isRandom() const override	{ return true; }
    int				nrTrcs() const override { return path_.size(); }
    TrcKey			getTrcKey(int trcidx) const override;
    int				getGlobalIdx(const TrcKey&) const override;

    ZSampling			zRange() const override { return zsamp_; }
    void			setZRange( const StepInterval<float>& zrg )
				{ zsamp_ = zrg; }

    void			setPath( const TrcKeySet& path )
				{ path_ = path; }
    const TrcKeySet&		getPath() const		{ return path_; }

    bool			addComponent(const char* nm) override;

    static RefMan<RandomSeisDataPack> createDataPackFromRM(
						    const RegularSeisDataPack&,
						    const RandomLineID&,
						    const Interval<float>& zrg,
						    const BufferStringSet* nms);

    static RefMan<RandomSeisDataPack> createDataPackFromRM(
						const RegularSeisDataPack&,
						const TrcKeySet& path,
						const Interval<float>& zrg,
						const BufferStringSet* nms);
protected:
				~RandomSeisDataPack();

    TrcKeySet			path_;
    ZSampling			zsamp_;

public:

    static DataPackID		createDataPackFrom(const RegularSeisDataPack&,
						   const RandomLineID&,
						   const Interval<float>& zrg);

    static DataPackID		createDataPackFrom(const RegularSeisDataPack&,
					       const RandomLineID&,
					       const Interval<float>& zrg,
					       const BufferStringSet* nms);

    static DataPackID		createDataPackFrom(const RegularSeisDataPack&,
						   const TrcKeySet& path,
						   const Interval<float>& zrg);

    static DataPackID		createDataPackFrom(const RegularSeisDataPack&,
					       const TrcKeySet& path,
					       const Interval<float>& zrg,
					       const BufferStringSet* nms);

    mDeprecated("Use setPath")
    TrcKeySet&			getPath()		{ return path_; }

};


/*!
\brief Base class for RegularSeisFlatDataPack and RandomSeisFlatDataPack.
*/

mExpClass(Seis) SeisFlatDataPack : public FlatDataPack
{
public:

    int				nrTrcs() const;
    DataPackID			getSourceID() const;
    ConstRefMan<SeisVolumeDataPack> getSource() const;
    int				getSourceGlobalIdx(const TrcKey&) const;

    bool			isVertical() const override		= 0;
    bool			is2D() const;
    virtual bool		isRandom() const	{ return false; }
    bool			isStraight() const;

    virtual const TrcKeySet&	getPath() const				= 0;
				//!< Will be empty if isVertical() is false
				//!< Eg: Z-slices. Or if the data corresponds
				//!< to a single trace.
    ZSampling			zRange() const		{ return zsamp_; }

    void			getAltDimKeys(uiStringSet&,
					      bool dim0) const override;
    void			getAltDimKeysUnitLbls(uiStringSet&,bool dim0,
				   bool abbreviated=true,
				   bool withparentheses=true) const override;
    double			getAltDimValue(int ikey,bool dim0,
					       int i0) const override;
    bool			dimValuesInInt(const uiString& keystr,
					       bool dim0) const override;

    const Scaler*		getScaler() const;
    float			nrKBytes() const override;
    RandomLineID		getRandomLineID() const;

protected:

				SeisFlatDataPack(const SeisVolumeDataPack&,
						 int comp,const char* ctgy);
				~SeisFlatDataPack();

    virtual void		setSourceData()				= 0;
    virtual void		setTrcInfoFlds()			= 0;
    void			setPosData();
				/*!< Sets distances from start and Z-values
				 as X1 and X2 posData. Assumes getPath() is
				 not empty. */

    TrcKey			getTrcKey_(int trcidx) const;

    ConstRefMan<SeisVolumeDataPack> source_;
    int				comp_;
    const StepInterval<float>	zsamp_;

    TypeSet<SeisTrcInfo::Fld>	tiflds_;
    RandomLineID		rdlid_;
};


/*!
\brief FlatDataPack for 2D and 3D seismic data.
*/

mExpClass(Seis) RegularSeisFlatDataPack : public SeisFlatDataPack
{
public:
				RegularSeisFlatDataPack(
						const RegularSeisDataPack&,
						int component,
						const char* category=nullptr);

    bool			isVertical() const override
				{ return dir_ != TrcKeyZSampling::Z; }
    const TrcKeySet&		getPath() const override { return path_; }
    float			getPosDistance(bool dim0,
					       float trcfidx) const override;

    const TrcKeyZSampling&	sampling() const	{ return sampling_; }
    TrcKey			getTrcKey(int i0,int i1) const override;
    Coord3			getCoord(int i0,int i1) const override;
    double			getZ(int i0,int i1) const override;

    uiString			dimName(bool dim0) const override;
    uiString			dimUnitLbl(bool dim0,bool display,
					   bool abbreviated=true,
				    bool withparentheses=true) const override;
    const UnitOfMeasure*	dimUnit(bool dim0,bool display) const override;

protected:
				~RegularSeisFlatDataPack();

    void			setSourceDataFromMultiCubes();
    void			setSourceData() override;
    void			setTrcInfoFlds() override;

    TrcKeySet			path_;
    const TrcKeyZSampling&	sampling_;
    TrcKeyZSampling::Dir	dir_;
    bool			usemulticomps_;
    bool			hassingletrace_;
};


/*!
\brief FlatDataPack for random lines.
*/

mExpClass(Seis) RandomSeisFlatDataPack : public SeisFlatDataPack
{
public:
				RandomSeisFlatDataPack(
						const RandomSeisDataPack&,
						int component,
						const char* category=nullptr);

    bool			isVertical() const override	{ return true; }
    bool			isRandom() const override	{ return true; }

    int				getNearestGlobalIdx(const TrcKey&) const;
    const TrcKeySet&		getPath() const override   { return path_; }
    TrcKey			getTrcKey(int i0,int i1) const override;
    Coord3			getCoord(int itrc,int isamp) const override;
    double			getZ(int itrc,int isamp) const override;
    float			getPosDistance(bool dim0,
					       float trcfidx) const override;

    uiString			dimName(bool dim0) const override;
    uiString			dimUnitLbl(bool dim0,bool display,
					   bool abbreviated=true,
				    bool withparentheses=true) const override;
    const UnitOfMeasure*	dimUnit(bool dim0,bool display) const override;

protected:
				~RandomSeisFlatDataPack();

    void			setSourceData() override;
    void			setRegularizedPosData();
				/*!< Sets distances from start and Z-values
				 as X1 and X2 posData after regularizing. */
    void			setTrcInfoFlds() override;

    const TrcKeySet&		path_;
};

// Do not use, only for legacy code
typedef RegularSeisFlatDataPack RegularFlatDataPack;
typedef RandomSeisFlatDataPack RandomFlatDataPack;
