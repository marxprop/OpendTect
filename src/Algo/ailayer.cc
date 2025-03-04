/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ailayer.h"

#include "arrayndimpl.h"
#include "math2.h"
#include "mathfunc.h"
#include "ranges.h"
#include "zvalseriesimpl.h"


#define mIsValidThickness(val) \
( !mIsUdf(val) && validThicknessRange().includes(val,false) )
#define mIsValidDen(val) ( validDensityRange().includes(val,false) )
#define mIsValidVel(val) ( validVelocityRange().includes(val,false) )
#define mIsValidImp(val) ( validImpRange().includes(val,false) )


// RefLayer

mDefineEnumUtils(RefLayer,Type,"Layer Type")
{
    "Acoustic Layer",
    "Elastic Layer",
    "VTI Layer",
    "HTI Layer",
    nullptr
};


RefLayer* RefLayer::create( Type typ )
{
    if ( typ == Acoustic )
	return new AILayer( mUdf(float), mUdf(float), mUdf(float) );
    if ( typ == Elastic )
	return new ElasticLayer( mUdf(float), mUdf(float),
				 mUdf(float), mUdf(float) );
    if ( typ == VTI )
	return new VTILayer( mUdf(float), mUdf(float), mUdf(float),
			     mUdf(float), mUdf(float) );
    if ( typ == HTI )
	return new HTILayer( mUdf(float), mUdf(float), mUdf(float),
			     mUdf(float), mUdf(float), mUdf(float) );

    return nullptr;
}


RefLayer* RefLayer::clone( const RefLayer& layer, const Type* reqtyp )
{
    if ( !reqtyp || (reqtyp && layer.getType() == *reqtyp) )
	return layer.clone();

    auto* ret = create( reqtyp ? *reqtyp : layer.getType() );
    *ret = layer;
    if ( ret->isElastic() )
	ret->asElastic()->fillVsWithVp( true );
    if ( ret->isVTI() && !ret->isValidFracRho() )
	ret->setFracRho( 0.f );
    if ( ret->isHTI() && !ret->isValidFracAzi() )
	ret->setFracAzi( 0.f );

    return ret;
}


RefLayer::RefLayer()
{
}


RefLayer::~RefLayer()
{
}


RefLayer::Type RefLayer::getType( bool needswave, bool needfracrho,
				  bool needfracazi )
{
    if ( needfracazi )
	return HTI;
    if ( needfracrho )
	return VTI;
    if ( needswave )
	return Elastic;
    return Acoustic;
}


RefLayer& RefLayer::operator =( const RefLayer& oth )
{
    if ( &oth == this )
	return *this;

    copyFrom( oth );
    return *this;
}


bool RefLayer::operator ==( const RefLayer& oth ) const
{
    if ( &oth == this )
	return true;

    if ( getThickness() != oth.getThickness() || getPVel() != oth.getPVel() ||
	 getDen() != oth.getDen() )
	return false;

    if ( isElastic() != oth.isElastic() ||
	(isElastic() && getSVel() != oth.getSVel()) )
	return false;

    if ( isVTI() != oth.isVTI() ||
	(isVTI() && getFracRho() != oth.getFracRho()) )
	return false;

    if ( isHTI() != oth.isHTI() ||
	(isHTI() && getFracAzi() != oth.getFracAzi()) )
	return false;

    return true;
}


bool RefLayer::operator !=( const RefLayer& oth ) const
{
    return !(*this == oth);
}


bool RefLayer::isOK( bool dodencheck, bool dosvelcheck,
		     bool dofracrhocheck, bool dofracazicheck ) const
{
    if ( !isValidThickness() || !isValidVel() )
	return false;

    if ( dodencheck && !isValidDen() )
	return false;

    if ( isElastic() && dosvelcheck && !isValidVs() )
	return false;

    if ( isVTI() && dofracrhocheck && !isValidFracRho() )
	return false;

    if ( isHTI() && dofracazicheck && !isValidFracAzi() )
	return false;

    return true;
}


// AILayer

AILayer::AILayer( float thkness, float vel, float den )
    : RefLayer()
    , thickness_(thkness)
    , den_(den)
    , vel_(vel)
{
}


AILayer::AILayer( float thkness, float ai, float den, bool needcompthkness )
    : RefLayer()
    , thickness_(thkness)
    , den_(den)
{
    const bool hasdensity = isValidDen();
    //compute vel_ using Gardner's equation vel = (den/a)^(1/(1+b))
    //with default values for C0 and C1 respectively 310 and 0.25
    setPVel( hasdensity ? ai / den : Math::PowerOf( ai/310.f, 0.8f ) );
    if ( !hasdensity )
	setDen( ai/vel_ );

    if ( needcompthkness )
	thickness_ *= vel_ / 2.0f;
}


AILayer::AILayer( const RefLayer& oth )
    : RefLayer()
{
    AILayer::copyFrom( oth );
}


AILayer::AILayer( const AILayer& oth )
    : RefLayer()
{
    AILayer::copyFrom( oth );
}


AILayer::~AILayer()
{
}


RefLayer* AILayer::clone() const
{
    return new AILayer( *this );
}


RefLayer& AILayer::operator =( const AILayer& oth )
{
    return RefLayer::operator=( oth );
}


void AILayer::copyFrom( const RefLayer& oth )
{
    setThickness( oth.getThickness() );
    setDen( oth.getDen() );
    setPVel( oth.getPVel() );
}


float AILayer::getAI() const
{
    return isValidDen() && isValidVel() ? den_ * vel_ : mUdf(float);
}


