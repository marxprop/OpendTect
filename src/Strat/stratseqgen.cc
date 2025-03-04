/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratlayseqgendesc.h"

#include "ascstream.h"
#include "iopar.h"
#include "keystrs.h"
#include "mathproperty.h"
#include "ptrman.h"
#include "separstr.h"
#include "stratlayer.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlaymodgen.h"
#include "strattransl.h"
#include "stratsinglaygen.h"
#include "stratreftree.h"
#include "unitofmeasure.h"


#define mFileType "Layer Sequence Generator Description"

static const char* sKeyFileType = mFileType;
static const char* sKeyIDNew = "[New]";
static const char* sKeyTopdepth = "Top depth";
static const char* sKeyVelAbove = "Overburden velocity";

mImplFactory(Strat::LayerGenerator,Strat::LayerGenerator::factory)
mDefSimpleTranslators(StratLayerSequenceGenDesc,mFileType,od,Mdl);

const char* Strat::LayerSequenceGenDesc::sKeyWorkBenchParams() { return "WB"; }


Strat::LayerModelGenerator::LayerModelGenerator(
		const Strat::LayerSequenceGenDesc& desc, Strat::LayerModel& lm,
		int nrseqs )
    : Executor("Layer Sequence Generator")
    , desc_(desc)
    , lm_(lm)
    , nrseqs_(nrseqs)
{
}


Strat::LayerModelGenerator::~LayerModelGenerator()
{}


bool Strat::LayerModelGenerator::goImpl( od_ostream* strm, bool first,
					 bool last, int delay )
{
    lm_.setEmpty();
    seqnr_ = 0;
    if ( desc_.prepareGenerate() )
	msg_ = tr("Generating layer sequences");
    else
    {
	msg_ = desc_.errMsg();
	seqnr_ = -1;
    }

    lm_.propertyRefs() = desc_.propSelection();
    return Executor::goImpl( strm, first, last, delay );
}


int Strat::LayerModelGenerator::nextStep()
{
    if ( seqnr_ == -1 )
	return ErrorOccurred();

    const float modpos = nrseqs_ < 2 ? 0.5f : ((float)seqnr_)/(nrseqs_-1);
    if ( !desc_.generate(lm_.addSequence(),modpos) )
    {
	msg_ = desc_.errMsg();
	return ErrorOccurred();
    }

    seqnr_++;
    return seqnr_ >= nrseqs_ ? Finished() : MoreToDo();
}


Strat::LayerGenerator::~LayerGenerator()
{}


Strat::LayerGenerator* Strat::LayerGenerator::get( const IOPar& iop,
						const Strat::RefTree& rt )
{
    Strat::LayerGenerator* ret = factory().create( iop.find(sKey::Type()) );
    if ( !ret )
	return nullptr;

    if ( ret->usePar(iop,rt) )
	return ret;

    delete ret;
    return nullptr;
}


bool Strat::LayerGenerator::usePar( const IOPar&, const Strat::RefTree& )
{
    return true;
}


void Strat::LayerGenerator::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), factoryKeyword() );
}


bool Strat::LayerGenerator::generateMaterial( Strat::LayerSequence& seq,
					      Property::EvalOpts eo ) const
{
    if ( seq.propertyRefs().isEmpty() )
	updateUsedProps( seq.propertyRefs() );
    return genMaterial( seq, eo );
}


// Strat::LayerSequenceGenDesc

float Strat::LayerSequenceGenDesc::defOverburdenVelocity()
{
    return SI().depthsInFeet() ? 8000.f : 2000.f;
}


Strat::LayerSequenceGenDesc::LayerSequenceGenDesc( const RefTree& rt )
    : ObjectSet<LayerGenerator>()
    , rt_(rt)
{
    startdepth_ = getConvertedValue( -SI().seismicReferenceDatum(),
				    UnitOfMeasure::surveyDefSRDStorageUnit(),
				    UnitOfMeasure::surveyDefSRDUnit() );
    velocityabove_ = defOverburdenVelocity();

    if ( !ePROPS().ensureHasElasticProps(false,true) )
	return;

    const PropertyRefSet& props = PROPS();
    propsel_.add( props.getByMnemonic( Mnemonic::defDEN() ) )
	    .add( props.getByMnemonic( Mnemonic::defPVEL() ) )
	    .add( props.getByMnemonic( Mnemonic::defAI() ) );
}


