/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: attribprovider.cc,v 1.11 2005-06-02 10:37:53 cvshelene Exp $";

#include "attribprovider.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attriblinebuffer.h"
#include "attribparam.h"
#include "basictask.h"
#include "cubesampling.h"
#include "errh.h"
#include "seisreq.h"
#include "survinfo.h"
#include "threadwork.h"


namespace Attrib
{

class ProviderBasicTask : public BasicTask
{
public:

ProviderBasicTask( const Provider& p ) : provider( p )	{}

void setScope( const DataHolder* res_, const BinID& relpos_, int t0_,
	       int nrsamples_ )
{
    res = res_;
    relpos = relpos_;
    t0 = t0_;
    nrsamples = nrsamples_;
}

int nextStep()
{
    if ( !res ) return 0;
    return provider.computeData(*res,relpos,t0,nrsamples)?0:-1;
}

protected:

    const Provider&		provider;
    const DataHolder*		res;
    BinID			relpos;
    int				t0;
    int				nrsamples;

};


Provider* Provider::create( Desc& desc )
{
    ObjectSet<Provider> existing;
    bool issame = false;
    Provider* prov = internalCreate( desc, existing, issame );
    prov->computeRefZStep( existing );
    prov->propagateZRefStep( existing );
    return prov;
}


Provider* Provider::internalCreate( Desc& desc, ObjectSet<Provider>& existing, 
					bool& issame )
{
    for ( int idx=0; idx<existing.size(); idx++ )
    {
	if ( existing[idx]->getDesc().isIdenticalTo( desc, false ) )
	{
	    if ( existing[idx]->getDesc().selectedOutput() 
		    				!= desc.selectedOutput() )
		existing[idx]->enableOutput( desc.selectedOutput() );
	    issame = true;
	    return existing[idx];
	}
    }

    if ( desc.nrInputs() && !desc.descSet() )
	return 0;
    Provider* res = PF().create( desc );
    if ( !res ) return 0;

    res->ref();
    
    if ( desc.selectedOutput()!=-1 && !existing.size() )
	res->enableOutput( desc.selectedOutput(), true );

    existing += res;

    for ( int idx=0; idx<desc.nrInputs(); idx++ )
    {
	Desc* inputdesc = desc.getInput(idx);
	if ( !inputdesc ) continue;

	Provider* inputprovider = internalCreate(*inputdesc, existing, issame);
	if ( !inputprovider )
	{
	    existing.remove(existing.indexOf(res), existing.size()-1 );
	    res->unRef();
	    return 0;
	}

	if ( !issame )
	    res->setInput( idx, inputprovider );
	issame = false;
    }

    if ( !res->init() )
    {
	existing.remove(existing.indexOf(res), existing.size()-1 );
	res->unRef();
	return 0;
    }

    res->unRefNoDelete();
    return res;
}


Provider::Provider( Desc& nd )
    : desc( nd )
    , desiredvolume( 0 )
    , possiblevolume( 0 ) 
    , outputinterest( nd.nrOutputs(), 0 )
    , bufferstepout( 0, 0 )
    , threadmanager( 0 )
    , currentbid( -1, -1 )
    , curlinekey_( 0, 0 )
    , linebuffer( 0 )
    , refstep( 0 )
    , trcnr_( -1 )
    , seldata_(*new SeisSelData)
{
    desc.ref();
    inputs.allowNull(true);
    for ( int idx=0; idx<desc.nrInputs(); idx++ )
	inputs += 0;
}


Provider::~Provider()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] ) inputs[idx]->unRef();
    inputs.erase();

    desc.unRef();

    delete threadmanager;
    deepErase( computetasks );

    delete linebuffer;
    delete &seldata_;
}


bool Provider::isOK() const
{
    return true; /* Huh? &parser && parser.isOK(); */
}


Desc& Provider::getDesc()
{
    return desc;
}


const Desc& Provider::getDesc() const
{
    return const_cast<Provider*>(this)->getDesc();
}


void Provider::enableOutput( int out, bool yn )
{
    if ( out<0 || out >= outputinterest.size() )
	{ pErrMsg( "Huh?" ); return; }

    if ( yn )
	outputinterest[out]++;
    else
    {
	if ( !outputinterest[out] )
	{
	    pErrMsg( "Hue?");
	    return;
	}
	outputinterest[out]--;
    }
}