RefLayer& AILayer::setThickness( float thickness )
{
    thickness_ = thickness;
    return *this;
}


RefLayer& AILayer::setPVel( float vel )
{
    vel_ = vel;
    return *this;
}


RefLayer& AILayer::setDen( float den )
{
    den_ = den;
    return *this;
}


bool AILayer::fillDenWithVp( bool onlyinvalid )
{
    if ( onlyinvalid && isValidDen() )
	return true;

    if ( !isValidVel() )
	return false;

    setDen( mCast( float, 310. * Math::PowerOf( (double)vel_, 0.25 ) ) );
    return isValidDen();
}


bool AILayer::isValidThickness() const
{
    return mIsValidThickness(thickness_);
}


bool AILayer::isValidVel() const
{
    return mIsValidVel(vel_);
}


bool AILayer::isValidDen() const
{
    return mIsValidDen(den_);
}


// ElasticLayer

ElasticLayer::ElasticLayer( float thkness, float pvel, float svel, float den )
    : AILayer(thkness,pvel,den)
    , svel_(svel)
{
}


ElasticLayer::ElasticLayer( float thkness, float ai, float si, float den,
			    bool needcompthkness )
    : AILayer(thkness,ai,den,needcompthkness)
{
    setSVel( mIsValidImp(si) && isValidDen() ? si / getDen() : mUdf(float) );
}


ElasticLayer::ElasticLayer( const RefLayer& oth )
    : AILayer(oth.getThickness(),oth.getPVel(),oth.getDen())
    , svel_(mUdf(float))
{
    if ( oth.isElastic() )
	setSVel( oth.getSVel() );
    else
	fillVsWithVp( true );
}


ElasticLayer::~ElasticLayer()
{
}


RefLayer* ElasticLayer::clone() const
{
    return new ElasticLayer( *this );
}


RefLayer& ElasticLayer::operator =( const ElasticLayer& oth )
{
    return RefLayer::operator=( oth );
}


void ElasticLayer::copyFrom( const RefLayer& oth )
{
    AILayer::copyFrom( oth );
    if ( oth.isElastic() )
	setSVel( oth.getSVel() );
}


float ElasticLayer::getSI() const
{
    return isValidDen() && isValidVs() ? getDen() * svel_ : mUdf(float);
}


RefLayer& ElasticLayer::setSVel( float vel )
{
    svel_ = vel;
    return *this;
}


bool ElasticLayer::fillVsWithVp( bool onlyinvalid )
{
    if ( onlyinvalid && isValidVs() )
	return true;

    if ( !isValidVel() )
	return false;

    setSVel( mCast( float, 0.8619 * (double)getPVel() -1172. ) );
    return isValidVs();
}


bool ElasticLayer::isValidVs() const
{
    return mIsValidVel(svel_);
}


// VTILayer

VTILayer::VTILayer( float thkness, float pvel, float svel, float den,
		    float fracrho )
    : ElasticLayer(thkness,pvel,svel,den)
    , fracrho_(fracrho)
{
}


VTILayer::VTILayer( float thkness, float ai, float si, float den,
		    float fracrho, bool needcompthkness )
    : ElasticLayer(thkness,ai,si,den,needcompthkness)
    , fracrho_(fracrho)
{
}


VTILayer::VTILayer( const RefLayer& oth )
    : ElasticLayer(oth.getThickness(),oth.getPVel(),oth.getSVel(),oth.getDen())
    , fracrho_(mUdf(float))
{
    setFracRho( oth.isVTI() ? oth.getFracRho() : 0.f );
}


VTILayer::~VTILayer()
{
}


RefLayer& VTILayer::operator =( const VTILayer& oth )
{
    return RefLayer::operator=( oth );
}


RefLayer* VTILayer::clone() const
{
    return new VTILayer( *this );
}


RefLayer& VTILayer::setFracRho( float val )
{
    fracrho_ = val;
    return *this;
}


bool VTILayer::isValidFracRho() const
{
    return !mIsUdf(fracrho_) && fracrho_ >= 0.f;
}


void VTILayer::copyFrom( const RefLayer& oth )
{
    ElasticLayer::copyFrom( oth );
    setFracRho( oth.isVTI() ? oth.getFracRho() : 0.f );
}


// HTILayer

HTILayer::HTILayer( float thkness, float pvel, float svel, float den,
		    float fracrho, float fracazi )
    : VTILayer(thkness,pvel,svel,den,fracrho)
    , fracazi_(fracazi)
{
}


HTILayer::HTILayer( float thkness, float ai, float si, float den,
		    float fracrho, float fracazi, bool needcompthkness )
    : VTILayer(thkness,ai,si,den,fracrho,needcompthkness)
    , fracazi_(fracazi)
{
}


HTILayer::HTILayer( const RefLayer& oth )
    : VTILayer(oth.getThickness(),oth.getPVel(),oth.getSVel(),oth.getDen(),
	       oth.getFracRho())
    , fracazi_(mUdf(float))
{
    setFracAzi( oth.isHTI() ? oth.getFracAzi() : 0.f );
}


HTILayer::~HTILayer()
{
}


RefLayer& HTILayer::operator =( const HTILayer& oth )
{
    return RefLayer::operator=( oth );
}


RefLayer* HTILayer::clone() const
{
    return new HTILayer( *this );
}


RefLayer& HTILayer::setFracAzi( float val )
{
    fracazi_ = val;
    return *this;
}


bool HTILayer::isValidFracAzi() const
{
    return !mIsUdf(fracazi_);
}


void HTILayer::copyFrom( const RefLayer& oth )
{
    VTILayer::copyFrom( oth );
    setFracAzi( oth.isHTI() ? oth.getFracAzi() : 0.f );
}


