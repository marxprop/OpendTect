/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattrdesced.h"
#include "uiattrsel.h"
#include "uisteeringsel.h"

#include "attribdescset.h"
#include "attribdescsetman.h"
#include "attribfactory.h"
#include "attribparambase.h"
#include "attribprovider.h"
#include "attribstorprovider.h"
#include "iopar.h"
#include "survinfo.h"

using namespace Attrib;

const char* uiAttrDescEd::depthgatestr()	{ return "Depth gate"; }
const char* uiAttrDescEd::timegatestr()		{ return "Time gate"; }
const char* uiAttrDescEd::stepoutstr()		{ return "Stepout"; }
const char* uiAttrDescEd::frequencystr()	{ return "Frequency"; }
const char* uiAttrDescEd::filterszstr()		{ return "Filter size"; }

const char* uiAttrDescEd::sKeyOtherGrp()	{ return "Other"; }
const char* uiAttrDescEd::sKeyBasicGrp()	{ return "Basic"; }
const char* uiAttrDescEd::sKeyFilterGrp()	{ return "Filters"; }
const char* uiAttrDescEd::sKeyFreqGrp()	        { return "Frequency"; }
const char* uiAttrDescEd::sKeyPatternGrp()	{ return "Patterns"; }
const char* uiAttrDescEd::sKeyStatsGrp()	{ return "Statistics"; }
const char* uiAttrDescEd::sKeyPositionGrp()	{ return "Positions"; }
const char* uiAttrDescEd::sKeyDipGrp()	        { return "Dip"; }

uiString uiAttrDescEd::sInputTypeError( int inp )
{
    return tr("The suggested attribute for input %1 "
            "is incompatible with the input (wrong datatype)")
            .arg( toString( inp ) );
}



const char* uiAttrDescEd::getInputAttribName( uiAttrSel* inpfld,
					      const Desc& desc )
{
    Attrib::DescID did = inpfld->attribID();
    RefMan<Attrib::Desc> attrd = desc.descSet()->getDesc(did);

    return attrd ? attrd->attribName().buf() : "";
}


uiAttrDescEd::uiAttrDescEd( uiParent* p, bool is2d, const HelpKey& helpkey )
    : uiGroup(p)
    , helpkey_(helpkey)
    , is2d_(is2d)
    , needinpupd_(false)
{
}


uiAttrDescEd::~uiAttrDescEd()
{
}


void uiAttrDescEd::setDesc( Attrib::Desc* desc, Attrib::DescSetMan* adsm )
{
    desc_ = desc;
    adsman_ = adsm;
    if ( desc_ )
    {
	if ( adsman_ )
	    chtr_.setVar( adsman_->unSaved() );
	setParameters( *desc );
	setInput( *desc );
	setOutput( *desc );
    }
}


void uiAttrDescEd::setZDomainInfo( const ZDomain::Info* info )
{
    zdomaininfo_ = info;
}


const ZDomain::Info* uiAttrDescEd::getZDomainInfo() const
{
    return zdomaininfo_;
}


void uiAttrDescEd::setDataPackInp( const TypeSet<DataPack::FullID>& ids )
{
    dpfids_ = ids;

    if ( desc_ )
	setInput( *desc_ );
}


void uiAttrDescEd::fillInp( uiAttrSel* fld, Attrib::Desc& desc, int inp )
{
    if ( inp >= desc.nrInputs() || !desc.descSet() )
	return;

    fld->processInput();
    const DescID attribid = fld->attribID();

    ConstRefMan<Attrib::Desc> inpdesc = desc.getInput( inp );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), attribid );
    else
	chtr_.setChanged( true );

    if ( !desc.setInput(inp,desc.descSet()->getDesc(attribid).ptr()) )
    {
	errmsg_ = tr("The suggested attribute for input %1 "
                      "is incompatible with the input (wrong datatype)")
                      .arg( toString(inp) );
    }

    mDynamicCastGet(const uiImagAttrSel*,imagfld,fld)
    if ( imagfld )
	desc.setInput( inp+1, desc.descSet()->getDesc(imagfld->imagID()).ptr());
}