bool Provider::isOutputEnabled( int out ) const
{
    if ( out<0 || out >= outputinterest.size() )
	return false;
    else
	return outputinterest[out];
}


void Provider::setBufferStepout( const BinID& ns )
{
    if ( ns.inl <= bufferstepout.inl && ns.crl <= bufferstepout.crl )
	return;

    if ( ns.inl > bufferstepout.inl ) bufferstepout.inl = ns.inl;
    if ( ns.crl > bufferstepout.crl ) bufferstepout.crl = ns.crl;

    updateInputReqs(-1);
}


const BinID& Provider::getBufferStepout() const
{
    return bufferstepout;
}


void Provider::setDesiredVolume( const CubeSampling& ndv )
{
    if ( !desiredvolume )
	desiredvolume = new CubeSampling(ndv);
    else
	*desiredvolume = ndv;

    CubeSampling inputcs;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	for ( int idy=0; idy<outputinterest.size(); idy++ )
	{
	    if ( outputinterest[idy]<1 ) continue;

	    if ( computeDesInputCube( idx, idy, inputcs ) )
		inputs[idx]->setDesiredVolume( inputcs );
	}
    }
}


#define mGetMargin( type, var, tmpvar, tmpvarsource ) \
{ \
    type* tmpvar = tmpvarsource; \
    if ( tmpvar ) { var.start += tmpvar->start; var.stop += tmpvar->stop; } \
}

#define mGetOverallMargin( type, var, funcPost ) \
type var(0,0); \
mGetMargin( type, var, des##var, des##funcPost ); \
mGetMargin( type, var, req##var, req##funcPost )

bool Provider::getPossibleVolume( int output, CubeSampling& res )
{
    if ( inputs.size()==0 )
    {
	res.init(true);
	return true;
    }

    if ( !desiredvolume ) return false;

    TypeSet<int> outputs;
    if ( output != -1 )
	outputs += output;
    else
    {
	for ( int idx=0; idx<outputinterest.size(); idx++ )
	{
	    if ( outputinterest[idx] > 0 )
		outputs += idx;
	}
    }

    CubeSampling inputcs = res;
    bool isset = false;
    for ( int idx=0; idx<outputs.size(); idx++ )
    {
	const int out = outputs[idx];
	for ( int inp=0; inp<inputs.size(); inp++ )
	{
	    if ( !inputs[inp] )
		continue;

	    TypeSet<int> inputoutput;
	    if ( !getInputOutput( inp, inputoutput ) )
		continue;
	    
	    for ( int idy=0; idy<inputoutput.size(); idy++ )
	    {
		if ( !inputs[inp]->getPossibleVolume( idy, inputcs ) ) 
		    continue;

		const BinID* stepout = reqStepout(inp,out);
		if ( stepout )
		{
		    inputcs.hrg.start.inl += stepout->inl;
		    inputcs.hrg.start.crl += stepout->crl;
		    inputcs.hrg.stop.inl -= stepout->inl;
		    inputcs.hrg.stop.crl -= stepout->crl;
		}

		const Interval<float>* zrg = reqZMargin(inp,out);
		if ( zrg )
		{
		    inputcs.zrg.start -= zrg->start;
		    inputcs.zrg.stop -= zrg->stop;
		}

//		if ( !isset )
//		{
//		    res = inputcs;
//		    isset = true;
//		    continue;
//		}

#		define mAdjustIf(v1,op,v2) if ( v1 op v2 ) v1 = v2;
		mAdjustIf(res.hrg.start.inl,<,inputcs.hrg.start.inl);
		mAdjustIf(res.hrg.start.crl,<,inputcs.hrg.start.crl);
		mAdjustIf(res.zrg.start,<,inputcs.zrg.start);
		mAdjustIf(res.hrg.stop.inl,>,inputcs.hrg.stop.inl);
		mAdjustIf(res.hrg.stop.crl,>,inputcs.hrg.stop.crl);
		mAdjustIf(res.zrg.stop,>,inputcs.zrg.stop);
		isset = true;
	    }
	}
    }

    if ( !possiblevolume )
	possiblevolume = new CubeSampling;
    
    possiblevolume->hrg = res.hrg;
    possiblevolume->zrg = res.zrg;
    return isset;
}


int Provider::moveToNextTrace()
{

    ObjectSet<Provider> movinginputs;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	const int res = inputs[idx]->moveToNextTrace();
	if ( res!=1 ) return res;

	if ( !inputs[idx]->getSeisRequester() ) continue;
	movinginputs += inputs[idx];
    }

    if ( !movinginputs.size() )
    {
	currentbid = BinID(-1,-1);
	return true;
    }

    for ( int idx=0; idx<movinginputs.size()-1; idx++ )
    {
	for ( int idy=idx+1; idy<movinginputs.size(); idy++ )
	{
	    bool idxmoved = false;

	    while ( true )
	    {
		int compres = movinginputs[idx]->getSeisRequester()->comparePos(
				    *movinginputs[idy]->getSeisRequester() );
		if ( compres == -1 )
		{
		    idxmoved = true;
		    const int res = movinginputs[idx]->moveToNextTrace();
		    if ( res != 1 ) return res;
		}
		else if ( compres == 1 )
		{
		    const int res = movinginputs[idy]->moveToNextTrace();
		    if ( res != 1 ) return res;
		}
		else 
		    break;
	    }

	    if ( idxmoved )
	    {
		idx = -1;
		break;
	    }
	}
    }
    currentbid = movinginputs[0]->getCurrentPosition();
    if ( strcmp(curlinekey_.lineName(),"") )
	trcnr_ = movinginputs[0]-> getCurrentTrcNr();

    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	if ( !inputs[idx]->setCurrentPosition(currentbid) )
	    return -1;
    }

    return 1;
}


