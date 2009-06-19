/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uislicepos.cc,v 1.3 2009-06-19 09:36:34 cvsnanne Exp $";

#include "uislicepos.h"

#include "uibutton.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uitoolbar.h"

#include "cubesampling.h"
#include "ioman.h"
#include "pixmap.h"
#include "survinfo.h"

uiSlicePos::uiSlicePos( uiParent* p )
    : positionChg(this)
{
    toolbar_ = new uiToolBar( p, "Slice position" );

    uiGroup* grp = new uiGroup( 0, "Position boxes" );
    sliceposbox_ = new uiLabeledSpinBox( grp, "Crl", 0,
	    				 "Slice position" );
    sliceposbox_->box()->valueChanging.notify(
	    			mCB(this,uiSlicePos,slicePosChg) );

    slicestepbox_ = new uiLabeledSpinBox( grp, "Step", 0, "Slice step" );
    slicestepbox_->box()->valueChanged.notify(
	    			mCB(this,uiSlicePos,sliceStepChg) );
    slicestepbox_->attach( rightTo, sliceposbox_ );

    prevbut_ = new uiToolButton( grp, "Previous position",
	    ioPixmap("prevpos.png"), mCB(this,uiSlicePos,prevCB) );
    nextbut_ = new uiToolButton( grp, "Next position",
	    ioPixmap("nextpos.png"), mCB(this,uiSlicePos,nextCB) );
    prevbut_->attach( rightTo, slicestepbox_ );
    nextbut_->attach( rightTo, prevbut_ );
    toolbar_->addObject( grp->attachObj() );

    IOM().surveyChanged.notify( mCB(this,uiSlicePos,initSteps) );
    initSteps();
}


uiSlicePos::~uiSlicePos()
{
    IOM().surveyChanged.remove( mCB(this,uiSlicePos,initSteps) );
    delete toolbar_;
}


void uiSlicePos::initSteps( CallBacker* )
{
    laststeps_[0] = SI().inlStep();
    laststeps_[1] = SI().crlStep();
    laststeps_[2] = mNINT( SI().zStep()*SI().zFactor() );
}


void uiSlicePos::setBoxLabel( Orientation orientation )
{
    if ( orientation == uiSlicePos::Inline )
	sliceposbox_->label()->setText( "Inl" );
    else if ( orientation == uiSlicePos::Crossline )
	sliceposbox_->label()->setText( "Crl" );
    else
	sliceposbox_->label()->setText( "Z" );
}


void uiSlicePos::updatePos( CallBacker* )
{
    setPosBoxValue();
}


void uiSlicePos::slicePosChanged( Orientation orientation,
				  const CubeSampling& oldcs )
{
    uiSpinBox* posbox = sliceposbox_->box();
    curcs_ = oldcs;
    if ( orientation == uiSlicePos::Inline )
	curcs_.hrg.start.inl = curcs_.hrg.stop.inl = posbox->getValue();
    else if ( orientation == uiSlicePos::Crossline )
	curcs_.hrg.start.crl = curcs_.hrg.stop.crl = posbox->getValue();
    else
	curcs_.zrg.start = curcs_.zrg.stop = posbox->getValue()/SI().zFactor();

    if ( oldcs == curcs_ )
	return;

    positionChg.trigger();
}


void uiSlicePos::sliceStepChanged( Orientation orientation )
{
    laststeps_[(int)orientation] = slicestepbox_->box()->getValue();

    sliceposbox_->box()->setStep( laststeps_[(int)orientation] );
}


void uiSlicePos::setBoxRg( Orientation orientation, const CubeSampling& survcs )
{
    uiSpinBox* posbox = sliceposbox_->box();
    uiSpinBox* stepbox = slicestepbox_->box();
    NotifyStopper posstop( posbox->valueChanging );
    NotifyStopper stepstop( stepbox->valueChanged );

    if ( orientation == uiSlicePos::Inline )
    {
	posbox->setInterval( survcs.hrg.start.inl, survcs.hrg.stop.inl );
	stepbox->setInterval( survcs.hrg.step.inl,
			      survcs.hrg.stop.inl-survcs.hrg.start.inl,
			      survcs.hrg.step.inl );
    }
    else if ( orientation == uiSlicePos::Crossline )
    {
	posbox->setInterval( survcs.hrg.start.crl, survcs.hrg.stop.crl );
	stepbox->setInterval( survcs.hrg.step.crl,
			      survcs.hrg.stop.crl-survcs.hrg.start.crl,
			      survcs.hrg.step.crl );
    }
    else
    {
	const float zfac = SI().zFactor();
	posbox->setInterval( survcs.zrg.start*zfac, survcs.zrg.stop*zfac);
	stepbox->setInterval( survcs.zrg.step*zfac,
			      (survcs.zrg.stop-survcs.zrg.start)*zfac,
			      survcs.zrg.step*zfac );
    }
}


void uiSlicePos::setPosBoxVal( Orientation orientation, const CubeSampling& cs )
{
    uiSpinBox* posbox = sliceposbox_->box();
    NotifyStopper posstop( posbox->valueChanging );

    if ( orientation == uiSlicePos::Inline )
	posbox->setValue( cs.hrg.start.inl );
    else if ( orientation == uiSlicePos::Crossline )
	posbox->setValue( cs.hrg.start.crl );
    else
	posbox->setValue( cs.zrg.start*SI().zFactor() );
}


void uiSlicePos::prevCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_->box();
    uiSpinBox* stepbox = slicestepbox_->box();
    posbox->setValue( posbox->getValue()-stepbox->getValue() );
}


void uiSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_->box();
    uiSpinBox* stepbox = slicestepbox_->box();
    posbox->setValue( posbox->getValue()+stepbox->getValue() );
}
