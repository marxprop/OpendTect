#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vismultiattribsurvobj.h"

#include "mousecursor.h"
#include "randomlinegeom.h"
#include "ranges.h"
#include "seisdatapack.h"
#include "visevent.h"
#include "vismarkerset.h"
#include "vispolyline.h"
#include "visrandomtrackdragger.h"
#include "vistexturepanelstrip.h"
#include "zaxistransform.h"

class TrcKeyZSampling;


namespace visSurvey
{

class Scene;

/*!\brief Used for displaying a random or arbitrary line.

    RandomTrackDisplay is the front-end class for displaying arbitrary lines.
    The complete line consists of separate sections connected at
    inline/crossline positions, called knots or nodes. Several functions are
    available for adding or inserting node positions. The depth range of the
    line can be changed by <code>setDepthInterval(const Interval<float>&)</code>
*/

mExpClass(visSurvey) RandomTrackDisplay : public MultiTextureSurveyObject
{ mODTextTranslationClass(RandomTrackDisplay);
public:
				RandomTrackDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject,RandomTrackDisplay,
				    "RandomTrackDisplay",
				    ::toUiString(sFactoryKeyword()) )

    void			setRandomLineID(const RandomLineID&);
    RandomLineID		getRandomLineID() const;
    const Geometry::RandomLine* getRandomLine() const;
    Geometry::RandomLine*	getRandomLine();

    int				nameNr() const { return namenr_; }
				/*!<\returns a number that is unique for
				     this rtd, and is present in its name. */

    bool			isInlCrl() const override { return true; }

    int				nrResolutions() const override	{ return 3; }
    void			setResolution(int,TaskRunner*) override ;

    bool			hasPosModeManipulator() const override
				{ return true; }
    void			showManipulator(bool) override;
    bool			isManipulatorShown() const override;
    bool			isManipulated() const override;
    bool			canResetManipulation() const override
				{ return true; }
    void			resetManipulation() override;
    void			acceptManipulation() override;
    uiString			getManipulationString() const override;

    bool			canDuplicate() const override	{ return true; }
    SurveyObject*		duplicate(TaskRunner*) const override;
    MultiID			getMultiID() const override;

    bool			allowMaterialEdit() const override
				{ return true; }

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const override;

    void			getTraceKeyPath(TrcKeySet&,
						TypeSet<Coord>*) const override;
    Interval<float>		getDataTraceRange() const override;
    TypeSet<Coord>		getTrueCoords() const;

    ConstRefMan<VolumeDataPack> getDisplayedVolumeDataPack(int attrib)
								const override;
    RefMan<VolumeDataPack>	getDisplayedVolumeDataPack(int attrib);

    bool			canAddNode(int nodenr) const;
				/*!< If nodenr<nrNodes the function Checks if
				     a node can be added before the nodenr.
				     If nodenr==nrNodes, it checks if a node
				     can be added. */
    void			addNode(int nodenr);
				/*!< If nodenr<nrNodes, a node is added before
				     the nodenr. If nodenr==nrNodes, a node is
				     added at the end. */

    int				nrNodes() const;
    void			addNode(const BinID&);
    void			insertNode(int,const BinID&);
    void			setNodePos(int,const BinID&);
    BinID			getNodePos(int) const;
    BinID			getManipNodePos(int) const;
    void			getAllNodePos(TrcKeySet&) const;
    TypeSet<BinID>*		getNodes()		{ return &nodes_; }
    void			removeNode(int);
    void			removeAllNodes();
    bool			setNodePositions(const TypeSet<BinID>&);
    void			lockGeometry(bool);
    bool			isGeometryLocked() const;

    TrcKeyZSampling		getTrcKeyZSampling(bool displayspace,
						int attrib) const override;
    void			setDepthInterval(const Interval<float>&);
    Interval<float>		getDepthInterval() const;

    const MouseCursor*		getMouseCursor() const override
				{ return &mousecursor_; }

    void			getMousePosInfo(const visBase::EventInfo&,
						IOPar&) const override;
    void			getMousePosInfo(const visBase::EventInfo&,
						Coord3&, BufferString&,
						uiString&) const override;
    void			getObjectInfo(uiString&) const override;

    int				getSelNodeIdx() const	{ return selnodeidx_; }
				//!<knotidx>=0, panelidx<0

    NotifierAccess*		getMovementNotifier() override
				{ return &moving_; }
    NotifierAccess*		getManipulationNotifier() override
				{ return &nodemoving_; }

    int				getClosestPanelIdx(const Coord&) const;
    Coord3			getNormal(const Coord3&) const override;
    float			calcDist(const Coord3&) const override;
    bool			allowsPicks() const override
				{ return true; }

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    bool			canBDispOn2DViewer() const override
				{ return true; }
    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;
    const visBase::TexturePanelStrip* getTexturePanelStrip() const;
    visBase::TexturePanelStrip* getTexturePanelStrip();
    BufferString		getRandomLineName() const;

    Notifier<RandomTrackDisplay> moving_;
    Notifier<RandomTrackDisplay> nodemoving_;