void uiAttrDescEd::fillInp( uiSteeringSel* fld, Attrib::Desc& desc, int inp )
{
    if ( inp >= desc.nrInputs() )
	return;

    const DescID descid = fld->descID();
    ConstRefMan<Attrib::Desc> inpdesc = desc.getInput( inp );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), descid );
    else if ( fld->willSteer() )
	chtr_.setChanged( true );

    if ( !desc.setInput( inp, desc.descSet()->getDesc(descid).ptr() ) )
    {
	errmsg_ = sInputTypeError( inp );
    }
}


void uiAttrDescEd::fillInp( uiSteerAttrSel* fld, Attrib::Desc& desc, int inp )
{
    if ( inp >= desc.nrInputs() )
	return;

    fld->processInput();
    const DescID inlid = fld->inlDipID();
    ConstRefMan<Attrib::Desc> inpdesc = desc.getInput( inp );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), inlid );
    else
	chtr_.setChanged( true );

    if ( !desc.setInput( inp, desc.descSet()->getDesc(inlid).ptr() ) )
    {
        errmsg_ = sInputTypeError( inp );
    }

    const DescID crlid = fld->crlDipID();
    inpdesc = desc.getInput( inp+1 );
    if ( inpdesc )
	chtr_.set( inpdesc->id(), crlid );
    else
	chtr_.setChanged( true );

    desc.setInput( inp+1, desc.descSet()->getDesc(crlid).ptr() );
}


void uiAttrDescEd::fillOutput( Attrib::Desc& desc, int selout )
{
    if ( chtr_.set(desc.selectedOutput(),selout) )
	desc.selectOutput( selout );
}


uiAttrSel* uiAttrDescEd::createInpFld( bool is2d, const uiString& txt )
{
    uiAttrSelData asd( is2d );
    return new uiAttrSel( this, asd.attrSet(), txt, asd.attribid_ );
}


uiAttrSel* uiAttrDescEd::createInpFld( const uiAttrSelData& asd,
				       const uiString& txt )
{
    return new uiAttrSel( this, txt, asd );
}


uiImagAttrSel* uiAttrDescEd::createImagInpFld( bool is2d )
{
    uiAttrSelData asd( is2d );
    return new uiImagAttrSel( this, uiString::empty(), asd);
}


void uiAttrDescEd::putInp( uiAttrSel* inpfld, const Attrib::Desc& ad,
			   int inpnr )
{
    if ( dpfids_.size() )
	inpfld->setPossibleDataPacks( dpfids_ );

    ConstRefMan<Attrib::Desc> inpdesc = ad.getInput( inpnr );
    if ( !inpdesc )
    {
	if ( needinpupd_ )
	{
	    Attrib::DescID defaultdid = ad.descSet()->ensureDefStoredPresent();
	    RefMan<Attrib::Desc> defaultdesc =
				ad.descSet()->getDesc( defaultdid );
	    if ( !defaultdesc )
		inpfld->setDescSet( ad.descSet() );

	    inpfld->setDesc( defaultdesc.ptr() );
	}
	else
	    inpfld->setDescSet( ad.descSet() );
    }
    else
    {
	inpfld->setDesc( inpdesc.ptr() );
	if ( adsman_ )
	    inpfld->updateHistory( adsman_->inputHistory() );
    }
}


void uiAttrDescEd::putInp( uiSteerAttrSel* inpfld, const Attrib::Desc& ad,
			   int inpnr )
{
    ConstRefMan<Attrib::Desc> inpdesc = ad.getInput( inpnr );
    if ( !inpdesc )
        inpfld->setDescSet( ad.descSet() );
    else
    {
	inpfld->setDesc( inpdesc.ptr() );
	if ( adsman_ )
	    inpfld->updateHistory( adsman_->inputHistory() );
    }
}


void uiAttrDescEd::putInp( uiSteeringSel* inpfld, const Attrib::Desc& ad,
			   int inpnr )
{
    ConstRefMan<Attrib::Desc> inpdesc = ad.getInput( inpnr );
    if ( !inpdesc )
	inpfld->setDescSet( ad.descSet() );
    else
	inpfld->setDesc( inpdesc.ptr() );
}


