#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "attribsel.h"
#include "datapointset.h"
#include "emmarchingcubessurface.h"
#include "emposid.h"
#include "visgeomindexedshape.h"
#include "vismarchingcubessurface.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

namespace Geometry { class ExplicitIndexedShape; }
namespace EM { class ImplicitBody; }

namespace visSurvey
{

mExpClass(visSurvey) MarchingCubesDisplay : public visBase::VisualObjectImpl
					  , public SurveyObject
{ mODTextTranslationClass(MarchingCubesDisplay);
public:
			    MarchingCubesDisplay();

			    mDefaultFactoryInstantiation(
				SurveyObject, MarchingCubesDisplay,
				"MarchingCubesDisplay",
				::toUiString(sFactoryKeyword()) )

    MultiID			getMultiID() const override;
    bool			isInlCrl() const override	{ return true; }

    bool			hasColor() const override	{ return true; }
    bool			usesColor() const override;
    OD::Color			getColor() const override;
    void			setColor(OD::Color) override;
    bool			allowMaterialEdit() const  override
				{ return true; }

    void			useTexture(bool yn,bool trigger) override;
    bool			canShowTexture() const override;

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    bool			setVisSurface(visBase::MarchingCubesSurface*);
				//!<Creates an EMObject for it.
    bool			setEMID(const EM::ObjectID&,TaskRunner*);
    EM::ObjectID		getEMID() const;

    const char*			errMsg() const override { return errmsg_.str();}

    int				nrAttribs() const override;
    bool			canAddAttrib(int) const override;
    bool			canRemoveAttrib() const override;
    bool			canHandleColTabSeqTrans(int) const override;
    const ColTab::MapperSetup*	getColTabMapperSetup(int,int) const override;

    void			setColTabMapperSetup(int,
					const ColTab::MapperSetup&,
					TaskRunner*) override;
    const ColTab::Sequence*	getColTabSequence(int) const override;
    bool			canSetColTabSequence() const override;
    void			setColTabSequence(int,const ColTab::Sequence&,
					TaskRunner*) override;
    void			setSelSpec(int,const Attrib::SelSpec&) override;
    void			setSelSpecs(int attrib,
				    const TypeSet<Attrib::SelSpec>&) override;
    const Attrib::SelSpec*	getSelSpec(int attrib,
					   int version=-1) const override;
    const TypeSet<Attrib::SelSpec>* getSelSpecs(int attrib) const override;
    void			setDepthAsAttrib(int);
    void			setIsopach(int);
    void			enableAttrib(int attrib,bool yn) override;
    bool			isAttribEnabled(int attrib) const override;
    bool			hasSingleColorFallback() const override
				{ return true; }

    bool			usesDataPacks() const override	{ return true; }
    SurveyObject::AttribFormat	getAttributeFormat(int) const override
				{ return SurveyObject::RandomPos; }
    bool			getRandomPos(DataPointSet&,
					     TaskRunner*) const override;
    bool			setRandomPosData(int attrib,const DataPointSet*,
						 TaskRunner*) override;
    bool			setPointDataPack(int attrib,PointDataPack*,
						 TaskRunner*) override;
    ConstRefMan<DataPack>	getDataPack(int attrib) const override;
    ConstRefMan<PointDataPack>	getPointDataPack(int attrib) const override;

    bool			isVerticalPlane() const override
				{ return false; }

    void			setOnlyAtSectionsDisplay(bool yn) override;
    bool			displayedOnlyAtSections() const override;

    bool			canRemoveSelection() const override
				{ return true; }
    void			removeSelection(const Selector<Coord3>&,
						TaskRunner*);
    const EM::MarchingCubesSurface* getMCSurface() const
				{ return emsurface_.ptr(); }
    const visBase::MarchingCubesSurface* getDisplaySurface() const
				{ return displaysurface_.ptr(); }

protected:
				~MarchingCubesDisplay();

    bool			updateVisFromEM(bool onlyshape,TaskRunner*);
    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;
    void			materialChangeCB(CallBacker*) override;

    void			getMousePosInfo(const visBase::EventInfo&,
						IOPar&) const override;
    void			getMousePosInfo(const visBase::EventInfo&,
					Coord3& xyzpos,BufferString& val,
					uiString& info) const override;
    void			otherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					const VisID& whichobj) override;
    void			updateIntersectionDisplay();
    void			updateSingleColor();

    static const char*	sKeyEarthModelID()	{ return "EM ID"; }
    static const char*	sKeyAttribSelSpec()	{ return "Attrib SelSpec"; }
    static const char*	sKeyColTabMapper()	{ return "Coltab mapper"; }
    static const char*	sKeyColTabSequence()	{ return "Coltab sequence"; }
    static const char*	sKeyUseTexture()	{ return "Use texture"; }

    RefMan<visBase::MarchingCubesSurface>	displaysurface_;
    RefMan<EM::MarchingCubesSurface>		emsurface_;
    RefObjectSet<DataPointSet>			datapacks_;
    TypeSet<Attrib::SelSpec>			selspecs_;

    bool					isattribenabled_ = true;

    EM::ImplicitBody*				impbody_	= nullptr;
    bool					displayintersections_ = false;

    struct PlaneIntersectInfo
    {
						PlaneIntersectInfo();
						~PlaneIntersectInfo();

	RefMan<visBase::GeomIndexedShape>	visshape_;
	Geometry::ExplicitIndexedShape*		shape_;

	VisID					planeid_;
	char					planeorientation_ = -1;
	float					planepos_	= mUdf(float);

	bool					computed_	= false;
    };

    ObjectSet<PlaneIntersectInfo>		intsinfo_;
    RefMan<visBase::Transformation>		model2displayspacetransform_;
    ConstRefMan<mVisTrans>			intersectiontransform_;

public:
    void			displayIntersections(bool yn)
				{ setOnlyAtSectionsDisplay(yn); }
    bool			areIntersectionsDisplayed() const
				{ return displayedOnlyAtSections(); }
};

} // namespace visSurvey
