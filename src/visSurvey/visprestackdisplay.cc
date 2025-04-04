/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visprestackdisplay.h"

#include "flatposdata.h"
#include "ioman.h"
#include "iopar.h"
#include "oscommand.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "prestackprocessor.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "sorting.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"
#include "unitofmeasure.h"
#include "visdataman.h"
#include "vismaterial.h"

#include <math.h>

#define mDefaultWidth ((SI().inlDistance() + SI().crlDistance() ) * 100)

static const char* sKeyMultiID()	{ return "Data ID"; }
static const char* sKeyTraceNr()	{ return "TraceNr"; }

namespace visSurvey
{

PreStackDisplay::PreStackDisplay()
    : visBase::VisualObjectImpl(true)
    , draggermoving(this)
    , width_(mDefaultWidth)
    , offsetrange_(0,mDefaultWidth)
    , zrg_(SI().zRange(true))
    , movefinished_(this)
{
    ref();
    planedragger_ = visBase::DepthTabPlaneDragger::create();
    flatviewer_ = visBase::FlatViewer::create();
    setMaterial( nullptr );

    flatviewer_->enableTraversal( visBase::cDraggerIntersecTraversalMask(),
				  false);
    flatviewer_->setSelectable( false );
    flatviewer_->appearance().setGeoDefaults( true );
    flatviewer_->getMaterial()->setDiffIntensity( 0.2 );
    flatviewer_->getMaterial()->setAmbience( 0.8 );
    flatviewer_->appearance().ddpars_.vd_.mappersetup_.symmidval_ = 0;
    mAttachCB( flatviewer_->dataChanged, PreStackDisplay::dataChangedCB );
    addChild( flatviewer_->osgNode() );

    planedragger_->removeScaleTabs();
    mAttachCB( planedragger_->motion, PreStackDisplay::draggerMotion );
    mAttachCB( planedragger_->finished, PreStackDisplay::finishedCB );
    addChild( planedragger_->osgNode() );
    unRefNoDelete();
}


PreStackDisplay::~PreStackDisplay()
{
    detachAllNotifiers();
    delete reader_;
    delete ioobj_;
    delete preprocmgr_;
}


void PreStackDisplay::setSceneEventCatcher( visBase::EventCatcher* evcatcher )
{
    if ( eventcatcher_ )
	mDetachCB( eventcatcher_->eventhappened,
		   PreStackDisplay::updateMouseCursorCB );

    eventcatcher_ = evcatcher;

    if ( eventcatcher_ )
	mAttachCB( eventcatcher_->eventhappened,
		   PreStackDisplay::updateMouseCursorCB );
}


void PreStackDisplay::updateMouseCursorCB( CallBacker* cb )
{
    if ( !planedragger_ || !isOn() || isLocked() )
	mousecursor_.shape_ = MouseCursor::NotSet;
    else
	initAdaptiveMouseCursor( cb, id(), mUdf(int), mousecursor_ );
}


void PreStackDisplay::allowShading( bool yn )
{
    flatviewer_->allowShading( yn );
}


BufferString PreStackDisplay::getObjectName() const
{
    return ioobj_ ? ioobj_->name().buf() : "";
}


void PreStackDisplay::setMultiID( const MultiID& mid )
{
    mid_ = mid;
    delete ioobj_;
    ioobj_ = IOM().get( mid_ );
    deleteAndNullPtr( reader_ );
    if ( !ioobj_ )
	return;

    if ( section_ )
	reader_ = SPSIOPF().get3DReader( *ioobj_ );
    else if ( seis2d_ )
	reader_ = SPSIOPF().get2DReader( *ioobj_, seis2d_->getGeomID() );

    if ( !reader_ )
	return;

    if ( seis2d_ )
    {
	mAttachCB( seis2d_->getMovementNotifier(),
		   PreStackDisplay::seis2DMovedCB );
    }

    if ( section_ )
    {
	mAttachCB( section_->getMovementNotifier(),
		   PreStackDisplay::sectionMovedCB );
    }
}


ConstRefMan<PreStack::Gather> PreStackDisplay::getProcessedGather()
{
    if ( !ioobj_ || !reader_ )
	return nullptr;

    if ( !preprocmgr_ )
	setProcMgr( is3DSeis() ? OD::Geom3D : OD::Geom2D );

    if ( !preprocmgr_->nrProcessors() || !preprocmgr_->reset() )
	return nullptr;

    if ( !preprocmgr_->prepareWork() )
	return nullptr;

    const BinID stepout = preprocmgr_->getInputStepout();

    BinID relbid;
    for ( relbid.inl()=-stepout.inl(); relbid.inl()<=stepout.inl();
				       relbid.inl()++ )
    {
	for ( relbid.crl()=-stepout.crl(); relbid.crl()<=stepout.crl();
					   relbid.crl()++ )
	{
	    if ( !preprocmgr_->wantsInput(relbid) )
		continue;

	    TrcKey tk;
	    if ( is3DSeis() )
	    {
		const BinID inputbid = bid_ +
				relbid * BinID(SI().inlStep(),SI().crlStep());
		tk.setPosition( inputbid );
	    }
	    else
	    {
		const int trcnr = trcnr_ + relbid.trcNr();
		tk.setGeomID( seis2d_->getGeomID() ).setTrcNr( trcnr );
	    }

	    RefMan<PreStack::Gather> gather = new PreStack::Gather;
	    if ( !gather->readFrom(*ioobj_,*reader_,tk) )
		continue;

	    preprocmgr_->setInput( relbid, gather.ptr() );
	}
    }

    if ( !preprocmgr_->process() )
	return nullptr;

    return preprocmgr_->getOutput();
}


void PreStackDisplay::setProcMgr( OD::GeomSystem gs )
{
    delete preprocmgr_;
    preprocmgr_ = new PreStack::ProcessManager( gs );
    if ( !preprociop_.isEmpty() )
	preprocmgr_->usePar( preprociop_ );
}


void PreStackDisplay::setProcPar( const IOPar& par )
{
    preprociop_ = par;
    if ( preprocmgr_ )
	preprocmgr_->usePar( par );
}


void PreStackDisplay::getProcPar( IOPar& par )
{
    if ( preprocmgr_ )
	preprocmgr_->fillPar( par );
    else
	par.merge( preprociop_ );
}


bool PreStackDisplay::setPosition( const TrcKey& tk )
{
    if ( !tk.is3D() )
	{ pErrMsg("Incorrect TrcKey type"); }

    if ( bid_ == tk.position() )
	return true;

    bid_ = tk.position();

    gather_ = new PreStack::Gather;
    if ( !ioobj_ || !reader_ || !gather_->readFrom(*ioobj_,*reader_,tk) )
    {
	mDefineStaticLocalObject( bool, shown3d, = false );
	mDefineStaticLocalObject( bool, resetpos, = true );
	if ( !shown3d )
	{
	    resetpos = true;
	    shown3d = true;
	}

	bool hasdata = false;
	if ( resetpos )
	{
	    BinID nearbid = getNearBinID( bid_ );
	    if ( nearbid.isUdf() )
	    {
		const StepInterval<int> rg = getTraceRange( bid_, false );
		BufferString msg( "No gather data at the whole section.\n" );
                msg.add( "Data available at: ").add( rg.start_ ).add( " - " )
                        .add( rg.stop_ ).add( " - " ).add( rg.step_ );
		OD::DisplayErrorMessage( msg );
	    }
	    else
	    {
		bid_ = nearbid;
		hasdata = true;
	    }
	}
	else
	    OD::DisplayErrorMessage( "No gather data at the picked location." );

	if ( !hasdata )
	{
	    flatviewer_->appearance().annot_.x1_.showgridlines_ = false;
	    flatviewer_->appearance().annot_.x2_.showgridlines_ = false;
	    flatviewer_->turnOnGridLines( false, false );
	}
    }

    draggerpos_ = bid_;
    draggermoving.trigger();
    dataChangedCB( nullptr );
    return updateData();
}


bool PreStackDisplay::updateData()
{
    if ( (is3DSeis() && bid_.isUdf()) ||
	 (!is3DSeis() && !seis2d_) || !ioobj_ || !reader_ )
    {
	turnOn(false);
	return true;
    }

    const bool haddata = flatviewer_->hasPack( false );
    if ( preprocmgr_ && preprocmgr_->nrProcessors() )
    {
	gather_ = getProcessedGather().getNonConstPtr();
    }
    else
    {
	TrcKey tk;
	if ( is3DSeis() )
	    tk.setPosition( bid_ );
	else
	    tk.setGeomID( seis2d_->getGeomID() ).setTrcNr( trcnr_ );

	if ( !gather_ || gather_->getTrcKey()!=tk )
	    gather_ = new PreStack::Gather;
    }

    if ( gather_ )
    {
	const bool canupdate = flatviewer_->enableChange( false );
	if ( !flatviewer_->zDomain(false) )
	{
	    const ZDomain::Info& datazdom = gather_->zDomain();
	    const ZDomain::Info* displayzdom = datazdom.isDepth()
					? &ZDomain::DefaultDepth( true )
					: nullptr;
	    flatviewer_->setZDomain( datazdom, false );
	    if ( displayzdom )
		flatviewer_->setZDomain( *displayzdom, true );
	}

	flatviewer_->setVisible( FlatView::Viewer::VD, true );
	flatviewer_->enableChange( canupdate );
	flatviewer_->setPack( FlatView::Viewer::VD, gather_.ptr(), !haddata );
    }
    else
    {
	if ( haddata )
	{
	    const bool canupdate = flatviewer_->enableChange( false );
	    flatviewer_->setVisible( FlatView::Viewer::VD, false );
	    flatviewer_->enableChange( canupdate );
	    flatviewer_->setPack( FlatView::Viewer::VD, nullptr );
	}
	else
	    dataChangedCB( nullptr );

	return false;
    }

    turnOn( true );
    return true;
}


StepInterval<int> PreStackDisplay::getTraceRange( const BinID& bid,
						  bool oncurrentline ) const
{
    if ( is3DSeis() )
    {
	mDynamicCastGet( SeisPS3DReader*, rdr3d, reader_ );
	if ( !rdr3d )
	    return StepInterval<int>(mUdf(int),mUdf(int),1);

	const PosInfo::CubeData& posinfo = rdr3d->posData();
	const bool docrlrg = (isOrientationInline() && oncurrentline) ||
			     (!isOrientationInline() && !oncurrentline);
	if ( docrlrg )
	{
	    const int inlidx = posinfo.indexOf( bid.inl() );
	    if ( inlidx==-1 )
		return StepInterval<int>(mUdf(int),mUdf(int),1);

	    const int seg = posinfo[inlidx]->nearestSegment( bid.crl() );
	    return posinfo[inlidx]->segments_[seg];
	}
	else
	{
	    StepInterval<int> res;
	    posinfo.getInlRange( res );
	    return res;
	}
    }
    else
    {
	mDynamicCastGet( SeisPS2DReader*, rdr2d, reader_ );
	if ( !seis2d_ || !rdr2d )
	    return StepInterval<int>(mUdf(int),mUdf(int),1);

	const TypeSet<PosInfo::Line2DPos>& posnrs
					    = rdr2d->posData().positions();
	const int nrtraces = posnrs.size();
	if ( !nrtraces )
	     return StepInterval<int>(mUdf(int),mUdf(int),1);

	mAllocVarLenArr( int, trcnrs, nrtraces );

	for ( int idx=0; idx<nrtraces; idx++ )
	    trcnrs[idx] = posnrs[idx].nr_;

	quickSort( mVarLenArr(trcnrs), nrtraces );
	const int trstep = nrtraces>1 ? trcnrs[1]-trcnrs[0] : 0;
	return StepInterval<int>( trcnrs[0], trcnrs[nrtraces-1], trstep );
    }
}


BinID PreStackDisplay::getNearBinID( const BinID& bid ) const
{
    const StepInterval<int> tracerg = getTraceRange( bid );
    if ( tracerg.isUdf() )
	return BinID::udf();

    BinID res = bid;
    if ( isOrientationInline() )
    {
	res.crl() = bid.crl()<tracerg.start_
		  ? tracerg.start_
		  : ( bid.crl()>tracerg.stop_ ? tracerg.stop_
					      : tracerg.snap(bid.crl()) );
    }
    else
    {
	res.inl() = bid.inl()<tracerg.start_
		  ? tracerg.start_
		  : ( bid.inl()>tracerg.stop_ ? tracerg.stop_
					      : tracerg.snap(bid.inl()) );
    }

    return res;
}


int PreStackDisplay::getNearTraceNr( int trcnr ) const
{
    mDynamicCastGet(SeisPS2DReader*, rdr2d, reader_ );
    if ( !rdr2d )
	return -1;

    const TypeSet<PosInfo::Line2DPos>& posnrs = rdr2d->posData().positions();
    if ( posnrs.isEmpty() )
	return -1;

    int mindist=-1, residx=0;
    for ( int idx=0; idx<posnrs.size(); idx++ )
    {
	const int dist = abs( posnrs[idx].nr_ - trcnr );
	if ( mindist==-1 || mindist>dist )
	{
	    mindist = dist;
	    residx = idx;
	}
    }

   return posnrs[residx].nr_;
}


void PreStackDisplay::displaysAutoWidth( bool yn )
{
    if ( autowidth_ == yn )
	return;

    autowidth_ = yn;
    dataChangedCB( nullptr );
}


void PreStackDisplay::displaysOnPositiveSide( bool yn )
{
    if ( posside_ == yn )
	return;

    posside_ = yn;
    dataChangedCB( nullptr );
}


void PreStackDisplay::setFactor( float scale )
{
    if ( factor_ == scale )
	return;

    factor_ = scale;
    dataChangedCB( nullptr );
}

void PreStackDisplay::setWidth( float width )
{
    if ( width_ == width )
	return;

    width_ = width;
    dataChangedCB( nullptr );
}


void PreStackDisplay::dataChangedCB( CallBacker* )
{
    if ( (!section_ && !seis2d_) || factor_<0 || width_<0 )
	return;

    if ( section_ && bid_.isUdf() )
	return;

    const Coord direction = posside_ ? basedirection_ : -basedirection_;
    const double offsetscale =
			Coord( basedirection_.x_*SI().inlDistance(),
			       basedirection_.y_*SI().crlDistance() ).abs();

    ConstRefMan<FlatDataPack> fdp = flatviewer_->getPack( false ).get();
    int nrtrcs = 0;
    if ( fdp && fdp->isOK() )
    {
	offsetrange_.setFrom( fdp->posData().range( true ) );
	zrg_.setFrom( fdp->posData().range( false ) );
	nrtrcs = fdp->size( true );
    }

    if ( nrtrcs < 2 )
	offsetrange_.set( 0.f, mDefaultWidth );
    else
    {
	const float inltrcdist = SI().inlDistance() * SI().inlStep();
	const float crltrcdist = SI().crlDistance() * SI().crlStep();
	const float minwidth = (nrtrcs - 1) * (inltrcdist + crltrcdist);
	if ( offsetrange_.width() < minwidth )
	    offsetrange_.set( 0.f, minwidth );
    }

    Coord startpos( bid_.inl(), bid_.crl() );
    if ( seis2d_ )
	startpos = seis2dpos_;

    const Coord stoppos = autowidth_
	? startpos + direction*offsetrange_.width()*factor_ / offsetscale
	: startpos + direction*width_ / offsetscale;

    if ( seis2d_ )
	seis2dstoppos_ = stoppos;

    if ( autowidth_ )
	width_ = offsetrange_.width()*factor_;
    else
	factor_ = width_/offsetrange_.width();

    const Coord3 c00( startpos, zrg_.start_ );
    const Coord3 c01( startpos, zrg_.stop_ );
    const Coord3 c11( stoppos, zrg_.stop_ );
    const Coord3 c10( stoppos, zrg_.start_ );

    flatviewer_->setPosition( c00, c01, c10, c11 );

    Interval<float> xlim( mCast(float, SI().inlRange(true).start_),
                          mCast(float, SI().inlRange(true).stop_) );
    Interval<float> ylim( mCast(float, SI().crlRange(true).start_),
                          mCast(float, SI().crlRange(true).stop_) );

    bool isinline = true;
    if ( section_ )
    {
	isinline = section_->getOrientation()==OD::SliceType::Inline;
	if ( isinline )
	{
            xlim.set( mCast(float,startpos.x_), mCast(float,stoppos.x_) );
	    xlim.sort();
	}
	else
	{
            ylim.set( mCast(float,startpos.y_), mCast(float,stoppos.y_) );
	    ylim.sort();
	}
    }
    else if ( seis2d_ && !seis2d_->getGeometry().positions().isEmpty() )
    {
	const Coord startpt = seis2d_->getGeometry().positions().first().coord_;
	const Coord stoppt = seis2d_->getGeometry().positions().last().coord_;
	const BinID startbid = SI().transform( startpt );
	const BinID stopbid = SI().transform( stoppt );
	const BinID diff = stopbid - startbid;
	isinline = Math::Abs(diff.inl()) < Math::Abs(diff.crl());

        xlim.start_ = mCast(float, mMIN(startbid.inl(),stopbid.inl()));
        xlim.stop_ = mCast(float, mMAX(startbid.inl(),stopbid.inl()));
        ylim.start_ = mCast(float, mMIN(startbid.crl(),stopbid.crl()));
        ylim.stop_ = mCast(float, mMAX(startbid.crl(),stopbid.crl()));
    }
    else
	return;

    planedragger_->setDim( isinline ? 1 : 0 );

    const float xwidth =  isinline
		       ? (float) fabs(stoppos.x_-startpos.x_)
		       : SI().inlDistance();
    const float ywidth = isinline
		       ?  SI().crlDistance()
		       : (float) fabs(stoppos.y_-startpos.y_);

    planedragger_->setSize( Coord3(xwidth,ywidth,zrg_.width(true)) );
    planedragger_->setCenter( (c01+c10)/2 );
    planedragger_->setSpaceLimits( xlim, ylim, SI().zRange(true) );
}


TrcKey PreStackDisplay::getTrcKey() const
{
    TrcKey tk;
    if ( section_ )
	tk.setIs3D().setPosition( bid_ );
    else if ( seis2d_ )
	tk.setGeomID( seis2d_->getGeomID() ).setTrcNr( trcnr_ );

    return tk;
}


const BinID& PreStackDisplay::getPosition() const
{
    return bid_;
}


bool PreStackDisplay::isOrientationInline() const
{
    if ( !section_ )
	return false;

    return section_->getOrientation() == OD::SliceType::Inline;
}


const PlaneDataDisplay* PreStackDisplay::getSectionDisplay() const
{
    return section_.ptr();
}


PlaneDataDisplay* PreStackDisplay::getSectionDisplay()
{
    return section_.ptr();
}


void PreStackDisplay::setDisplayTransformation(
					const visBase::Transformation* nt )
{
    flatviewer_->setDisplayTransformation( nt );
    if ( planedragger_ )
	planedragger_->setDisplayTransformation( nt );
}


void PreStackDisplay::setSectionDisplay( PlaneDataDisplay* pdd )
{
    if ( section_ )
	mDetachCB( section_->getMovementNotifier(),
		   PreStackDisplay::sectionMovedCB );

    section_ = pdd;
    if ( !section_ )
	return;

    if ( section_->id().asInt() > id().asInt() )
    {
	pErrMsg("The display restore order is wrong. The section id has to be \
		  lower than PreStack id so that it can be restored earlier \
		  than PreStack display in the sessions." );
	return;
    }


    if ( ioobj_ && !reader_ )
	reader_ = SPSIOPF().get3DReader( *ioobj_ );

    const bool offsetalonginl =
	section_->getOrientation()==OD::SliceType::Crossline;
    basedirection_ = offsetalonginl ? Coord( 0, 1  ) : Coord( 1, 0 );

    if ( section_->getOrientation() == OD::SliceType::Z )
	return;

    mAttachCB(section_->getMovementNotifier(), PreStackDisplay::sectionMovedCB);
}


void PreStackDisplay::sectionMovedCB( CallBacker* )
{
    BinID newpos = bid_;

    if ( !section_ )
	return;
    else
    {
	if ( section_->getOrientation() == OD::SliceType::Inline )
	{
	    newpos.inl() =
		section_->getTrcKeyZSampling( -1 ).hsamp_.start_.inl();
	}
	else if ( section_->getOrientation() == OD::SliceType::Crossline )
	{
	    newpos.crl() =
		section_->getTrcKeyZSampling( -1 ).hsamp_.start_.crl();
	}
	else
	    return;
    }

    if ( !setPosition(TrcKey(newpos)) )
	return;
}


const Seis2DDisplay* PreStackDisplay::getSeis2DDisplay() const
{
    return seis2d_.ptr();
}


ConstRefMan<DataPack> PreStackDisplay::getDataPack( int attrib ) const
{
    ConstRefMan<FlatDataPack> flatdp = getFlatDataPack( attrib );
    return flatdp.ptr();
}


ConstRefMan<FlatDataPack> PreStackDisplay::getFlatDataPack( int ) const
{
    WeakPtr<FlatDataPack> flatdp = flatviewer_->getPack( false );
    return flatdp.get();
}


ConstRefMan<PreStack::Gather> PreStackDisplay::getGather() const
{
    ConstRefMan<FlatDataPack> flatdp = getFlatDataPack();
    return dynamic_cast<const PreStack::Gather*>( flatdp.ptr() );
}


bool PreStackDisplay::is3DSeis() const
{
    return section_;
}


void PreStackDisplay::setTraceNr( int trcnr )
{
    if ( seis2d_ )
    {
	const TrcKey tk( seis2d_->getGeomID(), trcnr );
	RefMan<PreStack::Gather> gather = new PreStack::Gather;
	if ( !ioobj_ || !reader_ || !gather->readFrom(*ioobj_,*reader_,tk) )
	{
	    mDefineStaticLocalObject( bool, show2d, = false );
	    mDefineStaticLocalObject( bool, resettrace, = true );
	    if ( !show2d )
	    {
//		resettrace = uiMSG().askContinue(
//		"There is no data at the selected location."
//		"\n\nDo you want to find a nearby location to continue?" );
		show2d = true;
	    }

	    if ( resettrace )
	    {
		trcnr_ = getNearTraceNr( trcnr );
		if ( trcnr_==-1 )
		{
//		    uiMSG().warning("Can not read or no data at the section.");
		    trcnr_ = trcnr; //If no data, we still display the panel.
		}
	    }
	}
	else
	{
	    gather_ = gather.ptr();
	    trcnr_ = trcnr;
	}
    }
    else
	trcnr_ = trcnr;

    draggermoving.trigger();
    seis2DMovedCB( nullptr );
    updateData();
    turnOn( true );
}


bool PreStackDisplay::setSeis2DDisplay( Seis2DDisplay* s2d, int trcnr )
{
    if ( !s2d )
	return false;

    if ( seis2d_ )
	mDetachCB( seis2d_->getMovementNotifier(),
		   PreStackDisplay::seis2DMovedCB );

    seis2d_ = s2d;

    if ( seis2d_->id().asInt() > id().asInt() )
    {
	pErrMsg("The display restore order is wrong. The Seis2d display id \
		has to be lower than PreStack id so that it can be restored \
		earlier than PreStack display in the sessions." );
	return false;
    }

     if ( ioobj_ && !reader_ )
	 reader_ = SPSIOPF().get2DReader( *ioobj_, seis2d_->getGeomID() );

    setTraceNr( trcnr );
    if ( trcnr_ < 0 )
	return false;

    const Coord orig = SI().binID2Coord().transformBackNoSnap( Coord(0,0) );
    basedirection_ = SI().binID2Coord().transformBackNoSnap(
			seis2d_->getNormal(trcnr_) ) - orig;
    seis2dpos_ = SI().binID2Coord().transformBackNoSnap(
			seis2d_->getCoord(trcnr_) );

    mAttachCB( seis2d_->getMovementNotifier(), PreStackDisplay::seis2DMovedCB );
    planedragger_->showDraggerBorder( false );

    return updateData();
}


void PreStackDisplay::seis2DMovedCB( CallBacker* )
{
    if ( !seis2d_ || trcnr_<0 )
	return;

    const Coord orig = SI().binID2Coord().transformBackNoSnap( Coord(0,0) );
    basedirection_ = SI().binID2Coord().transformBackNoSnap(
	    seis2d_->getNormal(trcnr_) ) - orig;
    seis2dpos_ = SI().binID2Coord().transformBackNoSnap(
	    seis2d_->getCoord(trcnr_) );
    dataChangedCB( nullptr );
}


const Coord PreStackDisplay::getBaseDirection() const
{
    return basedirection_;
}


BufferString PreStackDisplay::lineName() const
{
    if ( !seis2d_ )
	return BufferString::empty();

    return seis2d_->name();
}


void PreStackDisplay::otherObjectsMoved( const ObjectSet<const SurveyObject>&,
					 const VisID& whichobj )
{
    if ( !section_ && ! seis2d_ )
	return;

    if ( !whichobj.isValid() )
    {
	if ( section_ )
	    turnOn( section_->isShown() );

	if ( seis2d_ )
	    turnOn( seis2d_->isShown() );
	return;
    }

    if ( (section_ && section_->id() != whichobj) ||
	 (seis2d_ && seis2d_->id() != whichobj) )
	return;

    if ( section_ )
	turnOn( section_->isShown() );

    if ( seis2d_ )
	turnOn( seis2d_->isShown() );
}


void PreStackDisplay::draggerMotion( CallBacker* )
{
    Coord draggerbidf = planedragger_->center();

    bool showplane = false;
    if ( section_ )
    {
	const OD::SliceType orientation = section_->getOrientation();
        const int newinl = SI().inlRange( true ).snap( draggerbidf.x_ );
        const int newcrl = SI().crlRange( true ).snap( draggerbidf.y_ );
	if ( orientation==OD::SliceType::Inline && newcrl!=bid_.crl() )
	    showplane = true;
	else if ( orientation==OD::SliceType::Crossline && newinl!=bid_.inl() )
	    showplane = true;

	draggerpos_ = BinID( newinl, newcrl );
    }
    else if ( seis2d_ )
    {
	const int dimtoadjust = planedragger_->getDim() ? 0 : 1;
	draggerbidf[dimtoadjust] = seis2dpos_[dimtoadjust];
	const Coord draggercrd = SI().binID2Coord().transform( draggerbidf );
	const int nearesttrcnr =
		seis2d_->getNearestTraceNr( Coord3(draggercrd,0.) );
	if ( nearesttrcnr != trcnr_ )
	    showplane = true;

	const Coord trcpos = seis2d_->getCoord( nearesttrcnr );
	const Coord newdraggerbidf =
			SI().binID2Coord().transformBackNoSnap( trcpos );

	const Coord direction = posside_ ? basedirection_ : -basedirection_;
	const float offsetscale = mCast(float,
			    Coord(basedirection_.x_*SI().inlDistance(),
				  basedirection_.y_*SI().crlDistance()).abs());

	seis2dpos_ = newdraggerbidf;
	seis2dstoppos_ = autowidth_
	    ? seis2dpos_ + direction*offsetrange_.width()*factor_ / offsetscale
	    : seis2dpos_ + direction*width_ / offsetscale;

        const Coord3 c01( seis2dpos_, zrg_.stop_ );
        const Coord3 c10( seis2dstoppos_, zrg_.start_ );

	planedragger_->setCenter( (c01+c10)/2 );
	trcnr_ = nearesttrcnr;
    }

    planedragger_->showPlane( showplane );
    planedragger_->showDraggerBorder( !showplane && section_ );

    draggermoving.trigger();
}


void PreStackDisplay::finishedCB( CallBacker* )
{
    Coord draggerbidf = planedragger_->center();
    if ( section_ )
    {
        int newinl = SI().inlRange( true ).snap( draggerbidf.x_ );
        int newcrl = SI().crlRange( true ).snap( draggerbidf.y_ );
	if ( section_->getOrientation() == OD::SliceType::Inline )
	    newinl = section_->getTrcKeyZSampling( -1 ).hsamp_.start_.inl();
	else if ( section_->getOrientation() == OD::SliceType::Crossline )
	    newcrl = section_->getTrcKeyZSampling( -1 ).hsamp_.start_.crl();

	setPosition( TrcKey(BinID(newinl,newcrl)) );
    }
    else if ( seis2d_ )
    {
	const int dimtoadjust = planedragger_->getDim() ? 0 : 1;
	draggerbidf[dimtoadjust] = seis2dpos_[dimtoadjust];
	const Coord draggercrd = SI().binID2Coord().transform( draggerbidf );
	const int nearesttrcnr =
		seis2d_->getNearestTraceNr( Coord3(draggercrd,0.) );
	setTraceNr( nearesttrcnr );
    }
    else
	return;

    planedragger_->showPlane( false );
    planedragger_->showDraggerBorder( section_ );
}


void PreStackDisplay::getMousePosInfo( const visBase::EventInfo& ei,
				       Coord3& pos, BufferString& val,
				       uiString& info ) const
{
    val.setEmpty();
    info.setEmpty();
    if ( !flatviewer_  )
	return;

    ConstRefMan<PreStack::Gather> gather = flatviewer_->getPack( false ).get();
    if ( !gather || !gather->isOK() )
	return;

    const int nrtrcs = gather->size( true );
    const FlatPosData& posdata = gather->posData();

    int trcidx = -1;
    Coord disppos;
    if ( seis2d_ )
    {
	const double displaywidth = seis2dstoppos_.distTo(seis2dpos_);
	const double curdist =
	    SI().binID2Coord().transformBackNoSnap( pos ).distTo( seis2dpos_ );
	trcidx = mNINT32( (nrtrcs-1)*curdist/displaywidth );
	disppos = SI().binID2Coord().transform( seis2dpos_ );
    }
    else if ( section_ )
    {
	if ( mIsZero(width_,mDefEpsF) )
	    return;

	disppos = SI().transform( bid_ );
	const double distance = pos.coord().distTo( disppos );
	trcidx = mNINT32( (nrtrcs-1)*distance/width_ );
    }

    if ( trcidx<0 )
	trcidx = 0;
    else if ( trcidx>=nrtrcs )
	trcidx = nrtrcs-1;

    const TrcKey tk = getTrcKey();
    const float offset = gather->getOffset( trcidx );
    const float azimuth = gather->getAzimuth( trcidx );

    if ( tk.is3D() )
    {
	if ( isOrientationInline() )
	    info.set( uiStrings::sInline() ).addMoreInfo( tk.inl() );
	else
	    info.set( uiStrings::sCrossline() ).addMoreInfo( tk.crl() );
    }
    else
    {
	info.set( uiStrings::sLine() ).addMoreInfo( lineName() );
	uiString trcmsg = toUiString( "TrcNr" );
	trcmsg.addMoreInfo( trcnr_ );
	info.appendPhrase( trcmsg, uiString::Comma, uiString::OnSameLine );
	if ( tk.is2D() )
	{
	    const Pos::GeomID gid = tk.geomID();
	    ConstRefMan<Survey::Geometry> geom = Survey::GM().getGeometry(gid);
	    if ( geom && geom->is2D() )
	    {
		Coord crd2d; float spnr = mUdf(float);
		if ( geom->as2D()->getPosByTrcNr(tk.trcNr(),crd2d,spnr) &&
		     !mIsUdf(spnr) && !mIsEqual(spnr,-1.f,1e-5f) )
		{
		    uiString spmsg = toUiString( "SP" );
		    spmsg.addMoreInfo( toUiString(spnr,0,'f',2) );
		    info.appendPhrase( spmsg, uiString::Comma,
				       uiString::OnSameLine );
		}
	    }
	}
    }

    if ( !mIsUdf(offset) )
    {
	const UnitOfMeasure* offsuom =
			UnitOfMeasure::offsetUnit( gather->offsetType() );
	uiString offmsg = gather->isOffsetAngle() ? uiStrings::sAngle()
						  : uiStrings::sOffset();
	offmsg.constructWordWith( offsuom->getUiLabel(), true )
	      .addMoreInfo( toUiStringDec(offset,0) );
	info.appendPhrase( offmsg, uiString::Comma, uiString::OnSameLine );
    }

    if ( !mIsUdf(azimuth) )
    {
	const float azideg = azimuth * mRad2DegF;
	uiString azimsg = uiStrings::sAzimuth();
	azimsg.constructWordWith( UnitOfMeasure::degreesUnit()->getUiLabel() )
	      .addMoreInfo( toUiStringDec(azideg,0) );
	info.appendPhrase( azimsg, uiString::Comma, uiString::OnSameLine );
    }

    pos = Coord3( disppos, pos.z_ );
    const int zsample = posdata.range(false).nearestIndex( pos.z_ );
    val.set( gather->data().get( trcidx, zsample ) );
}


void PreStackDisplay::fillPar( IOPar& par ) const
{
    if ( !section_ && !seis2d_ )
	return;

    VisualObjectImpl::fillPar( par );
    SurveyObject::fillPar( par );

    if ( section_ )
    {
	par.set( sKeyParent(), section_->id() );
	par.set( sKey::Position(), bid_ );
    }

    if  ( seis2d_ )
    {
	par.set( sKeyParent(), seis2d_->id() );
	par.set( sKeyTraceNr(), trcnr_ );
    }

    par.set( sKeyMultiID(), mid_ );
    par.setYN( sKeyAutoWidth(), autowidth_ );
    par.setYN( sKeySide(), posside_ );

    if ( flatviewer_ )
	flatviewer_->appearance().ddpars_.fillPar( par );

    if ( autowidth_ )
	par.set( sKeyFactor(), factor_ );
    else
	par.set( sKeyWidth(), width_ );
}


bool PreStackDisplay::usePar( const IOPar& par )
{
    if ( !VisualObjectImpl::usePar(par) || !SurveyObject::usePar(par) )
	 return false;

    int parentid = -1;
    if ( !par.get(sKeyParent(),parentid) )
    {
	if ( !par.get("Seis2D ID",parentid) )
	    if ( !par.get("Section ID",parentid) )
		return false;
    }

    visBase::DataObject* parent = visBase::DM().getObject( VisID(parentid) );
    if ( !parent )
	return false;

    MultiID mid;
    if ( !par.get(sKeyMultiID(),mid) )
    {
	if ( !par.get("PreStack MultiID",mid) )
	{
	    return false;
	}
    }

    setMultiID( mid );

    mDynamicCastGet( PlaneDataDisplay*, pdd, parent );
    mDynamicCastGet( Seis2DDisplay*, s2d, parent );
    if ( !pdd && !s2d )
	return false;

    if ( pdd )
    {
	setSectionDisplay( pdd );
	BinID bid;
	if ( !par.get(sKey::Position(),bid) )
	{
	    if ( !par.get("PreStack BinID",bid) )
		return false;
	}

	if ( !setPosition(TrcKey(bid)) )
	    return false;
    }

    if ( s2d )
    {
	int tnr;
	if ( !par.get(sKeyTraceNr(),tnr) )
	{
	    if ( !par.get("Seis2D TraceNumber",tnr) )
		return false;
	}

	setSeis2DDisplay( s2d, tnr );
    }

    float factor, width;
    if ( par.get(sKeyFactor(),factor) )
	setFactor( factor );

    if ( par.get(sKeyWidth(),width) )
	setWidth( width );

    bool autowidth, side;
    if ( par.getYN(sKeyAutoWidth(),autowidth) )
	 displaysAutoWidth( autowidth );

    if ( par.getYN(sKeySide(),side) )
	displaysOnPositiveSide( side );

    if ( flatviewer_ )
    {
	flatviewer_->appearance().ddpars_.usePar( par );
	flatviewer_->handleChange( FlatView::Viewer::DisplayPars );
    }

    return true;
}


bool PreStackDisplay::setPosition( const BinID& nb )
{
    const TrcKey tk( nb );
    return setPosition( tk );
}


DataPackID PreStackDisplay::preProcess()
{
    ConstRefMan<PreStack::Gather> gather = getProcessedGather();
    return gather ? gather->id() : DataPackID::udf();
}

} // namespace visSurvey