BinID Provider::getCurrentPosition() const
{
    return currentbid;
}


bool Provider::setCurrentPosition( const BinID& bid )
{
    if ( currentbid == BinID(-1,-1) )
	currentbid = bid;
    else if ( bid != currentbid )
    {
	pErrMsg( "Huh? (should never happen)");
	return false;
    }

    //TODO Remove old buffers
    return true;
}


void Provider::addLocalCompZIntervals( const TypeSet< Interval<int> >& ni )
{
    CubeSampling cs = *desiredvolume;
    getPossibleVolume(-1, cs);
  //  TypeSet< TypeSet<Interval<int> > >inputranges;
    Interval<int> inputranges[inputs.size()][ni.size()];

    float surveystep = SI().zRange().step;
    const float dz = (refstep==0) ? surveystep : refstep;
    int cssamplstart = (int)( cs.zrg.start / refstep );
    int cssamplstop = (int)( cs.zrg.stop / refstep );
    
    for ( int idx=0; idx<ni.size(); idx++ )
    {
	Interval<int> nigoodstep(ni[idx]);
	if ( surveystep != refstep )
	{
	    nigoodstep.start *= (int)(surveystep / refstep);
	    nigoodstep.stop *= (int)(surveystep / refstep);
	}
	
	nigoodstep.start = ( cssamplstart < nigoodstep.start ) ? 
			    nigoodstep.start : cssamplstart;
	nigoodstep.stop = ( cssamplstop > nigoodstep.stop ) ? 
			    nigoodstep.stop : cssamplstop;
	
	localcomputezintervals += nigoodstep;

	for ( int out=0; out<outputinterest.size(); out++ )
	{
	    if ( !outputinterest[out] ) continue;

	    for ( int inp=0; inp<inputs.size(); inp++ )
	    {
		if ( !inputs[inp] )
		    continue;

		Interval<int> inputrange( nigoodstep );
		Interval<float> zrg( 0, 0 );
		const Interval<float>* req = reqZMargin( inp, out );
		if ( req ) zrg = *req;
		const Interval<float>* des = desZMargin( inp, out );
		if ( des ) zrg.include( *des );

		inputrange.start += (int)(zrg.start / dz - 0.5);
		inputrange.stop += (int)(zrg.stop / dz + 0.5);

		inputranges[inp][idx] = inputrange;
	    }
	}
	for ( int inp=0; inp<inputs.size(); inp++ )
	{
	    if ( !inputs[inp] )
		continue;

	    TypeSet<Interval<int> > inpranges;
	    for ( int idinpr=0; idinpr<ni.size(); idinpr++ )
		inpranges += inputranges[inp][idinpr];
	    inputs[inp]->addLocalCompZIntervals( inpranges );
	}
    }
}