    const char*			errMsg() const override { return errmsg_.str();}
    void			setPolyLineMode(bool yn);
    bool			createFromPolyLine();
    void			setColor(OD::Color) override;

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;
    const ZAxisTransform*	getZAxisTransform() const override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			annotateNextUpdateStage(bool yn) override;
    void			setPixelDensity(float) override;
    const TrcKeySet*		getTrcKeyPath() const { return &trckeypath_; }

    static const char*		sKeyPanelDepthKey()  { return "PanelDepthKey"; }
    static const char*		sKeyPanelPlaneKey()  { return "PanelPlaneKey"; }
    static const char*		sKeyPanelRotateKey() { return "PanelRotateKey";}

protected:
				~RandomTrackDisplay();

    bool			getCacheValue(int attrib,int version,
					      const Coord3&,
					      float&) const override;

    void			addCache() override;
    void			removeCache(int) override;
    void			swapCache(int,int) override;
    void			emptyCache(int) override;
    bool			hasCache(int) const override;

    BinID			proposeNewPos(int node) const;
    void			updateChannels(int attrib,TaskRunner*);
    void			createTransformedDataPack(int attrib,
							  TaskRunner* =nullptr);

    bool			usesDataPacks() const override	{ return true; }
    bool			setVolumeDataPack(int attrib,VolumeDataPack*,
						TaskRunner*) override;
    ConstRefMan<DataPack>	getDataPack(int attrib) const override;
    ConstRefMan<VolumeDataPack> getVolumeDataPack(int attrib) const override;
    RefMan<VolumeDataPack>	getVolumeDataPack(int attrib);

    void			setNodePos(int,const BinID&,bool check);
    BinID			snapPosition(const BinID&) const;
    bool			checkPosition(const BinID&) const;

    void			geomChangeCB(CallBacker*);
    void			nodeMoved(CallBacker*);
    void			draggerRightClick(CallBacker*);

    void			pickCB(CallBacker*);
    bool			checkValidPick(const visBase::EventInfo&,
					       const Coord3& pos) const;
    void			setPickPos(const Coord3& pos);
    void			removePickPos(const Coord3&);
    void			dataTransformCB(CallBacker*);
    void			updateRanges(bool,bool);

    void			updatePanelStripPath();
    void			setPanelStripZRange(const Interval<float>&);
    float			appliedZRangeStep() const;
    void			draggerMoveFinished(CallBacker*);
    void			updateMouseCursorCB(CallBacker*) override;

    int				nrgeomchangecbs_ = 0;
    TypeSet<VisID>*		premovingselids_ = nullptr;
    bool			geomnodejustmoved_ = false;

    RefMan<Geometry::RandomLine> rl_;
    RefMan<visBase::TexturePanelStrip>	panelstrip_;

    RefMan<visBase::RandomTrackDragger> dragger_;

    RefMan<visBase::PolyLine>	polyline_;
    RefMan<visBase::MarkerSet>	markerset_;

    RefMan<visBase::EventCatcher> eventcatcher_;
    MouseCursor			mousecursor_;

    int				selnodeidx_	= mUdf(int);
    RefObjectSet<RandomSeisDataPack>	datapacks_;
    RefObjectSet<RandomSeisDataPack>	transfdatapacks_;

    RefMan<ZAxisTransform>	datatransform_;
    Interval<float>		depthrg_;
    int				voiidx_ = -1;

    TypeSet<BinID>		nodes_;
    TrcKeySet			trckeypath_;
    TypeSet<int>		segments_;
    int				pickstartnodeidx_ = -1;
    bool			ispicking_ = false;
    int				oldstyledoubleclicked_ = 0;

    struct UpdateStageInfo
    {
	float			oldzrgstart_;
    };
    UpdateStageInfo		updatestageinfo_;

    bool			lockgeometry_ = false;
    bool			ismanip_ = false;
    int				namenr_;
    bool			polylinemode_ = false;
    bool			interactivetexturedisplay_ = false;
    int				originalresolution_ = -1;

    static const char*		sKeyTrack();
    static const char*		sKeyNrKnots();
    static const char*		sKeyKnotPrefix();
    static const char*		sKeyDepthInterval();
    static const char*		sKeyLockGeometry();

    void			updateTexOriginAndScale(
					    int attrib,const TrcKeySet&,
					    const ZSampling&);
public:

    bool			getSelMousePosInfo(const visBase::EventInfo&,
						   Coord3&, BufferString&,
						   uiString&) const;
    mDeprecated("Use TrcKey")
    void			getAllNodePos(TypeSet<BinID>&) const;

protected:
    int				getClosestPanelIdx(const Coord&,
						   double* fracptr) const;
    void			mouseCB(CallBacker*);
    bool			isPicking() const override;
    void			removePickPos(int polyidx);

    void			addNodeInternal(const BinID&);
    void			insertNodeInternal(int,const BinID&);
    void			removeNodeInternal(int);
    void			movingNodeInternal(int selnodeidx);
    void			finishNodeMoveInternal();
    void			geomNodeMoveCB( CallBacker*);
    void			setNodePositions(const TypeSet<BinID>&,
						 bool onlyinternal);

    bool			isMappingTraceOfBid(BinID bid,int trcidx,
						    bool forward=true) const;

    void			snapZRange(Interval<float>&);
    void			updatePath();
};

} // namespace visSurvey
