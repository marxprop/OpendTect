/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicalcpoly2horvol.h"
#include "poly2horvol.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "od_helpids.h"
#include "pickset.h"
#include "picksettr.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "veldesc.h"

#include "uibutton.h"
#include "uichecklist.h"
#include "uiconstvel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiunitsel.h"

#include <math.h>

#include "hiddenparam.h"

class HP_uiCalcHorVol
{
mOD_DisableCopy(HP_uiCalcHorVol)
public:
HP_uiCalcHorVol()	{}
~HP_uiCalcHorVol()	{}

    uiGenInput*		areafld_		= nullptr;
    uiUnitSel*		areaunitfld_		= nullptr;
    uiUnitSel*		volumeunitfld_		= nullptr;

    float		areainm2_		= mUdf(float);
    float		volumeinm3_		= mUdf(float);
};

static HiddenParam<uiCalcHorVol,HP_uiCalcHorVol*> hp( nullptr );


uiCalcHorVol::uiCalcHorVol( uiParent* p,const uiString& dlgtxt )
	: uiDialog(p,Setup(tr("Calculate Volume"),dlgtxt,
			   mODHelpKey(mCalcPoly2HorVolHelpID)))
	, zinft_(SI().depthsInFeet())
{
    setCtrlStyle( CloseOnly );

    hp.setParam( this, new HP_uiCalcHorVol() );
}


uiCalcHorVol::~uiCalcHorVol()
{
    detachAllNotifiers();
    hp.removeAndDeleteParam( this );
}


uiGroup* uiCalcHorVol::mkStdGrp()
{
    const CallBack calccb( mCB(this,uiCalcHorVol,calcReq) );

    uiGroup* grp = new uiGroup( this, "uiCalcHorVol group" );

    optsfld_ = new uiCheckList( grp );
    optsfld_->addItem( tr("Ignore negative thicknesses") )
	     .addItem( tr("Upward") );
    optsfld_->setChecked( 0, true ).setChecked( 1, true );

    uiObject* attachobj = optsfld_->attachObj();
    if ( SI().zIsTime() )
    {
	velfld_ = new uiGenInput( grp, VelocityDesc::getVelVolumeLabel(),
		FloatInpSpec(Vel::getGUIDefaultVelocity()) );
	velfld_->attach( alignedBelow, optsfld_ );
	velfld_->valuechanged.notify( calccb );
	attachobj = velfld_->attachObj();
    }

    auto* calcbut = new uiPushButton( grp, uiStrings::sCalculate(),
				      calccb, true);
    calcbut->setIcon( "downarrow" );
    calcbut->attach( alignedBelow, attachobj );

    auto* sep = new uiSeparator( grp, "Hor sep" );
    sep->attach( stretchedBelow, calcbut );

    auto* areafld = new uiGenInput( grp, tr("Area") );
    areafld->attach( alignedBelow, calcbut );
    areafld->attach( ensureBelow, sep );
    areafld->setReadOnly( true );

    auto* areaunitfld = new uiUnitSel( grp, Mnemonic::Area );
    areaunitfld->setUnit( UoMR().get(SI().depthsInFeet() ? "mi2" : "km2"));
    mAttachCB( areaunitfld->selChange, uiCalcHorVol::unitChgCB );
    areaunitfld->attach( rightOf, areafld );

    valfld_ = new uiGenInput( grp, tr("Volume") );
    valfld_->setReadOnly( true );
    valfld_->attach( alignedBelow, areafld );

    auto* volumeunitfld = new uiUnitSel( grp, Mnemonic::Vol );
    volumeunitfld->setUnit( UoMR().get(SI().depthsInFeet() ? "ft3" : "m3") );
    mAttachCB( volumeunitfld->selChange, uiCalcHorVol::unitChgCB );
    volumeunitfld->attach( rightOf, valfld_ );

    auto* extra = hp.getParam( this );
    extra->areafld_ = areafld;
    extra->areaunitfld_ = areaunitfld;
    extra->volumeunitfld_ = volumeunitfld;

    grp->setHAlignObj( valfld_ );
    return grp;
}