Strat::LayerSequenceGenDesc::LayerSequenceGenDesc(
					const LayerSequenceGenDesc& oth )
    : rt_(oth.rt_)
{
    *this = oth;
}


Strat::LayerSequenceGenDesc::~LayerSequenceGenDesc()
{
    deepErase( *this );
}


Strat::LayerSequenceGenDesc& Strat::LayerSequenceGenDesc::operator=(
	const Strat::LayerSequenceGenDesc& oth )
{
    if ( this == &oth )
	return *this;

    deepCopyClone( *this, oth );
    setPropSelection( oth.propsel_ );
    workbenchparams_ = oth.workbenchparams_;
    elasticpropselmid_ = oth.elasticpropselmid_;
    startdepth_ = oth.startdepth_;
    velocityabove_ = oth.velocityabove_;

    return *this;
}


void Strat::LayerSequenceGenDesc::erase()
{
    deepErase( *this );
    startdepth_ = getConvertedValue( -SI().seismicReferenceDatum(),
				    UnitOfMeasure::surveyDefSRDStorageUnit(),
				    UnitOfMeasure::surveyDefSRDUnit() );
    velocityabove_ = defOverburdenVelocity();
    propsel_.setEmpty();
    elasticpropselmid_.setUdf();
    errmsg_.setEmpty();
}


bool Strat::LayerSequenceGenDesc::getFrom( od_istream& strm )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyFileType) )
	{ errmsg_ = tr("Bad header found"); return false; }

    erase();

    IOPar iop; iop.getFrom( astrm );

    BufferString topdepthstr;
    if ( iop.get(sKeyTopdepth,topdepthstr) && !topdepthstr.isEmpty() )
    {
	const FileMultiString fms( topdepthstr );
	const UnitOfMeasure* depthuom = PropertyRef::thickness().unit();
	const UnitOfMeasure* zvaluom = nullptr;
	float topdepth = mUdf(float);
	if ( !fms.isEmpty() )
	    topdepth = fms.getFValue( 0 );

	if ( fms.size() > 1 )
	{
	    const StringView zunitlbl = fms[1];
	    zvaluom = UoMR().get( zunitlbl.buf() );
	}

	convValue( topdepth, zvaluom, depthuom );
	if ( !mIsUdf(topdepth) )
	    startdepth_ = topdepth;
    }

    const float abovevel = getOverburdenVelocity( sKeyVelAbove, iop );
    if ( !mIsUdf(abovevel) )
	velocityabove_ = abovevel;

    iop.get( ElasticPropSelection::sKeyElasticPropSel(), elasticpropselmid_ );
    PtrMan<IOPar> workbenchpars = iop.subselect( sKeyWorkBenchParams() );
    if ( !workbenchpars || workbenchpars->isEmpty() )
	workbenchpars = iop.subselect( "Workbench parameters" );
    if ( workbenchpars )
	workbenchparams_ = *workbenchpars;
    else
	workbenchparams_.setEmpty();

    while ( !atEndOfSection(astrm.next()) )
    {
	iop.setEmpty(); iop.getFrom(astrm);
	if ( iop.isEmpty() )
	    continue;

	auto* lg = LayerGenerator::get( iop, rt_ );
	if ( lg )
	    { lg->setGenDesc( this ); add( lg ); }
    }

    if ( isEmpty() )
    {
	errmsg_ = tr( "Empty description : no valid units found" );
	return false;
    }

    for ( const auto* lgen : *this )
	lgen->updateUsedProps( propsel_ );

    return true;
}


bool Strat::LayerSequenceGenDesc::putTo( od_ostream& strm ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(sKeyFileType) )
	{ errmsg_ = tr("Cannot write file header"); return false; }

    IOPar iop;
    FileMultiString fms;
    fms.add( startdepth_ ).add( PropertyRef::thickness().disp_.getUnitLbl() );
    iop.set( sKeyTopdepth, fms );

    setOverburdenVelocity( sKeyVelAbove, velocityabove_, iop );

    if ( !elasticpropselmid_.isUdf() )
	iop.set( ElasticPropSelection::sKeyElasticPropSel(),elasticpropselmid_);

    iop.mergeComp( workbenchparams_, sKeyWorkBenchParams() );
    iop.putTo( astrm );

    for ( int idx=0; idx<size(); idx++ )
    {
	iop.setEmpty(); (*this)[idx]->fillPar(iop);
	iop.putTo( astrm );
    }

    return true;
}