const TypeSet< Interval<int> >& Provider::localCompZIntervals() const
{
    return localcomputezintervals;
}


const DataHolder* Provider::getData( const BinID& relpos, int idi )
{
    const DataHolder* constres = getDataDontCompute(relpos);
    if ( constres && constres->t0_ == localcomputezintervals[idi].start 
	    && constres->nrsamples_ == localcomputezintervals[idi].width() )
	return constres;

    if ( !linebuffer )
	linebuffer = new DataHolderLineBuffer;
    DataHolder* outdata =
        linebuffer->createDataHolder( currentbid+relpos,
				      localcomputezintervals[idi].start,
				      localcomputezintervals[idi].width()+1 );
    if ( !outdata || !getInputData(relpos, idi) )
	return 0;
    
    for ( int idx=0; idx<outputinterest.size(); idx++ )
    {
	while ( outdata->nrItems()<=idx )
	    outdata->add();
	
	if ( outputinterest[idx]<=0 ) 
	{
	    if ( outdata->item(idx)->arr() )
	    {
		delete outdata->item(idx);
		outdata->replace( idx, 0 );
	    }

	    continue;
	}

	if ( !outdata->item(idx)->arr() )
	{
	    float* ptr = new float[outdata->nrsamples_];
	    outdata->replace( idx, new ArrayValueSeries<float>(ptr) );
	}
    }

    const int t0 = outdata->t0_;
    const int nrsamples = outdata->nrsamples_;

    bool success = false;
    if ( !threadmanager )
	success = computeData( *outdata, relpos, t0, nrsamples );
    else
    {
	if ( !computetasks.size() )
	{
	    for ( int idx=0; idx<threadmanager->nrThreads(); idx++)
		computetasks += new ProviderBasicTask(*this);
	}

	//TODO Divide task
	success = threadmanager->addWork( computetasks );
    }

    if ( !success )
    {
	linebuffer->removeDataHolder( currentbid+relpos );
	return 0;
    }

    return outdata;
}


const DataHolder* Provider::getDataDontCompute( const BinID& relpos ) const
{
    return linebuffer ? linebuffer->getDataHolder(currentbid+relpos) : 0;
}


SeisRequester* Provider::getSeisRequester()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( !inputs[idx] ) continue;
	SeisRequester* res = inputs[idx]->getSeisRequester();
	if ( res ) return res;
    }

    return 0;
}


bool Provider::init()
{
    return true;
}


bool Provider::getInputData( const BinID&, int )
{
    return true;
}


bool Provider::getInputOutput( int input, TypeSet<int>& res ) const
{
    res.erase();

    Desc* inputdesc = desc.getInput(input);
    if ( !inputdesc ) return false;

    res += inputdesc->selectedOutput();
    return true;
}


void Provider::setInput( int inp, Provider* np )
{
    if ( inputs[inp] )
    {
	TypeSet<int> inputoutputs;
	if ( getInputOutput( inp, inputoutputs ) )
	{
	    for ( int idx=0; idx<inputoutputs.size(); idx++ )
		inputs[inp]->enableOutput( inputoutputs[idx], false );
	}
	inputs[inp]->unRef();
    }

    inputs.replace( inp, np );
    if ( !inputs[inp] )
	return;

    inputs[inp]->ref();
    TypeSet<int> inputoutputs;
    if ( getInputOutput( inp, inputoutputs ) )
    {
	for ( int idx=0; idx<inputoutputs.size(); idx++ )
	    inputs[inp]->enableOutput( inputoutputs[idx], true );
    }

    updateInputReqs(inp);
    inputs[inp]->updateStorageReqs();
}