// ElasticModel

ElasticModel::ElasticModel()
    : ObjectSet<RefLayer>()
{
}


ElasticModel::ElasticModel( const ObjectSet<RefLayer>& oth )
    : ObjectSet<RefLayer>()
{
    deepCopyClone( *this, oth );
}


ElasticModel::ElasticModel( const ElasticModel& oth )
    : ObjectSet<RefLayer>()
{
    *this = oth;
}


ElasticModel::~ElasticModel()
{
    erase();
}


ElasticModel& ElasticModel::operator =( const ElasticModel& oth )
{
    if ( &oth == this )
	return *this;

    ObjectSet<RefLayer>::operator= (oth );
    abovethickness_ = oth.abovethickness_;
    starttime_ = oth.starttime_;

    return *this;
}


ElasticModel& ElasticModel::operator -=( RefLayer* ptr )
{
    if ( ptr )
    {
	this->vec_.erase( (RefLayer*)ptr );
	delete ptr;
    }

    return *this;
}


void ElasticModel::append( const ObjectSet<RefLayer>& oth )
{
    const int sz = oth.size();
    this->vec_.setCapacity( this->size()+sz, true );
    for ( idx_type vidx=0; vidx<sz; vidx++ )
    {
	const RefLayer* obj = oth.get( vidx );
	if ( obj )
	    ObjectSet<RefLayer>::add( obj->clone() );
    }
}


void ElasticModel::erase()
{
    deepErase( *this );
}


RefLayer* ElasticModel::pop()
{
    delete ObjectSet<RefLayer>::pop();
    return nullptr;
}


RefLayer* ElasticModel::removeSingle( int vidx, bool kporder )
{
    delete ObjectSet<RefLayer>::removeSingle( vidx, kporder );
    return nullptr;
}


void ElasticModel::removeRange( int i1, int i2 )
{
    for ( int vidx=i1; vidx<=i2; vidx++ )
	delete this->get(vidx);

    ObjectSet<RefLayer>::removeRange( i1, i2 );
}


RefLayer* ElasticModel::replace( int vidx, RefLayer* ptr )
{
    delete ObjectSet<RefLayer>::replace( vidx, ptr );
    return nullptr;
}


RefLayer* ElasticModel::removeAndTake( int vidx, bool kporder )
{
    return ObjectSet<RefLayer>::removeSingle( vidx, kporder );
}


ElasticModel& ElasticModel::copyFrom( const ElasticModel& oth,
				      RefLayer::Type reqtyp )
{
    if ( oth.getMinType() >= reqtyp )
    {
	*this = oth;
	return *this;
    }

    const int sz = oth.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	auto* newlayer = RefLayer::clone( *oth.get(idx), &reqtyp );
	if ( validIdx(idx) )
	    replace( idx, newlayer );
	else
	    add( newlayer );
    }

    abovethickness_ = oth.abovethickness_;
    starttime_ = oth.starttime_;

    return *this;
}


ElasticModel& ElasticModel::setOverburden( double th, double t0 )
{
    abovethickness_ = th;
    starttime_ = t0;
    return *this;
}


double ElasticModel::aboveThickness() const
{
    if ( mIsUdf(abovethickness_) )
	return 0.;

    return abovethickness_;
}


double ElasticModel::startTime() const
{
    if ( mIsUdf(starttime_) )
	return 0.;

    return starttime_;
}


RefLayer::Type ElasticModel::getType() const
{
    if ( isHTI() )
	return RefLayer::HTI;
    if ( isVTI() )
	return RefLayer::VTI;
    if ( isElastic() )
	return RefLayer::Elastic;

    return RefLayer::Acoustic;
}


RefLayer::Type ElasticModel::getMinType() const
{
    RefLayer::Type ret = RefLayer::HTI;
    for ( const auto* layer : *this )
    {
	const RefLayer::Type typ = layer->getType();
	if ( typ < ret )
	    ret = typ;
	if ( ret == RefLayer::Acoustic )
	    break;
    }

    return ret;
}


bool ElasticModel::isElastic() const
{
    for ( const auto* layer : *this )
	if ( layer->isElastic() )
	    return true;

    return false;
}


bool ElasticModel::isVTI() const
{
    for ( const auto* layer : *this )
	if ( layer->isVTI() )
	    return true;

    return false;
}


bool ElasticModel::isHTI() const
{
    for ( const auto* layer : *this )
	if ( layer->isHTI() )
	    return true;

    return false;
}


int ElasticModel::isOK( bool dodencheck, bool dosvelcheck,
			bool dofracrhocheck, bool dofracazicheck ) const
{
    int idx = -1;
    for ( const auto* layer : *this )
    {
	idx++;
	if ( !layer->isOK(dodencheck,dosvelcheck,dofracrhocheck,dofracazicheck))
	    return idx;
    }

    return -1;
}


#define mRmLay(idx) \
{ \
    firsterroridx = idx; \
    removeSingle( idx ); \
    continue; \
}

void ElasticModel::checkAndClean( int& firsterroridx, bool dodencheck,
				  bool dosvelcheck, bool onlyinvalid )
{
    for ( int idx=size()-1; idx>=0; idx-- )
    {
	RefLayer& lay = *get(idx);
	if ( !lay.isOK(false,false) )
	    mRmLay(idx)

	if ( dodencheck && !lay.isValidDen() )
	{
	    if ( !lay.asAcoustic().fillDenWithVp(onlyinvalid) )
	    {
		mRmLay(idx)
		continue;
	    }
	}

	if ( dosvelcheck && lay.isElastic() && !lay.isValidVs() )
	{
	    if ( !lay.asElastic()->fillVsWithVp(onlyinvalid) )
		mRmLay(idx)
	}

	// Not removing the VTI/HTI props, only setting a neutral value
	if ( dosvelcheck && (lay.isVTI() || lay.isHTI()) &&
	     !lay.isValidFracRho() )
	    lay.setFracRho( 0.f );

	if ( dosvelcheck && lay.isHTI() && !lay.isValidFracAzi() )
	    lay.setFracAzi( 0.f );
    }
}


