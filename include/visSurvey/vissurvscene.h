#ifndef vissurvscene_h
#define vissurvscene_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "visscene.h"
#include "bufstring.h"
#include "cubesampling.h"
#include "position.h"

class BaseMap;
class BaseMapMarkers;
class MouseCursor;
class TaskRunner;
class FontData;
class ZAxisTransform;
template <class T> class Selector;

namespace ZDomain { class Info; }

namespace visBase
{
    class Annotation;
    class MarkerSet;
    class PolygonSelection;
    class SceneColTab;
    class TopBotImage;
    class Transformation;
    class VisualObject;
};

namespace visSurvey
{

/*!\brief Database for 3D objects

<code>VisSurvey::Scene</code> is the database for all 'xxxxDisplay' objects.
Use <code>addObject(visBase::SceneObject*)</code> to add an object to the Scene.

It also manages the size of the survey cube. The ranges in each direction are
obtained from <code>SurveyInfo</code> class.<br>
The display coordinate system is given in [m/m/ms] if the survey's depth is
given in time. If the survey's depth is given in meters, the display coordinate
system is given as [m/m/m]. The display coordinate system is _righthand_
oriented!<br>

OpenInventor(OI) has difficulties handling real-world coordinates (like xy UTM).
Therefore the coordinates given to OI must be transformed from the UTM system
to the display coordinate system. This is done by the display transform, which
is given to all objects in the UTM system. These object are responsible to
transform their coords themselves before giving them to OI.<br>

The visSurvey::Scene has two domains:<br>
1) the UTM coordinate system. It is advised that most objects are here.
The objects added to this domain will have their transforms set to the
displaytransform which transforms their coords from UTM lefthand
(x, y, time[s] or depth[m] ) to display coords (righthand).<br>

2) the InlCrl domain. Here, OI takes care of the transformation between
inl/crl/t to display coords, so the objects does not need any own transform.

*/

mExpClass(visSurvey) Scene : public visBase::Scene
{
public:
    static Scene*		create()
				mCreateDataObj(Scene);

    virtual void		removeAll();
    virtual void		addObject(visBase::DataObject*);
				/*!< If the object is a visSurvey::SurveyObject
				     it will ask if it's an inlcrl-object or
				     not. If it's not an
				     visSurvey::SurveyObject, it will be put in
				     displaydomain
				*/

    virtual int			size() const;
    virtual int                 getFirstIdx(const DataObject*) const;
    virtual int                 getFirstIdx(int did) const
				{ return visBase::Scene::getFirstIdx(did); }
    visBase::DataObject*	getObject(int);
    const visBase::DataObject*	getObject(int) const;

    void			addUTMObject(visBase::VisualObject*);
    void			addInlCrlZObject(visBase::DataObject*);
    virtual void		removeObject(int idx);

    const CubeSampling&		getCubeSampling() const		{ return cs_; }
    void			setCubeSampling(const CubeSampling&);
    void			showAnnotText(bool);
    bool			isAnnotTextShown() const;
    void			showAnnotScale(bool);
    bool			isAnnotScaleShown() const;
    void			showAnnotGrid(bool);
    bool			isAnnotGridShown() const;
    void			showAnnot(bool);
    bool			isAnnotShown() const;
    void			setAnnotText(int dim,const char*);
    const FontData&		getAnnotFont() const;
    void			setAnnotFont(const FontData&);
    void			savePropertySettings();

    visBase::PolygonSelection*	getPolySelection() { return polyselector_; }
    void			setPolygonSelector(visBase::PolygonSelection*);
    const Selector<Coord3>*	getSelector() const;	/*! May be NULL */
    visBase::SceneColTab*	getSceneColTab()     { return scenecoltab_; }
    void			setSceneColTab(visBase::SceneColTab*);

    Notifier<Scene>		mouseposchange;
    Coord3			getMousePos(bool xyt) const;
				/*! If not xyt it is inlcrlt */
    BufferString		getMousePosValue() const;
    BufferString		getMousePosString() const;
    const MouseCursor*		getMouseCursor() const;

    void			objectMoved(CallBacker*);

    void			setFixedZStretch(float);
				/*!<Used to set the z-strecthing according
				    to the user's preference. Is unitless -
				    all entities (i.e. distance vs time)
				    should be handled by zscale.
				*/
    float			getFixedZStretch() const;

    void			setTempZStretch(float);
    float			getTempZStretch() const;

    void			setZScale(float);
				/*!<The zscale should compensate for different
				   entities in xy and z respectively and
				   remain constant through the life of the
				   scene.  */
    float			getZScale() const;
				/*!<Returns an anproximate figure how to scale Z
				    relates to XY coordinates in this scene. */

    float			getApparentVelocity(float zstretch) const;
				/*<!Velocity Unit depends on display depth in
                                    feet setting. */

    const mVisTrans*		getTempZStretchTransform() const;
    const mVisTrans*		getInlCrl2DisplayTransform() const;
    const mVisTrans*		getUTM2DisplayTransform() const;
    void			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    ZAxisTransform*		getZAxisTransform();
    const ZAxisTransform*	getZAxisTransform() const;
    void			setBaseMap(BaseMap*);
    BaseMap*			getBaseMap();

    bool			isRightHandSystem() const;

    void			setZDomainInfo(const ZDomain::Info&);
    const ZDomain::Info&	zDomainInfo() const;
    void			getAllowedZDomains(BufferString&) const;

    // Convenience
    const char*			zDomainKey() const;
    const char*			zDomainUserName() const;
    const char*			zDomainUnitStr(bool withparens=false) const;
    int				zDomainUserFactor() const;
    const char*			zDomainID() const;

    void			setAnnotColor(const Color&);
    const Color			getAnnotColor() const;
    void			setMarkerPos(const Coord3&,int sceneid);
    void			setMarkerSize(float );
    float			getMarkerSize() const;
    const Color&		getMarkerColor() const;
    void			setMarkerColor(const Color&);
    visBase::TopBotImage*	getTopBotImage(bool istop);

    void			fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    static const char*		sKeyZStretch();

protected:
				~Scene();

    void			setup();
    void			updateAnnotationText();
    void			updateTransforms(const CubeSampling&);
    void			mouseMoveCB(CallBacker*);
    visBase::MarkerSet*		createMarkerSet() const;
    void			updateBaseMapCursor(const Coord&);
    static const Color&		cDefaultMarkerColor();

    RefMan<visBase::Transformation>	tempzstretchtrans_;

    RefMan<visBase::Transformation>	inlcrlrotation_;
    RefMan<visBase::Transformation>	inlcrlscale_;
    RefMan<visBase::Transformation>	utm2disptransform_;

    ZAxisTransform*			datatransform_;


    BaseMap*			basemap_;
    BaseMapMarkers*		basemapcursor_;

    visBase::Annotation*	annot_;
    visBase::MarkerSet*		markerset_;
    visBase::PolygonSelection*	polyselector_;
    Selector<Coord3>*		coordselector_;
    visBase::SceneColTab*	scenecoltab_;

    visBase::TopBotImage*	topimg_;
    visBase::TopBotImage*	botimg_;
    int				getImageFromPar(const IOPar&,const char*,
					        visBase::TopBotImage*&);

    Coord3			xytmousepos_;
    BufferString		mouseposval_;
    BufferString		mouseposstr_;
    const MouseCursor*		mousecursor_;
    IOPar&			infopar_;
    float			curzstretch_;

    ZDomain::Info*		zdomaininfo_;
    float			zscale_;

    bool			userwantsshading_; //from settings
    CubeSampling		cs_;

    bool			ctshownusepar_;
    bool			usepar_;

    static const char*		sKeyShowAnnot();
    static const char*		sKeyShowScale();
    static const char*		sKeyShowGrid();
    static const char*		sKeyAnnotFont();
    static const char*		sKeyAnnotColor();
    static const char*		sKeyShowCube();
    static const char*		sKeyZAxisTransform();
    static const char*		sKeyAppAllowShading();
    static const char*		sKeyTopImageID();
    static const char*		sKeyBotImageID();
};

}; // namespace visSurvey


#endif