bool Provider::computeDesInputCube( int inp, int out, CubeSampling& res ) const
{
    if ( !desiredvolume )
	return false;

    res = *desiredvolume;
    int inlstepoutfact = desiredvolume->hrg.step.inl;
    int crlstepoutfact = desiredvolume->hrg.step.crl;

    BinID stepout(0,0);
    const BinID* reqstepout = reqStepout( inp, out );
    if ( reqstepout ) stepout=*reqstepout;
    const BinID* desstepout = desStepout( inp, out );
    if ( desstepout )
    {
	if ( stepout.inl < desstepout->inl ) stepout.inl = desstepout->inl;
	if ( stepout.crl < desstepout->crl ) stepout.crl = desstepout->crl;
    }

    res.hrg.start.inl -= stepout.inl * inlstepoutfact;
    res.hrg.start.crl -= stepout.crl * crlstepoutfact;
    res.hrg.stop.inl += stepout.inl * inlstepoutfact;
    res.hrg.stop.crl += stepout.crl * crlstepoutfact;

    Interval<float> zrg(0,0);
    const Interval<float>* reqzrg = reqZMargin(inp,out);
    if ( reqzrg ) zrg=*reqzrg;
    const Interval<float>* deszrg = desZMargin(inp,out);
    if ( deszrg ) zrg.include( *deszrg );
    
    res.zrg.start += zrg.start;
    res.zrg.stop += zrg.stop;

    return true;
}


void Provider::updateInputReqs(int inp)
{
    if ( inp == -1 )
    {
	for ( int idx=0; idx<inputs.size(); idx++ )
	    updateInputReqs(idx);
	return;
    }

    CubeSampling inputcs;
    for ( int out=0; out<outputinterest.size(); out++ )
    {
	if ( !outputinterest[out] ) continue;

	if ( computeDesInputCube( inp, out, inputcs ) )
	    inputs[inp]->setDesiredVolume( inputcs );

	BinID stepout(0,0);
	const BinID* req = reqStepout(inp,out);
	if ( req ) stepout=*req;
	const BinID* des = desStepout(inp,out);
	if ( des )
	{
	    stepout.inl= mMAX(stepout.inl,des->inl);
	    stepout.crl =  mMAX(stepout.crl,des->crl );
	}

	inputs[inp]->setBufferStepout( stepout+bufferstepout );
    }
}

const BinID* Provider::desStepout(int,int) const { return 0; }
const BinID* Provider::reqStepout(int,int) const { return 0; }
const Interval<float>* Provider::desZMargin(int,int) const { return 0; }
const Interval<float>* Provider::reqZMargin(int,int) const { return 0; }

int Provider::getTotalNrPos( bool is2d )
{
    if ( !possiblevolume )
	return false;
    return is2d ? possiblevolume->nrCrl()
		: possiblevolume->nrInl() * possiblevolume->nrCrl();
}


void Provider::computeRefZStep( const ObjectSet<Provider>& existing )
{
    for( int idx=0; idx<existing.size(); idx++ )
    {
	float step = 0;
//	CubeSampling storedvol;
	bool isstored = existing[idx]->getZStepStoredData(step);
	if ( isstored )
//	{
//	    existing[idx]-> getPossibleVolume(-1,storedvol)
	    refstep = ( refstep != 0 && refstep < step )? refstep : step;
	    
    }
}


void Provider::propagateZRefStep( const ObjectSet<Provider>& existing )
{
    for ( int idx=0; idx<existing.size(); idx++ )
	existing[idx]->refstep = refstep;
}

void Provider::setCurLineKey( const char* linename )
{
    curlinekey_.setLineName(linename);
    BufferString attrname;
    if ( strcmp(desc.attribName(),"Storage") )
	attrname = (BufferString)desc.attribName();
    else
    {
	const Param* idpar = desc.getParam( "id" );
	BufferString bfstr;
	if ( idpar ) 
	    idpar->getCompositeValue( bfstr );
	LineKey lk( bfstr );
	attrname = lk.attrName();
    }
    
    curlinekey_.setAttrName( (const char*)attrname);
    for ( int idx=0; idx<inputs.size(); idx++ )
    {   
	if ( !inputs[idx] ) continue;
	inputs[idx]->setCurLineKey(curlinekey_.lineName());
    }
}


void Provider::adjust2DLineStoredVolume()
{
    for ( int idx=0; idx<inputs.size(); idx++ )
	if ( inputs[idx] )
	    inputs[idx]->adjust2DLineStoredVolume();
}


void Provider::setSelData( const SeisSelData& seldata )
{
    seldata_ = seldata;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	if ( inputs[idx] )
	    inputs[idx]->setSelData(seldata);
    }
}
}; //namespace