void ElasticModel::interpolate( bool dovp, bool doden, bool dovs )
{
    BoolTypeSet dointerpolate;
    dointerpolate += dovp;
    dointerpolate += doden;
    if ( isElastic() )
	dointerpolate += dovs;
    if ( isVTI() )
	dointerpolate += true;
    if ( isHTI() )
	dointerpolate += true;

    for ( int iprop=0; iprop<dointerpolate.size(); iprop++ )
    {
	if ( !dointerpolate[iprop] )
	    continue;

	BendPointBasedMathFunction<float,float> data;
	for ( int idx=0; idx<size(); idx++ )
	{
	    const RefLayer& layer = *get(idx);
	    float val = mUdf(float);
	    if ( iprop == 0 && layer.isValidVel() )
		val = layer.getPVel();
	    else if ( iprop == 1 && layer.isValidDen() )
		val = layer.getDen();
	    else if ( iprop == 2 && layer.isValidVs() )
		val = layer.getSVel();
	    else if ( iprop == 3 && layer.isValidFracRho() )
		val = layer.getFracRho();
	    else if ( iprop == 4 && layer.isValidFracAzi() )
		val = layer.getFracAzi();

	    if ( !mIsUdf(val) )
		data.add( (float)idx, val, false );
	}

	if ( data.isEmpty() )
	    continue;

	for ( int idx=0; idx<size(); idx++ )
	{
	    const float val = data.getValue( (float)idx );
	    RefLayer& layer = *get(idx);
	    if ( iprop == 0 )
		layer.setPVel( val );
	    else if ( iprop == 1 )
		layer.setDen( val );
	    else if ( iprop == 2 )
		layer.setSVel( val );
	    else if ( iprop == 3 )
		layer.setFracRho( val );
	    else if ( iprop == 4 )
		layer.setFracAzi( val );
	}
    }
}


void ElasticModel::upscale( float maxthickness )
{
    if ( isEmpty() || maxthickness < cMinLayerThickness() )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();

    float totthickness = 0.f;
    ElasticModel curmodel;
    PtrMan<RefLayer> newlayer;
    for ( int lidx=0; lidx<orgmodl.size(); lidx++ )
    {
	PtrMan<RefLayer> curlayer = orgmodl.get(lidx)->clone();
	float thickness = curlayer->getThickness();
	if ( !curlayer->isValidThickness() || !curlayer->isValidVel() )
	    continue;

	if ( thickness > maxthickness-cMinLayerThickness() )
	{
	    if ( !curmodel.isEmpty() )
	    {
		newlayer = RefLayer::create( curmodel.getType() );
		if ( curmodel.getUpscaledBackus(*newlayer.ptr()) )
		    add( newlayer.release() );

		totthickness = 0.f;
		curmodel.setEmpty();
	    }

	    add( curlayer->clone() );
	    continue;
	}

	const bool lastlay = totthickness + thickness >
			     maxthickness - cMinLayerThickness();
	const float thicknesstoadd = lastlay ? maxthickness - totthickness
					     : thickness;
	totthickness += thicknesstoadd;
	if ( lastlay )
	{
	    thickness -= thicknesstoadd;
	    curlayer->setThickness( thicknesstoadd );
	}

	curmodel.add( curlayer->clone() );
	if ( lastlay )
	{
	    newlayer = RefLayer::create( curmodel.getType() );
	    if ( curmodel.getUpscaledBackus(*newlayer.ptr()) )
		add( newlayer.release() );

	    totthickness = thickness;
	    curmodel.setEmpty();
	    if ( thickness > cMinLayerThickness() )
	    {
		curlayer->setThickness( thickness );
		curmodel.add( curlayer->clone() );
	    }
	}
    }
    if ( totthickness > cMinLayerThickness() && !curmodel.isEmpty() )
    {
	newlayer = RefLayer::create( curmodel.getType() );
	if ( curmodel.getUpscaledBackus(*newlayer.ptr()) )
	    add( newlayer.release() );
    }
}


void ElasticModel::upscaleByN( int nbblock )
{
    if ( isEmpty() || nbblock < 2 )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();

    ElasticModel curmdl;
    PtrMan<RefLayer> newlayer;
    for ( int lidx=0; lidx<orgmodl.size(); lidx++ )
    {
	curmdl.add( orgmodl.get( lidx )->clone() );
	if ( (lidx+1) % nbblock == 0 )
	{
	    newlayer = RefLayer::create( curmdl.getType() );
	    if ( curmdl.getUpscaledBackus(*newlayer.ptr()) )
		add( newlayer->clone() );

	    curmdl.setEmpty();
	}
    }

    if ( !curmdl.isEmpty() )
    {
	newlayer = RefLayer::create( curmdl.getType() );
	if ( curmdl.getUpscaledBackus(*newlayer.ptr()) )
	    add( newlayer->clone() );
    }
}