void Strat::LayerSequenceGenDesc::setPropSelection(
					const PropertyRefSelection& prsel )
{
    propsel_ = prsel;
    for ( auto* gen : *this )
	gen->syncProps( propsel_ );
}


void Strat::LayerSequenceGenDesc::setElasticPropSel( const MultiID& mid )
{
    elasticpropselmid_ = mid;
}


const MultiID& Strat::LayerSequenceGenDesc::elasticPropSel() const
{
    return elasticpropselmid_;
}


bool Strat::LayerSequenceGenDesc::prepareGenerate() const
{
    errmsg_.setEmpty();
    for ( int idx=0; idx<size(); idx++ )
    {
	LayerGenerator& lgen = *const_cast<LayerGenerator*>(((*this)[idx]));
	lgen.gendesc_ = this;
	if ( !lgen.reset() )
	    errmsg_ = lgen.errMsg();
    }
    return true;
}


bool Strat::LayerSequenceGenDesc::generate( Strat::LayerSequence& ls,
					    float modpos ) const
{
    errmsg_.setEmpty();

    ls.setStartDepth( startDepth(), true );
    ls.setOverburdenVelocity( overburdenVelocity() );
    const Property::EvalOpts eo( Property::EvalOpts::New, modpos );
    for ( int idx=0; idx<size(); idx++ )
    {
	const LayerGenerator& lgen = *((*this)[idx]);

	if ( !lgen.generateMaterial(ls,eo) )
	{
	    errmsg_ = lgen.errMsg();
	    if ( errmsg_.isEmpty() )
		errmsg_ = tr("Error generating %1" ).arg( lgen.name() );
	    return false;
	}
    }

    for ( int idx=size()-1; idx>=0; idx-- )
    {
	const LayerGenerator& lgen = *((*this)[idx]);
	if ( !lgen.postProcess(ls,modpos) )
	{
	    errmsg_ = lgen.errMsg();
	    if ( errmsg_.isEmpty() )
		errmsg_ = tr( "Error post-processing %1" ).arg( lgen.name() );
	    return false;
	}
    }
    ls.prepareUse();

    return true;
}


const char* Strat::LayerSequenceGenDesc::userIdentification( int unnr ) const
{
    if ( unnr >= size() )
	return sKeyIDNew;

    const BufferString unnm( (*this)[unnr]->name() );
    int dupls = 0;
    for ( int idx=0; idx<unnr; idx++ )
    {
	const BufferString nm( (*this)[idx]->name() );
	if ( nm == unnm )
	    dupls++;
    }

    mDeclStaticString( ret ); ret = unnm;
    if ( dupls > 0 )
	ret.add( " [" ).add( dupls+1 ).add( "]" );
    return ret.buf();
}


static int fetchSeqNr( char* inpnm )
{
    int seqnr = 1;
    char* ptr = firstOcc( inpnm, '[' );
    if ( ptr )
    {
	*ptr++ = '\0';
	char* endptr = firstOcc( ptr+1, ']' );
	if ( endptr ) *endptr = '\0';
	seqnr = toInt( ptr );
    }
    return seqnr > 1 ? seqnr : 1;
}


int Strat::LayerSequenceGenDesc::indexFromUserIdentification(
					const char* unm ) const
{
    BufferString unnm( unm );
    if ( unnm == sKeyIDNew )
	return size();

    const int seqnr = fetchSeqNr( unnm.getCStr() );
    int nrfound = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( unnm == (*this)[idx]->name() )
	{
	    nrfound++;
	    if ( seqnr == nrfound )
		return idx;
	}
    }

    return -1;
}


float Strat::LayerSequenceGenDesc::getOverburdenVelocity( const char* skey,
							  const IOPar& iop )
{
    BufferString abovevelstr;
    if ( !iop.get(skey,abovevelstr) || abovevelstr.isEmpty() )
	return mUdf(float);

    const FileMultiString fms( abovevelstr );
    float abovevel = mUdf(float);
    const UnitOfMeasure* veluom = UnitOfMeasure::surveyDefVelUnit();
    if ( !fms.isEmpty() )
	abovevel = fms.getFValue( 0 );

    const UnitOfMeasure* velstoruom =
			    UnitOfMeasure::surveyDefVelStorageUnit();
    if ( fms.size() > 1 )
    {
	const StringView veluomstr = fms[1];
	const UnitOfMeasure* velfileuom =
			UoMR().get( veluom->propType(), veluomstr.buf() );
	if ( velfileuom )
	    velstoruom = velfileuom;
    }

    convValue( abovevel, velstoruom, veluom );
    return abovevel;
}


