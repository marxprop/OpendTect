#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapackbase.h"
#include "factory.h"
#include "horizon3dtracker.h"
#include "horizoneditor.h"
#include "uistring.h"
#include "visemobjdisplay.h"
#include "vishorizonsection.h"
#include "vishorizontexturehandler.h"
#include "vismarkerset.h"
#include "vispointset.h"
#include "vistransform.h"

namespace ColTab{ class Sequence; class MapperSetup; }
namespace visBase
{
    class TextureChannel2RGBA;
}

namespace visSurvey
{

mExpClass(visSurvey) HorizonDisplay : public EMObjectDisplay
{ mODTextTranslationClass(HorizonDisplay)

    friend class HorizonPathIntersector;
    struct IntersectionData;
public:
				HorizonDisplay();

				mDefaultFactoryInstantiation(
				    SurveyObject, HorizonDisplay,
				    "HorizonDisplay",
				    ::toUiString(sFactoryKeyword()) )

    void			setDisplayTransformation(
					    const mVisTrans*) override;
    void			setSceneEventCatcher(
					    visBase::EventCatcher*) override;

    void			enableTextureInterpolation(bool) override;
    bool			textureInterpolationEnabled() const override
				{ return enabletextureinterp_; }
    bool			canEnableTextureInterpolation() const override
				{ return true; }

    bool			setZAxisTransform(ZAxisTransform*,
						  TaskRunner*) override;

    void			setIntersectLineMaterial(visBase::Material*);

    bool			setEMObject(const EM::ObjectID&,
					    TaskRunner*) override;
    bool			updateFromEM(TaskRunner*) override;
    void			updateFromMPE() override;
    void			showPosAttrib(int attr,bool yn) override;

    StepInterval<int>		geometryRowRange() const;
    StepInterval<int>		geometryColRange() const;
    visBase::HorizonSection*	getHorizonSection();
    const visBase::HorizonSection* getHorizonSection() const;
    TypeSet<EM::SectionID>	getSectionIDs() const	{ return sids_; }

    void			useTexture(bool yn,bool trigger=false) override;
    bool			canShowTexture() const override;

    void			setOnlyAtSectionsDisplay(bool yn) override;
    bool			displayedOnlyAtSections() const override;

    void			displaySurfaceData(int attrib,int auxdatanr);

    int				nrTextures(int attrib) const override;
    void			selectTexture(int attrib,
					      int textureidx) override;
    int				selectedTexture(int attrib) const override;

    SurveyObject::AttribFormat	getAttributeFormat(int attrib) const override;
    OD::Pol2D3D			getAllowedDataType() const override
				{ return OD::Both2DAnd3D; }

    bool			canHaveMultipleAttribs() const override;
    int				nrAttribs() const override;
    int				maxNrAttribs() const override;
    bool			canAddAttrib(
					int nrattribstoadd=1) const override;
    bool			addAttrib() override;
    bool			canRemoveAttrib() const override;
    bool			removeAttrib(int attrib) override;
    bool			swapAttribs(int attrib0,int attrib1) override;
    void			setAttribTransparency(int,
						      unsigned char) override;
    unsigned char		getAttribTransparency(int) const override;
    void			enableAttrib(int attrib,bool yn) override;
    bool			isAttribEnabled(int attrib) const override;
    bool			hasSingleColorFallback() const override
				{ return true; }
    OD::Color			singleColor() const;

    void			allowShading(bool) override;
    const Attrib::SelSpec*	getSelSpec(int channel,
					   int version=-1) const override;
    const TypeSet<Attrib::SelSpec>* getSelSpecs(int attrib) const override;

    void			setSelSpec(int,const Attrib::SelSpec&) override;
    void			setSelSpecs(int attrib,
				    const TypeSet<Attrib::SelSpec>&) override;
    void			setDepthAsAttrib(int);

    bool			usesDataPacks() const override	{ return true; }
    bool			setFlatDataPack(int attrib,FlatDataPack*,
						TaskRunner*) override;
    ConstRefMan<DataPack>	getDataPack(int attrib) const override;
    ConstRefMan<FlatDataPack>	getFlatDataPack(int attrib) const override;

    bool			allowMaterialEdit() const override
				{ return true; }
    bool			hasColor() const override	{ return true; }
    bool			usesColor() const override;

    EM::SectionID		getSectionID(const VisID&) const override;

    bool			getRandomPos(DataPointSet&,
						TaskRunner*) const override;
    bool			getRandomPosCache(int,
						  DataPointSet&) const override;
    bool			setRandomPosData(int,const DataPointSet*,
						 TaskRunner*) override;

    void			setLineStyle(const OD::LineStyle&) override;
				/*!<If ls is solid, a 3d shape will be used,
				    otherwise 'flat' lines. */
    bool			hasStoredAttrib(int attrib) const;
    bool			hasDepth(int attrib) const;

    int				nrResolutions() const override;
    BufferString		getResolutionName(int) const override;
    int				getResolution() const override;
    bool			displaysSurfaceGrid() const;
    void			displaysSurfaceGrid(bool);
    void			setResolution(int,TaskRunner*) override;
				/*!< 0 is automatic */

    bool			allowsPicks() const override	{ return true; }
    void			getMousePosInfo( const visBase::EventInfo& e,
						 IOPar& i ) const override
				{ return EMObjectDisplay::getMousePosInfo(e,i);}
    void			getMousePosInfo(const visBase::EventInfo& pos,
					    Coord3&,BufferString& val,
					    uiString& info) const override;
    float			calcDist(const Coord3&) const override;
    float			maxDist() const override;

    const ColTab::Sequence*	getColTabSequence(int attr) const override;
    bool			canSetColTabSequence() const override;
    void			setColTabSequence(int attr,
				    const ColTab::Sequence&,
				    TaskRunner*) override;
    const ColTab::MapperSetup*	getColTabMapperSetup(int attr,
						     int v=0) const override;
    void			setColTabMapperSetup(int attr,
						     const ColTab::MapperSetup&,
						     TaskRunner*) override;
    const TypeSet<float>*	getHistogram(int) const override;

    Coord3			getTranslation() const override;
    void			setTranslation(const Coord3&) override;

    bool			setChannels2RGBA(
					visBase::TextureChannel2RGBA*) override;
    visBase::TextureChannel2RGBA* getChannels2RGBA() override;
    const visBase::TextureChannel2RGBA* getChannels2RGBA() const override;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

    bool			canBDispOn2DViewer() const override
				{ return false; }
    bool			isVerticalPlane() const override
				{ return false; }

    void			setAttribShift(int channel,
					       const TypeSet<float>& shifts);
				/*!<Gives the shifts for all versions of an
				    attrib. */

    const ObjectSet<visBase::VertexShape>& getIntersectionLines() const;
					/*!<This function will be deleted*/
    int				getIntersectionDataSize() const
				{ return intersectiondata_.size(); }
    const visBase::VertexShape* getLine(int) const;
    void			displayIntersectionLines(bool);
    bool			displaysIntersectionLines() const;
    const visBase::HorizonSection* getSection(int id) const;

    static HorizonDisplay*	getHorizonDisplay(const MultiID&);

    void			doOtherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					const VisID& whichobj) override;
    void			setPixelDensity(float) override;