void ElasticModel::block( float relthreshold, bool pvelonly )
{
    if ( isEmpty() || relthreshold < mDefEpsF || relthreshold > 1.f-mDefEpsF )
	return;

    TypeSet<Interval<int> > blocks;
    if ( !doBlocking(relthreshold,pvelonly,blocks) )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();
    for ( int lidx=0; lidx<blocks.size(); lidx++ )
    {
	ElasticModel blockmdl;
	const Interval<int> curblock = blocks[lidx];
	for ( int lidy=curblock.start_; lidy<=curblock.stop_; lidy++ )
	    blockmdl.add( orgmodl.get(lidy)->clone() );

	PtrMan<RefLayer> outlay = RefLayer::create( blockmdl.getType() );
	if ( !blockmdl.getUpscaledBackus(*outlay.ptr()) )
	    continue;

	add( outlay.release() );
    }
}


bool ElasticModel::getUpscaledByThicknessAvg( RefLayer& outlay ) const
{
    if ( isEmpty() )
	return false;

    outlay.setThickness( mUdf(float) );
    outlay.setPVel( mUdf(float) );
    outlay.setDen( mUdf(float) );
    outlay.setSVel( mUdf(float) );

    float totthickness=0.f, sonp=0.f, den=0.f, sson=0.f;
    float velpthickness=0.f, denthickness=0.f, svelthickness=0.f;
    for ( int lidx=0; lidx<size(); lidx++ )
    {
	const RefLayer& curlayer = *get(lidx);
	if ( !curlayer.isValidThickness() || !curlayer.isValidVel() )
	    continue;

	const float ldz = curlayer.getThickness();
	const float layinvelp = curlayer.getPVel();
	const float layinden = curlayer.getDen();
	const float layinsvel = curlayer.getSVel();

	totthickness += ldz;
	sonp += ldz / layinvelp;
	velpthickness += ldz;

	if ( curlayer.isValidDen() )
	{
	    den += layinden * ldz;
	    denthickness += ldz;
	}

	if ( curlayer.isElastic() && curlayer.isValidVs() )
	{
	    sson += ldz / layinsvel;
	    svelthickness += ldz;
	}
    }

    if ( totthickness<mDefEpsF || velpthickness<mDefEpsF || sonp<mDefEpsF )
	return false;

    const float velfinal = velpthickness / sonp;
    if ( !mIsValidVel(velfinal) )
	return false;

    outlay.setThickness( totthickness );
    outlay.setPVel( velfinal );
    if ( !mIsValidThickness(denthickness) || denthickness < mDefEpsF )
	outlay.setDen( mUdf(float) );
    else
    {
	const float denfinal = den / denthickness;
	outlay.setDen( mIsValidDen(denfinal) ? denfinal : mUdf(float) );
    }

    if ( outlay.isElastic() )
    {
	if ( !mIsValidThickness(svelthickness) || svelthickness < mDefEpsF )
	    outlay.setSVel( mUdf(float) );
	else
	{
	    const float svelfinal = svelthickness / sson;
	    outlay.setSVel( mIsValidVel(svelfinal) ? svelfinal : mUdf(float) );
	}
    }

    return true;
}


bool ElasticModel::getUpscaledBackus( RefLayer& outlay, float theta ) const
{
    if ( isEmpty() )
	return false;

    outlay.setThickness( mUdf(float) );
    outlay.setPVel( mUdf(float) );
    outlay.setDen( mUdf(float) );
    outlay.setSVel( mUdf(float) );

    float totthickness=0.f, den=0.f;
    float x=0.f, y=0.f, z=0.f, u=0.f, v=0.f, w=0.f;
    for ( int lidx=0; lidx<size(); lidx++ )
    {
	const RefLayer& curlayer = *get(lidx);
	const float ldz = curlayer.getThickness();
	const float layinvelp = curlayer.getPVel();
	const float layinden = curlayer.getDen();
	const float layinsvel = curlayer.getSVel();

	PtrMan<RefLayer> tmplay = curlayer.clone();
	tmplay->setThickness( ldz );
	tmplay->setPVel( layinvelp );
	tmplay->setDen( layinden );
	tmplay->setSVel( layinsvel );
	if ( !tmplay->isOK() )
	    continue;

	totthickness += ldz;
	const float vp2 = layinvelp * layinvelp;
	const float vs2 = layinsvel * layinsvel;
	const float mu = layinden * vs2;
	const float lam = layinden * ( vp2 - 2.f * vs2 );
	const float denomi = lam + 2.f * mu;
	den += ldz * layinden;
	x += ldz * mu * (lam + mu ) / denomi;
	y += ldz * mu * lam / denomi;
	z += ldz * lam / denomi;
	u += ldz / denomi;
	v += ldz / mu;
	w += ldz * mu;
    }

    if ( totthickness<mDefEpsF )
	return getUpscaledByThicknessAvg( outlay );

    den /= totthickness;
    x /= totthickness; y /= totthickness; z /= totthickness;
    u /= totthickness; v /= totthickness; w /= totthickness;
    outlay.setThickness( totthickness );
    outlay.setDen( den );

    const float c11 = 4.f * x + z * z / u;
    const float c12 = 2.f * y + z * z / u;
    const float c33 = 1.f / u;
    const float c13 = z / u;
    const float c44 = 1.f / v;
    const float c66 = w;
    const float c66qc = ( c11 - c12 ) / 2.f;

    if ( !mIsEqual(c66,c66qc,1e6f) )
	return getUpscaledByThicknessAvg( outlay );

    const bool zerooffset = mIsZero( theta, 1e-5f );
    const float s2 = zerooffset ? 0.f : sin( theta ) * sin( theta );
    const float c2 = zerooffset ? 1.f : cos( theta ) * cos( theta );
    const float s22 = zerooffset ? 0.f : sin( 2.f *theta ) * sin( 2.f *theta );

    const float mm = c11 * s2 + c33 * c2 + c44;
    const float mn = Math::Sqrt( ( ( c11 - c44 ) * s2 - ( c33 - c44 ) * c2 ) *
				 ( ( c11 - c44 ) * s2 - ( c33 - c44 ) * c2 )
				 + ( c13 + c44 ) *
				   ( c13 + c44 ) * s22 );
    const float mp2 = ( mm + mn ) / 2.f;
    const float msv2 = ( mm - mn ) / 2.f;

    outlay.setPVel( Math::Sqrt( mp2 / den ) );
    outlay.setSVel( Math::Sqrt( msv2 / den ) );

    return true;
}