void uiCalcHorVol::unitChgCB( CallBacker* cb )
{
    CallBacker* caller = cb ? cb->trueCaller() : nullptr;
    auto* extra = hp.getParam( this );
    BufferString txt;
    if ( caller && caller==extra->volumeunitfld_ )
    {
	const UnitOfMeasure* uom = extra->volumeunitfld_->getUnit();
	if ( uom && !mIsUdf(extra->volumeinm3_) )
	{
	    const float newval = uom->getUserValueFromSI( extra->volumeinm3_ );
	    txt.set( newval, 4 );
	}

	valfld_->setText( txt );
    }
    else if ( caller && caller==extra->areaunitfld_ )
    {
	const UnitOfMeasure* uom = extra->areaunitfld_->getUnit();
	if ( uom && !mIsUdf(extra->areainm2_) )
	{
	    const float newval = uom->getUserValueFromSI( extra->areainm2_ );
	    txt.set( newval, 4 );
	}

	extra->areafld_->setText( txt );
    }
}


void uiCalcHorVol::haveChg( CallBacker* )
{
    if ( valfld_ )
	valfld_->clear();
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiCalcHorVol::calcReq( CallBacker* )
{
    const Pick::Set* ps = getPickSet();
    if ( !ps ) mErrRet( tr("No Polygon selected") );

    const EM::Horizon3D* hor = getHorizon();
    if ( !hor ) mErrRet( tr("No Horizon selected") );

    float vel = 1;
    if ( velfld_ )
    {
	vel = velfld_->getFValue();
	if ( mIsUdf(vel) || vel < 0.1 )
	    mErrRet(tr("Please provide the velocity"))
	if ( zinft_ )
	    vel *= mFromFeetFactorF;
    }

    auto* extra = hp.getParam( this );
    const bool allownegativevalues = !optsfld_->isChecked( 0 );
    const bool upward = optsfld_->isChecked( 1 );
    Poly2HorVol ph2v( ps, const_cast<EM::Horizon3D*>(hor) );
    extra->volumeinm3_ = ph2v.getM3( vel, upward, allownegativevalues );
    unitChgCB( extra->volumeunitfld_ );

    extra->areainm2_ = ps->getXYArea(); // This area is in XYUnits^2
    if ( SI().xyInFeet() )
	extra->areainm2_ *= mFromFeetFactorF * mFromFeetFactorF;

    unitChgCB( extra->areaunitfld_ );
}



// uiCalcPolyHorVol
uiCalcPolyHorVol::uiCalcPolyHorVol( uiParent* p, const Pick::Set& ps )
	: uiCalcHorVol(p,tr("Polygon: %1").arg(ps.name()))
	, ps_(&ps)
{
    if ( ps_->size() < 3 )
    {
	new uiLabel( this, uiStrings::phrInvalid(uiStrings::sPolygon()) );
	return;
    }

    horsel_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D),
				tr("Calculate to") );
    horsel_->selectionDone.notify( mCB(this,uiCalcPolyHorVol,horSel) );

    mkStdGrp()->attach( alignedBelow, horsel_ );
}


uiCalcPolyHorVol::~uiCalcPolyHorVol()
{
}


void uiCalcPolyHorVol::horSel( CallBacker* cb )
{
    const IOObj* ioobj = horsel_->ioobj( true );
    if ( !ioobj )
	return;

    uiTaskRunner taskrunner( this );
    EM::EMObject* emobj =
		EM::EMM().loadIfNotFullyLoaded( ioobj->key(), &taskrunner );
    hor_ = sCast(EM::Horizon3D*,emobj);
    haveChg( cb );
}



// uiCalcHorPolyVol
uiCalcHorPolyVol::uiCalcHorPolyVol( uiParent* p, const EM::Horizon3D& h )
    : uiCalcHorVol(p,tr("Horizon: %1").arg(h.name()))
    , hor_(&h)
{
    if ( hor_->nrSections() < 1 )
    {
	new uiLabel( this, uiStrings::phrInvalid(uiStrings::sHorizon(1)));
	return;
    }

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.toselect_.require_.set( sKey::Type(), sKey::Polygon() );
    pssel_ = new uiIOObjSel( this, ctxt, uiStrings::phrCalculateFrom(
			     uiStrings::sPolygon()));
    pssel_->selectionDone.notify( mCB(this,uiCalcHorPolyVol,psSel) );

    mkStdGrp()->attach( alignedBelow, pssel_ );
}


uiCalcHorPolyVol::~uiCalcHorPolyVol()
{
}


void uiCalcHorPolyVol::psSel( CallBacker* cb )
{
    const IOObj* ioobj = pssel_->ioobj( true );
    if ( !ioobj )
	return;

    BufferString msg;
    ps_ = new Pick::Set;
    if ( !PickSetTranslator::retrieve(*ps_,ioobj,false,msg) )
	uiMSG().error( mToUiStringTodo(msg) );

    haveChg( cb );
}
