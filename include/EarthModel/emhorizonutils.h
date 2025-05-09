#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "emsurfaceiodata.h"
#include "emposid.h"
#include "multiid.h"
#include "ranges.h"

class RowCol;
class od_ostream;
class BinIDValueSet;
class DataPointSet;
class TrcKeySampling;
namespace Pos { class Provider; }

namespace EM
{

class Horizon;

/*!
\brief Group of utilities for horizons: here are all functions required in
od_process_attrib_em for computing data on, along or between 2 horizons.
*/

mExpClass(EarthModel) HorizonUtils final
{
public:

    static float	getZ(const RowCol&,const Horizon*);
    static float	getMissingZ(const RowCol&,const Horizon*,int);
    static Horizon*	getHorizon(const MultiID&);
    static void	getPositions(od_ostream&,const MultiID&,
				     ObjectSet<BinIDValueSet>&);
    static void	getExactCoords(od_ostream&,const MultiID&,
				       const Pos::GeomID&,const TrcKeySampling&,
				       ObjectSet<DataPointSet>&);
    static void	getWantedPositions(od_ostream&,ObjectSet<MultiID>&,
					   BinIDValueSet&,const TrcKeySampling&,
					   const Interval<float>& extraz,
					   int nrinterpsamp,int mainhoridx,
					   float extrawidth,
					   Pos::Provider* provider=0);
    static void	getWantedPos2D(od_ostream&,ObjectSet<MultiID>&,
				       DataPointSet*,const TrcKeySampling&,
				       const Interval<float>& extraz,
				       const Pos::GeomID&);
    static bool		getZInterval(int idi,int idc,Horizon*,Horizon*,
				     float& topz,float& botz,int nrinterpsamp,
				     int mainhoridx,float& lastzinterval,
				     float extrawidth);

    static bool		SolveIntersect(float& topz,float& botz,int nrinterpsamp,
				       int is1main,float extrawidth,
				       bool is1interp,bool is2interp);
    static void		addHorizonData(const MultiID&,const BufferStringSet&,
				       const ObjectSet<BinIDValueSet>&);

protected:

};


mExpClass(EarthModel) HorizonSelInfo
{
public:
			HorizonSelInfo(const MultiID&);
    virtual		~HorizonSelInfo();

    BufferString	name_;
    MultiID		key_;
    ObjectID		emobjid_;
    EM::SurfaceIOData	iodata_;

    static void		getAll(ObjectSet<HorizonSelInfo>&,bool is2d);
};

} // namespace EM