void ElasticModel::setMaxThickness( float maxthickness, bool extend )
{
    if ( isEmpty() || maxthickness < cMinLayerThickness() )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();
    const int initialsz = orgmodl.size();
    int nbinsert = mUdf(int);
    for ( int lidx=0; lidx<initialsz; lidx++ )
    {
	const float thickness = orgmodl.get(lidx)->getThickness();
	if ( !mIsValidThickness(thickness) )
	    continue;

	PtrMan<RefLayer> newlayer = orgmodl.get(lidx)->clone();
	nbinsert = 1;
	if ( thickness > maxthickness - cMinLayerThickness() )
	{
	    nbinsert = mNINT32( Math::Ceil(thickness/maxthickness) );
	    newlayer->setThickness( newlayer->getThickness() / nbinsert );
	}

	for ( int nlidx=0; nlidx<nbinsert; nlidx++ )
	    add( newlayer->clone() );
    }

    if ( !extend || isEmpty() ||
	 mIsUdf(abovethickness_) || abovethickness_ < cMinLayerThickness() )
	return;

    int nrextra = 1;
    const float pvel = mCast(float, 2. * abovethickness_ / starttime_ );
    PtrMan<RefLayer> newlayer =
			new AILayer( abovethickness_, pvel, mUdf(float) );
    if ( abovethickness_ > maxthickness - cMinLayerThickness() )
    {
	nrextra = mNINT32( Math::Ceil(abovethickness_/maxthickness) );
	newlayer->setThickness( newlayer->getThickness() / nrextra );
    }

    for ( int idx=0; idx<nrextra; idx++ )
	insertAt( newlayer->clone(), 0 );

    setOverburden( mUdf(double), mUdf(double) );
}


void ElasticModel::mergeSameLayers()
{
    if ( isEmpty() )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();
    const int initialsz = orgmodl.size();
    bool havemerges = false;
    float totthickness = 0.f;
    PtrMan<RefLayer> prevlayer = orgmodl.first()->clone();
    for ( int lidx=1; lidx<initialsz; lidx++ )
    {
	const RefLayer& curlayer = *orgmodl.get(lidx);
	if ( mIsEqual(curlayer.getPVel(),prevlayer->getPVel(),1e-2f) &&
	     mIsEqual(curlayer.getDen(),prevlayer->getDen(),1e-2f) &&
	     mIsEqual(curlayer.getSVel(),prevlayer->getSVel(),1e-2f) )
	{
	    if ( havemerges == false )
		totthickness = prevlayer->getThickness();

	    havemerges = true;
	    totthickness += curlayer.getThickness();
	}
	else
	{
	    if ( havemerges )
	    {
		prevlayer->setThickness( totthickness );
		havemerges = false;
	    }

	    add( prevlayer.release() );
	    prevlayer = curlayer.clone();
	}
    }
    if ( havemerges )
	prevlayer->setThickness( totthickness );

    add( prevlayer.release() );
}


bool ElasticModel::createFromVel( const ZValueSeries& zsamp,const float* pvel,
				  const float* svel, const float* den )
{
    if ( !zsamp.isOK() || !pvel )
	return false;

    setEmpty();
    const int sz = od_int64 (zsamp.size());
    const float startz = float (zsamp[0]);
    const bool zistime = zsamp.isTime();

    int firstidx = 0;
    float firstlayerthickness;
    const float firstvel = pvel[firstidx];
    if ( zistime )
    {
	firstlayerthickness = startz<0.f ? 0.0f : startz;
	firstlayerthickness *= firstvel / 2.0f;
    }
    else
    {
	if ( startz < 0. )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		if ( zsamp[idx] >= 0. )
		{
		    firstidx = idx;
		    break;
		}
	    }
	}

	firstlayerthickness = float (zsamp[firstidx]);
    }

    const float firstden = den ? den[firstidx] : mUdf(float);
    const float firstsvel = svel ? svel[firstidx] : mUdf(float);
    add( svel ? new ElasticLayer( firstlayerthickness, firstvel, firstsvel,
				  firstden )
	      : new AILayer( firstlayerthickness, firstvel, firstden ) );

    for ( int idx=firstidx+1; idx<sz; idx++ )
    {
	const float velp = pvel[idx];
	const float zstep = zsamp[idx] - zsamp[idx-1];
	const float layerthickness = zistime ? zstep * velp / 2.0f : zstep;
	const float rhob = den ? den[idx] : mUdf(float);
	if ( svel )
	{
	    const float vels = svel[idx];
	    add( new ElasticLayer( layerthickness, velp, vels, rhob ) );
	}
	else
	    add( new AILayer( layerthickness, velp, rhob ) );
    }

    if ( isEmpty() )
	return false;

    mergeSameLayers();
    if ( zsamp.isRegular() )
    {
	const float regzstep = float (
				dCast(const RegularZValues&,zsamp).getStep());
	removeSpuriousLayers( regzstep, zistime );
    }

    return !isEmpty();
}


