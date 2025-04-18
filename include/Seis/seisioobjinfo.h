#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "bufstring.h"
#include "datachar.h"
#include "datadistribution.h"
#include "file.h"
#include "ioobj.h"
#include "odcommonenums.h"
#include "seistype.h"
#include "survgeom.h"


class BinIDValueSet;
class BufferStringSet;
class FilePath;
class SeisIOObjInfo;
class SeisTrcTranslator;
class SurveyChanger;
class TrcKeyZSampling;
class UnitOfMeasure;

namespace ZDomain { class Def; class Info; }

/*!\brief Summary for a Seismic object */

namespace Seis {

mExpClass(Seis) ObjectSummary
{
public:
			ObjectSummary(const MultiID&);
			ObjectSummary(const DBKey&);
			ObjectSummary(const IOObj&);
			ObjectSummary(const IOObj&,const Pos::GeomID&);
			ObjectSummary(const ObjectSummary&);
			~ObjectSummary();

    ObjectSummary&	operator =(const ObjectSummary&);

    inline bool		isOK() const	{ return !bad_; }
    inline bool		is2D() const	{ return Seis::is2D(geomtype_); }
    inline bool		isPS() const	{ return Seis::isPS(geomtype_); }

    bool		hasSameFormatAs(const BinDataDesc&) const;
    inline const DataCharacteristics	getDataChar() const
					{ return datachar_; }
    inline GeomType	geomType() const	{ return geomtype_; }
    const StepInterval<float>&	zRange() const		{ return zsamp_; }

    const SeisIOObjInfo&	getFullInformation() const
				{ return ioobjinfo_; }

protected:

    const SeisIOObjInfo&	ioobjinfo_;

    DataCharacteristics datachar_;
    StepInterval<float> zsamp_;
    GeomType		geomtype_;
    BufferStringSet	compnms_;

    //Cached
    bool		bad_;
    int			nrcomp_;
    int			nrsamppertrc_;
    int			nrbytespersamp_;
    int			nrdatabytespespercomptrc_;
    int			nrdatabytespertrc_;
    int			nrbytestrcheader_;
    int			nrbytespertrc_;

private:

    void		init();
    void		init2D(const Pos::GeomID&);
    void		refreshCache(const SeisTrcTranslator&);

    friend class RawTrcsSequence;

};

} // namespace Seis

/*!\brief Info on IOObj for seismics */