    void			setSectionDisplayRestore(bool);
    BufferString		getSectionName(int secidx) const;

    void			selectParent(const TrcKey&);
    void			selectChildren(const TrcKey&);
    void			selectChildren();
    void			showParentLine(bool);
    void			showSelections(bool);
    void			showLocked(bool);
    bool			lockedShown() const;
    void			clearSelections() override;
    void			updateAuxData() override;
    bool			canBeRemoved() const override;

    mDeprecated("Use without SectionID")
    visBase::HorizonSection*	getHorizonSection(const EM::SectionID&)
				{ return getHorizonSection(); }
    mDeprecated("Use without SectionID")
    const visBase::HorizonSection*
				getHorizonSection(const EM::SectionID&) const
				{ return getHorizonSection(); }

private:
				~HorizonDisplay();

    void			removeEMStuff() override;

    EM::PosID			findClosestNode(const Coord3&) const override;
    void			createDisplayDataPacks(
					int attrib,const DataPointSet*);

    void			removeSectionDisplay(
						const EM::SectionID&) override;
    visBase::VisualObject*	createSection(const EM::SectionID&) const;
    bool			activateTracker() override;
    RefMan<MPE::ObjectEditor>	getMPEEditor(bool create) override;
    void			setDisplayDataPacks(int attrib,
					const ObjectSet<MapDataPack>&);
    bool			addSection(const EM::SectionID&,
					   TaskRunner*) override;
    void			emChangeCB(CallBacker*) override;
    int				getChannelIndex(const char* nm) const;
    void			updateIntersectionLines(
				    const ObjectSet<const SurveyObject>&,
				    const VisID& whichobj );
    void			updateSectionSeeds(
				    const ObjectSet<const SurveyObject>&,
				    const VisID& whichobj );
    void			otherObjectsMoved(
				    const ObjectSet<const SurveyObject>&,
				    const VisID& whichobj) override;
    void			updateSingleColor();