const char* uiAttrDescEd::zGateLabel() const
{
    return SI().zIsTime() ? timegatestr() : depthgatestr();
}


uiString uiAttrDescEd::zDepLabel( const uiString& pre,
				  const uiString& post ) const
{
    BufferString lbl;
    uiString zstr( zIsTime() ? uiStrings::sTime() : uiStrings::sDepth() );
    uiString ret;
    if ( !pre.isEmpty() && !post.isEmpty() )
    {
	return toUiString( "%1 %2 %3 %4" )
	    .arg( pre )
	    .arg( zstr.toLower() )
	    .arg( post )
	    .arg( SI().getUiZUnitString() );
    }

    if ( !pre.isEmpty() )
    {
	zstr.toLower( true );
	return uiStrings::phrJoinStrings( pre, zstr, SI().getUiZUnitString() );
    }

    if ( !post.isEmpty() )
	return uiStrings::phrJoinStrings( zstr, post, SI().getUiZUnitString());


    return uiStrings::phrJoinStrings( zstr, SI().getUiZUnitString() );
}


bool uiAttrDescEd::zIsTime() const
{
    return SI().zIsTime();
}


uiString uiAttrDescEd::errMsgStr( Attrib::Desc* desc )
{
    if ( !desc )
	return uiStrings::sEmptyString();

    errmsg_.setEmpty();

    if ( desc->isSatisfied() == Desc::Error )
    {
	const uiString derrmsg( desc->errMsg() );
	if ( !desc->isStored()
		|| derrmsg.getFullString() != DescSet::storedIDErrStr() )
	    errmsg_ = derrmsg;
	else
	{
	    errmsg_ = tr("Cannot find stored data %1.\n"
                         "Data might have been deleted or corrupted.\n"
                         "Please select valid stored data as input.")
                         .arg( desc->userRef() );
	}
    }

    const bool isuiok = areUIParsOK();
    if ( !isuiok && errmsg_.isEmpty() )
	errmsg_= tr("Please review your parameters, "
                    "some of them are not correct");

    return errmsg_;
}


uiString uiAttrDescEd::commit( Attrib::Desc* editdesc )
{
    if ( !editdesc )
	editdesc = desc_;
    if ( !editdesc )
	return uiStrings::sEmptyString();

    getParameters( *editdesc );
    errmsg_ = Provider::prepare( *editdesc );
    editdesc->updateParams();	//needed before getInput to set correct input nr
    if ( !getInput(*editdesc) || !getOutput(*editdesc) )
	return uiStrings::sEmptyString();

    editdesc->updateParams();	//needed after getInput to update inputs' params

    return errMsgStr( editdesc );
}


bool uiAttrDescEd::getOutput( Attrib::Desc& desc )
{
    desc.selectOutput( 0 );
    return true;
}


bool uiAttrDescEd::getInputDPID( uiAttrSel* inpfld,
				 DataPack::FullID& inpdpfid ) const
{
    const StringPair input( inpfld->getInput() );
    for ( int idx=0; idx<dpfids_.size(); idx++ )
    {
	DataPack::FullID dpfid = dpfids_[idx];
	BufferString dpnm = DataPackMgr::nameOf( dpfid );
	if ( input.first() == dpnm )
	{
	    inpdpfid = dpfid;
	    return true;
	}
    }

    return false;
}


RefMan<Desc> uiAttrDescEd::getInputDescFromDP( uiAttrSel* inpfld ) const
{
    if ( !dpfids_.size() )
    {
	pErrMsg( "No datapacks present to form Desc" );
	return nullptr;
    }

    DataPack::FullID inpdpfid;
    if ( !getInputDPID(inpfld,inpdpfid) )
	return nullptr;

    RefMan<Desc> inpdesc =
		Attrib::PF().createDescCopy( StorageProvider::attribName());
    Attrib::ValParam* param =
	inpdesc->getValParam( Attrib::StorageProvider::keyStr() );
    param->setValue( inpdpfid.asMultiID() );
    return inpdesc;
}