bool ElasticModel::createFromAI( const ZValueSeries& zsamp, const float* ai,
				 const float* si, const float* den )
{
    if ( !ai || !zsamp.isOK() )
	return false;

    setEmpty();
    const int sz = od_int64 (zsamp.size());
    const float startz = float (zsamp[0]);
    const bool zistime = zsamp.isTime();

    int firstidx = 0;
    float firstlayerthickness;
    if ( zistime )
	firstlayerthickness = startz<0.f ? 0.f : startz;
    else
    {
	if ( startz < 0. )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		if ( zsamp[idx] >= 0. )
		{
		    firstidx = idx;
		    break;
		}
	    }
	}

	firstlayerthickness = float (zsamp[firstidx]);
    }

    const float firstden = den ? den[firstidx] : mUdf(float);
    if ( si )
	add( new ElasticLayer( firstlayerthickness, ai[firstidx], si[firstidx],
			       firstden, zistime ) );
    else
	add( new AILayer(firstlayerthickness,ai[firstidx],firstden,zistime) );

    for ( int idx=firstidx+1; idx<sz; idx++ )
    {
	const float zstep = float (zsamp[idx] - zsamp[idx-1]);
	const float rhob = den ? den[idx] : mUdf(float);
	if ( si )
	    add( new ElasticLayer(zstep, ai[idx], si[idx], rhob,zistime) );
	else
	    add( new AILayer(zstep, ai[idx], rhob, zistime) );
    }

    if ( isEmpty() )
	return false;

    mergeSameLayers();
    if ( zsamp.isRegular() )
    {
	const float regzstep = float (
				dCast(const RegularZValues&,zsamp).getStep());
	removeSpuriousLayers( regzstep, zistime );
    }

    return !isEmpty();
}


void ElasticModel::removeSpuriousLayers( float zrgstep, bool zistime )
{
    for ( int idx=size()-2; idx>0; idx-- )
    {
	const float layervel = get(idx)->getPVel();
	const float layerthickness = get(idx)->getThickness();
	const float layertwtthickness = 2.f * layerthickness / layervel;
	if ( ( zistime && !mIsEqual(layertwtthickness,zrgstep,1e-2f) ) ||
	     (!zistime && !mIsEqual(layerthickness,zrgstep,1e-2f) ) )
	    continue;

	const float velabove = get(idx-1)->getPVel();
	const float velbelow = get(idx+1)->getPVel();
	const float layerthicknessabove = get(idx-1)->getThickness();
	const float layerthicknessbelow = get(idx+1)->getThickness();
	if ( zistime )
	{
	    const float twtthicknessabove = 2.f * layerthicknessabove /velabove;
	    const float twtthicknessbelow = 2.f * layerthicknessbelow /velbelow;
	    if ( mIsEqual(twtthicknessabove,zrgstep,1e-2f) ||
		 mIsEqual(twtthicknessbelow,zrgstep,1e-2f) )
		continue;
	}
	else
	{
	    if ( mIsEqual(layerthicknessabove,zrgstep,1e-2f) ||
		 mIsEqual(layerthicknessbelow,zrgstep,1e-2f) )
		continue;
	}

	const float twtbelow = layertwtthickness * ( layervel-velabove )
						 / ( velbelow-velabove );
	if ( twtbelow < 0.f )
	    continue;

	const float twtabove = layertwtthickness - twtbelow;
	if ( twtabove < 0.f )
	    continue;

	get(idx-1)->setThickness( get(idx-1)->getThickness() +
				  twtabove * velabove / 2.f );
	get(idx+1)->setThickness( get(idx+1)->getThickness() +
				  twtbelow * velbelow / 2.f );
	removeSingle( idx );
    }
}


bool ElasticModel::getValues( bool isden, bool issvel,
			      TypeSet<float>& vals ) const
{
    const int sz = size();
    vals.setEmpty();

    for ( int idx=0; idx<sz; idx++ )
    {
	const RefLayer& layer = *get(idx);
	const bool isvalid = isden ? layer.isValidDen()
				   : (issvel ? layer.isValidVel()
					     : layer.isValidVs());
	if ( !isvalid )
	    return false;

	vals += isden ? layer.getDen()
		      : ( issvel ? layer.getSVel() : layer.getPVel() );
    }

    return true;
}

#define mGetVals(dofill,isden,issvel,data) \
{ \
    if ( dofill ) \
    { \
	if ( !getValues(isden,issvel,data) ) \
		return false; \
	icomp++; \
    } \
}

#define mFillArr(dofill,data) \
{ \
    if ( dofill ) \
    { \
	for ( int idx=0; idx<sz; idx++ ) \
		vals.set( icomp, idx, data[idx] ); \
	icomp++; \
    } \
}

bool ElasticModel::getValues( bool vel, bool den, bool svel,
			      Array2D<float>& vals ) const
{
    const int sz = size();
    TypeSet<float> velvals, denvals, svelvals;

    int icomp = 0;
    mGetVals(vel,false,false,velvals);
    mGetVals(den,true,false,denvals);
    mGetVals(svel,false,true,svelvals);

    const Array2DInfoImpl info2d( icomp, sz );
    if ( !vals.setInfo(info2d) )
	return false;

    icomp = 0;
    mFillArr(vel,velvals);
    mFillArr(den,denvals);
    mFillArr(svel,svelvals);

    return true;
}