mExpClass(Seis) SeisIOObjInfo
{ mODTextTranslationClass(SeisIOObjInfo)
public:
			SeisIOObjInfo(const IOObj*);
			SeisIOObjInfo(const IOObj&);
			SeisIOObjInfo(const MultiID&);
			SeisIOObjInfo(const DBKey&);
			SeisIOObjInfo(const char* ioobjnm,Seis::GeomType);
			SeisIOObjInfo(const SeisIOObjInfo&);
			~SeisIOObjInfo();

    SeisIOObjInfo&	operator =(const SeisIOObjInfo&);

    inline bool		is2D() const	{ return geomtype_ > Seis::VolPS; }
    inline bool		isPS() const	{ return geomtype_ == Seis::VolPS
					      || geomtype_ == Seis::LinePS; }

    bool		isOK(bool createtr=false) const;
    Seis::GeomType	geomType() const	{ return geomtype_; }
    const IOObj*	ioObj() const		{ return ioobj_; }
    const ZDomain::Info& zDomain() const;
    const ZDomain::Def& zDomainDef() const;
    bool		isTime() const;
    bool		isDepth() const;
    bool		zInMeter() const;
    bool		zInFeet() const;
    const UnitOfMeasure* zUnit() const;
    ZSampling		getConvertedZrg(const ZSampling&) const;
			/*!\ If the dataset zunit is not the project zdomain
			     unit, convert zsamp to the project zdomain unit
			     Does not convert accross domains (Time/Depth)  */

    const UnitOfMeasure* offsetUnit() const;
			//<! Pre-Stack only
    Seis::OffsetType	offsetType() const;
			//<! Pre-Stack only
    bool		isCorrected() const;
			//<! Pre-Stack only

    mStruct(Seis) SpaceInfo
    {
			SpaceInfo(int ns=-1,int ntr=-1,int bps=4);
	mDeprecated("Use expectedSize")
	int		expectedMBs() const;
	od_int64	expectedSize() const;

	int		expectednrsamps;
	int		expectednrtrcs;
	int		maxbytespsamp;
    };

    bool		getDefSpaceInfo(SpaceInfo&) const;

    od_int64		expectedSize(const SpaceInfo&) const;
    od_int64		getFileSize() const;
    int			nrImpls() const;
    void		getAllFileNames(BufferStringSet&,
					bool forui=false) const;
    bool		getRanges(TrcKeyZSampling&) const;
    bool		isFullyRectAndRegular() const; // Only CBVS
    bool		getDataChar(DataCharacteristics&) const;
    bool		getBPS(int&,int icomp) const;
			//!< max bytes per sample, component -1 => add all

    int			nrComponents(const Pos::GeomID&
						=Pos::GeomID::udf()) const;
    void		getComponentNames(BufferStringSet&,
					  const Pos::GeomID&
						=Pos::GeomID::udf()) const;
    bool		getDisplayPars(IOPar&) const;

    bool		haveAux(const char* ext) const;
    bool		getAux(const char* ext,const char* ftyp,IOPar&) const;
    bool		havePars() const;
    bool		getPars(IOPar&) const;
    bool		haveStats() const;
    bool		getStats(IOPar&) const;
    bool		isAvailableIn(const TrcKeySampling&) const;

    RefMan<FloatDistrib> getDataDistribution(int icomp=-1) const;
			//this may take some time, use uiUserShowWait or alike
    Interval<float>	getDataRange(int icomp=-1) const;

    mStruct(Seis) Opts2D
    {
				Opts2D()
				    : steerpol_(2)	{}
	BufferString		zdomky_;	//!< default=empty=only SI()'s
				//!< Will be matched as GlobExpr
	int			steerpol_;	//!< 0=none, 1=only, 2=both
				//!< Casts into uiSeisSel::Setup::SteerPol
    };

    // 2D only
    void		getGeomIDs(TypeSet<Pos::GeomID>&) const;
    void		getLineNames( BufferStringSet& b,
				      Opts2D o2d=Opts2D() ) const
				{ getNms(b,o2d); }
    bool		getRanges(const Pos::GeomID& geomid,
				  StepInterval<int>& trcrg,
				  StepInterval<float>& zrg) const;

    static void		initDefault(const char* type=0);
			//!< Only does something if there is not yet a default
    static const MultiID& getDefault(const char* type=0);
    static void		setDefault(const MultiID&,const char* type=0);

    static bool		hasData(const Pos::GeomID&);
    static void		getDataSetNamesForLine(const Pos::GeomID& geomid,
					       BufferStringSet&,
					       const Opts2D& o2d={});
    static void		getDataSetIDsForLine(const Pos::GeomID& geomid,
					     TypeSet<MultiID>&,
					     const Opts2D& o2d={});
    static void		getDataSetInfoForLine(const Pos::GeomID& geomid,
					      TypeSet<MultiID>&,
					      BufferStringSet& datasetnames,
					      const Opts2D& o2d={});
    static void		getCompNames(const MultiID&,BufferStringSet&);
			//!< Function useful in attribute environments
			//!< The 'MultiID' must be IOObj_ID
    static void		getLinesWithData(BufferStringSet& lnms,
					 TypeSet<Pos::GeomID>& gids);
    static bool		isCompatibleType(const char* omftypestr1,
					 const char* omftypestr2);

    void		getUserInfo(uiStringSet&) const;
    uiString		errMsg() const			{ return errmsg_; }
    IOObj::Status	objStatus() const		{ return objstatus_; }


    mDeprecatedDef	SeisIOObjInfo(const char* ioobjnm);
    mDeprecated("Use getDataSetNamesForLine with Pos::GeomID")
    static void		getDataSetNamesForLine(const char* linenm,
					       BufferStringSet&,
					       Opts2D o2d={});

protected:

    Seis::GeomType		geomtype_;
    IOObj*			ioobj_;
    SurveyChanger*		surveychanger_	    = nullptr;
    mutable uiString		errmsg_;
    mutable IOObj::Status	objstatus_	    = IOObj::Status::Unknown;

    void		setType();

    void		getNms(BufferStringSet&,const Opts2D&) const;
    int			getComponentInfo(const Pos::GeomID&,
					 BufferStringSet*) const;

    void		getCommonUserInfo(uiStringSet&) const;
    void		getPostStackUserInfo(uiStringSet&) const;
    void		getPreStackUserInfo(uiStringSet&) const;
    bool		checkAndInitTranslRead(SeisTrcTranslator*,
					Seis::ReadMode rt=Seis::Prod) const;

public:
    mDeprecated("Use expectedSize")
    int			expectedMBs(const SpaceInfo&) const;
};