    void			calculateLockedPoints();
    void			initSelectionDisplay(bool erase);
    void			updateSelections() override;
    void			handleEmChange(const EM::EMObjectCallbackData&);
    void			updateLockedPointsColor();

    bool			allowshading_ = true;
    RefMan<mVisTrans>		translation_;
    Coord3			translationpos_ = Coord3::udf();

    RefMan<MPE::Horizon3DTracker>	tracker_;
    RefMan<MPE::HorizonEditor>		mpeeditor_;

    RefObjectSet<visBase::HorizonSection>  sections_;
    TypeSet<BufferString>		secnames_;
    TypeSet<EM::SectionID>		sids_;

    struct IntersectionData
    {
				IntersectionData(const OD::LineStyle&);
				~IntersectionData();

	void			addLine(const TypeSet<Coord3>&);
	void			clear();

	void			setPixelDensity(float);
	void			setDisplayTransformation(const mVisTrans*);
	void			updateDataTransform(const TrcKeyZSampling&,
						   ZAxisTransform*);
	void			setSceneEventCatcher(visBase::EventCatcher*);
	void			setMaterial(visBase::Material*);
	RefMan<visBase::VertexShape> setLineStyle(const OD::LineStyle&);
				//Returns old line if replaced

	RefMan<visBase::VertexShape>	line_;
	RefMan<visBase::MarkerSet>	markerset_;
	RefMan<ZAxisTransform>		zaxistransform_;
	int				voiid_ = -2;
	VisID				objid_;
    };

    IntersectionData*		getOrCreateIntersectionData(
				     ObjectSet<IntersectionData>& pool );
				//!<Return data from pool or creates new

    void			traverseLine(const TrcKeySet&,
					     const TypeSet<Coord>&,
					     const Interval<float>& zrg,
					     IntersectionData&) const;
				/*!<List of coordinates may be empty, coords
				    will then be fetched from trckeys. */
    void			drawHorizonOnZSlice(const TrcKeyZSampling&,
					     IntersectionData&) const;

    bool			isValidIntersectionObject(
				   const ObjectSet<const SurveyObject>&,
				   int& objidx,const VisID& objid) const;
				/*!<Check if the active object is one of
				planedata, z-slice, 2dline,..., if it is
				get the the idx in the stored object
				collection.*/
    ManagedObjectSet<IntersectionData>	intersectiondata_;
					//One per object we intersect with

    float				maxintersectionlinethickness_;
    RefMan<visBase::Material>		intersectionlinematerial_;

    RefMan<visBase::PointSet>		selections_;
    RefMan<visBase::PointSet>		lockedpts_;
    RefMan<visBase::PointSet>		sectionlockedpts_;
    RefMan<visBase::VertexShape>	parentline_;

    StepInterval<int>			parrowrg_;
    StepInterval<int>			parcolrg_;

    TypeSet<ColTab::MapperSetup>	coltabmappersetups_;//for each channel
    TypeSet<ColTab::Sequence>		coltabsequences_;  //for each channel
    bool				enabletextureinterp_	= true;

    char				resolution_	= 0;
    int					curtextureidx_	= 0;

    bool				displayintersectionlines_ = true;

    ObjectSet<TypeSet<Attrib::SelSpec> > as_;
    ObjectSet<RefObjectSet<MapDataPack> > datapacksset_;
    BoolTypeSet				enabled_;
    TypeSet<int>			curshiftidx_;
    ObjectSet< TypeSet<float> >		shifts_;
    bool				displaysurfacegrid_	= false;

    TypeSet<EM::SectionID>		oldsectionids_;
    TypeSet<StepInterval<int> >		olddisplayedrowranges_;
    TypeSet<StepInterval<int> >		olddisplayedcolranges_;

    RefObjectSet<visBase::HorizonTextureHandler> oldhortexhandlers_;
    Threads::Mutex*			locker_;
    bool				showlock_	= false;

    static const char*			sKeyTexture();
    static const char*			sKeyShift();
    static const char*			sKeyResolution();
    static const char*			sKeyRowRange();
    static const char*			sKeyColRange();
    static const char*			sKeyIntersectLineMaterialID();
    static const char*			sKeySurfaceGrid();
    static const char*			sKeySectionID();
    static const char*			sKeyZValues();
};

} // namespace visSurvey