void Strat::LayerSequenceGenDesc::setOverburdenVelocity( const char* skey,
						float vel, IOPar& iop )
{
    FileMultiString fms;
    fms.add( vel )
       .add( UnitOfMeasure::surveyDefVelUnit()->getLabel() );
    iop.set( skey, fms );
}


mDefineInstanceCreatedNotifierAccess(Strat::SingleLayerGenerator)

Strat::SingleLayerGenerator::SingleLayerGenerator( const LeafUnitRef* ur )
    : LayerGenerator()
    , unit_(ur)
    , content_(&Strat::Content::unspecified())
{
    props_.add( new ValueProperty(PropertyRef::thickness()) );
    mTriggerInstanceCreatedNotifier();
}


Strat::SingleLayerGenerator::SingleLayerGenerator(
					const SingleLayerGenerator& oth )
{
    *this = oth;
    mTriggerInstanceCreatedNotifier();
}


Strat::SingleLayerGenerator::~SingleLayerGenerator()
{
}


Strat::SingleLayerGenerator& Strat::SingleLayerGenerator::operator =(
					const SingleLayerGenerator& oth )
{
    if ( &oth == this )
	return *this;

    setGenDesc( getNonConst(oth.gendesc_) );
    unit_ = oth.unit_;
    props_ = oth.props_;
    content_ = oth.content_;

    return *this;
}


Strat::LayerGenerator* Strat::SingleLayerGenerator::createClone() const
{
    return new Strat::SingleLayerGenerator( *this );
}


const char* Strat::SingleLayerGenerator::name() const
{
    mDeclStaticString( ret ); ret = unit().fullCode();
    return ret.buf();
}


float Strat::SingleLayerGenerator::dispThickness( bool max ) const
{
    if ( props_.isEmpty() )
	return 1.f;

    const Property& thprop = *props_.first();
    if ( !thprop.ref().isThickness() )
    {
	pErrMsg( "Thickness should always be the first property" );
	return 1.f;
    }

    if ( !max )
	return thprop.value( mPropertyEvalAvg );

    const float th0 = thprop.value( mPropertyEvalNew(0) );
    const float th1 = thprop.value( mPropertyEvalNew(1) );
    if ( mIsUdf(th0) )
	return th1;

    if ( mIsUdf(th1) )
	return th0;

    return th0 < th1 ? th1 : th0;
}


const Strat::LeafUnitRef& Strat::SingleLayerGenerator::unit() const
{
    return unit_ ? *unit_ : RT().undefLeaf();
}


void Strat::SingleLayerGenerator::syncProps( const PropertyRefSelection& prsel )
{
    // remove old
    for ( int idx=props_.size()-1; idx>=0; idx-- )
    {
	const PropertyRef& pr = props_.get(idx)->ref();
	if ( !prsel.isPresent(&pr) )
	    delete props_.removeSingle( idx );
    }
    // add new
    for ( const auto* pr : prsel )
    {
	if ( !props_.getByName(pr->name(),false) )
	{
	    if ( pr->hasFixedDef() )
		props_.add( pr->fixedDef().clone() );
	    else
		props_.add( new ValueProperty(*pr) );
	}
    }

    //put everything in same order
    PropertySet copypropset( props_ );
    props_.erase();
    for ( const auto* pr : prsel )
    {
	const Property* copyprop = copypropset.getByName( pr->name() );
	props_.add( copyprop ? copyprop->clone() : new ValueProperty(*pr) );
    }
}


void Strat::SingleLayerGenerator::updateUsedProps(
					PropertyRefSelection& prsel ) const
{
    for ( const auto* prop : props_ )
    {
	const PropertyRef& pr = prop->ref();
	if ( !prsel.isPresent(&pr) )
	    prsel.add( &pr );
    }
}