bool ElasticModel::getRatioValues( bool vel, bool den, bool svel,
				   Array2D<float>& ratiovals,
				   Array2D<float>& vals ) const
{
    if ( !getValues(vel,den,svel,vals) )
	return false;

    const int sz = size();
    const int nrcomp = vals.info().getSize( 0 );
    if ( nrcomp == 0 )
	return false;

    const Array2DInfoImpl info2d( nrcomp, sz );
    if ( !ratiovals.setInfo(info2d) )
	return false;

    for ( int icomp=0; icomp<nrcomp; icomp++ )
    {
	float prevval = vals.get( icomp, 0 );
	ratiovals.set( icomp, 0, 0.f );
	for ( int idx=1; idx<sz; idx++ )
	{
	    const float curval = vals.get( icomp, idx );
	    const float val = curval < prevval
			    ? prevval / curval - 1.f
			    : curval / prevval - 1.f;
	    ratiovals.set( icomp, idx, val );
	    prevval = curval;
	}
    }

    return true;
}


#define mAddBlock( block ) \
    if ( !blocks.isEmpty() && block.start_<blocks[0].start_ ) \
	blocks.insert( 0, block ); \
    else \
	blocks += block


bool ElasticModel::doBlocking( float relthreshold, bool pvelonly,
			       TypeSet<Interval<int> >& blocks ) const
{
    blocks.setEmpty();

    Array2DImpl<float> vals( 1, size() );
    Array2DImpl<float> ratiovals( 1, size() );
    if ( !getRatioValues(true,!pvelonly,!pvelonly,ratiovals,vals) )
	return false;

    const int nrcomp = vals.info().getSize( 0 );
    const int modelsize = vals.info().getSize( 1 );

    TypeSet<Interval<int> > investigationqueue;
    investigationqueue += Interval<int>( 0, modelsize-1 );
    while ( investigationqueue.size() )
    {
	Interval<int> curblock = investigationqueue.pop();

	while ( true )
	{
	    const int width = curblock.width();
	    if ( width==0 )
	    {
		mAddBlock( curblock );
		break;
	    }

	    TypeSet<int> bendpoints;
	    const int lastblock = curblock.start_ + width;
	    TypeSet<float> firstval( nrcomp, mUdf(float) );
	    TypeSet< Interval<float> > valranges;
	    float maxvalratio = 0;
	    for ( int icomp=0; icomp<nrcomp; icomp++ )
	    {
		firstval[icomp] = vals.get( icomp, curblock.start_ );
		Interval<float> valrange(  firstval[icomp],  firstval[icomp] );
		valranges += valrange;
	    }

	    for ( int lidx=curblock.start_+1; lidx<=lastblock; lidx++ )
	    {
		for ( int icomp=0; icomp<nrcomp; icomp++ )
		{
		    const float curval = vals.get( icomp, lidx );
		    valranges[icomp].include( curval );

		    const float valratio = ratiovals.get( icomp, lidx );
		    if ( valratio >= maxvalratio )
		    {
			if ( !mIsEqual(valratio,maxvalratio,1e-5f) )
			    bendpoints.erase();

			bendpoints += lidx;
			maxvalratio = valratio;
		    }
		}
	    }

	    if ( maxvalratio<=relthreshold )
	    {
		mAddBlock( curblock );
		break;
	    }

	    int bendpoint = curblock.center();
	    if ( bendpoints.isEmpty() )
	    {
		pFreeFnErrMsg("Should never happen");
	    }
	    else if ( bendpoints.size()==1 )
	    {
		bendpoint = bendpoints[0];
	    }
	    else
	    {
		const int middle = bendpoints.size()/2;
		bendpoint = bendpoints[middle];
	    }

	    investigationqueue += Interval<int>( curblock.start_, bendpoint-1);
	    curblock = Interval<int>( bendpoint, curblock.stop_ );
	}
    }

    return !blocks.isEmpty() && ( blocks.size() < modelsize );
}


float ElasticModel::getLayerDepth( int ilayer ) const
{
    float depth = 0.f;
    if ( ilayer >= size() )
	ilayer = size();

    for ( int idx=0; idx<ilayer; idx++ )
	depth += get(idx)->getThickness();

    if ( ilayer < size() )
	depth += get(ilayer)->getThickness() / 2.f;

    return depth;
}


Interval<float> ElasticModel::getTimeSampling( bool usevs ) const
{
    Interval<float> ret( 0.f, 0.f );
    for ( const auto* layer : *this )
    {
	if ( !layer->isOK(false,usevs) )
	    continue;

	const float vel = usevs ? layer->getSVel() : layer->getPVel();
	ret.stop_ += layer->getThickness() / vel;
    }

    ret.stop_ *= 2.f; // TWT needed
    ret.shift( startTime() );
    return ret;
}


// ElasticModelSet

ElasticModelSet::ElasticModelSet()
    : ManagedObjectSet<ElasticModel>()
{
}


ElasticModelSet::~ElasticModelSet()
{
}


bool ElasticModelSet::setSize( int nrmdls )
{
    const int oldsz = size();
    if ( nrmdls == oldsz )
	return true;

    if ( nrmdls <= 0 )
	setEmpty();
    else if ( nrmdls < oldsz )
	removeRange( nrmdls, oldsz );
    else // nrmdls > oldsz
    {
	while ( size() < nrmdls )
	    add( new ElasticModel );
    }

    return true;
}


bool ElasticModelSet::getTimeSampling( Interval<float>& timerg,
				       bool usevs ) const
{
    if ( isEmpty() )
	return false;

    timerg.set( mUdf(float), -mUdf(float) );
    for ( const auto* model : *this )
    {
	const Interval<float> tsampling = model->getTimeSampling( usevs );
	timerg.include( tsampling, false );
    }

    return !timerg.isUdf();
}