bool Strat::SingleLayerGenerator::usePar( const IOPar& iop, const RefTree& rt )
{
    unit_ = nullptr;
    BufferString res = iop.find( sKey::Unit() );
    if ( !res.isEmpty() )
    {
	const UnitRef* ur = rt.find( res );
	if ( ur && ur->isLeaf() )
	    unit_ = static_cast<const LeafUnitRef*>( ur );
    }

    res = iop.find( sKey::Content() );
    if ( !res.isEmpty() )
    {
	content_ = rt.contents().getByName( res );
	if ( !content_ ) content_ = &Content::unspecified();
    }

    props_.erase();
    for ( int pidx=0; ; pidx++ )
    {
	PtrMan<IOPar> proppar = iop.subselect(
				IOPar::compKey(sKey::Property(),pidx) );
	if ( !proppar || proppar->isEmpty() )
	    break;

	Property* prop = Property::get( *proppar );
	if ( prop )
	    props_.set( prop );
    }

    reset();
    return true;
}


void Strat::SingleLayerGenerator::fillPar( IOPar& iop ) const
{
    LayerGenerator::fillPar( iop );
    iop.set( sKey::Unit(), unit().fullCode().buf() );

    if ( content().isUnspecified() )
	iop.removeWithKey( sKey::Content() );
    else
	iop.set( sKey::Content(), content().name() );

    for ( int iprop=0; iprop<props_.size(); iprop++ )
    {
	const auto* prop = props_.get( iprop );
	IOPar subpar;
	prop->fillPar( subpar );
	const BufferString ky( IOPar::compKey(sKey::Property(),iprop) );
	iop.mergeComp( subpar, ky );
    }
}


bool Strat::SingleLayerGenerator::reset() const
{
    if ( !props_.prepareUsage() )
	{ errmsg_ = props_.errMsg(); return false; }
    return true;
}


bool Strat::SingleLayerGenerator::genMaterial( Strat::LayerSequence& seq,
					       Property::EvalOpts eo ) const
{
    const PropertyRefSelection& prs = seq.propertyRefs();

    auto* newlay = new Layer( unit() );
    newlay->setContent( content() );
    const_cast<PropertySet*>( &props_ )->resetMemory();

    TypeSet<int> indexesofprsmath;
    TypeSet<int> correspondingidxinprops;

    // first non-Math
    for ( int ipr=0; ipr<prs.size(); ipr++ )
    {
	const PropertyRef* pr = prs[ipr];

	for ( int iprop=0; iprop<props_.size(); iprop++ )
	{
	    const auto* prop = props_.get( iprop );
	    if ( &prop->ref() != pr )
		continue;

	    if ( prop->isFormula() )
	    {
		indexesofprsmath += ipr;
		correspondingidxinprops += iprop;
	    }
	    else
	    {
		const float val = prop->value( eo );
		if (mIsUdf(val) && !prop->errMsg().isEmpty() )
		    { errmsg_ = prop->errMsg(); return false; }
		else if ( ipr == 0 && val < 1e-8 )
		    { delete newlay; return true; }

		newlay->setValue( iprop, val ) ;
		if ( iprop==0 && eo.relz_>1e-6 && !mIsUdf(eo.refz_) )
		{
		    // Lazy replacement of dummy relz_ (sequence thickness)
		    // by actual relz_
		    float reldepthfromseqtop = 0;
		    if ( !seq.isEmpty() )
			reldepthfromseqtop +=
			    (seq.layers().last()->zBot() - eo.refz_);

		    // val is thickness of the new layer;
		    reldepthfromseqtop += val/2;
		    eo.relz_ = reldepthfromseqtop / eo.relz_;
		}
	    }
	    break;
	}
    }

    // then Math
    for ( int mathidx=0; mathidx<indexesofprsmath.size(); mathidx++ )
    {
	const int ipr = indexesofprsmath[mathidx];
	const PropertyRef* pr = prs[ipr];
	const Property& prop = *props_.get( correspondingidxinprops[mathidx] );
	if ( pr != &prop.ref() )
	    { pErrMsg("Huh? should never happen"); continue; }
	if ( eo.isPrev() )
	    newlay->setValue( ipr, prop.value( eo ) );
	else
	{
	    mDynamicCastGet(const MathProperty&,mprop,prop)
	    newlay->setValue( ipr, mprop.getForm(), prs, eo );
	}
    }

    seq.layers() += newlay;
    return true;
}
