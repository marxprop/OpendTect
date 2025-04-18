#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "coord.h"
#include "imagedeftr.h"
#include "multiid.h"
#include "visobject.h"
#include "vistransform.h"

namespace OD{ class RGBImage; }
namespace osgGeo { class LayeredTexture; class TexturePlaneNode; }


namespace visBase
{

mExpClass(visBase) TopBotImage : public VisualObjectImpl
{
public:
    static RefMan<TopBotImage>	create();
				mCreateDataObj(TopBotImage);

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setImageID(const MultiID&);
    MultiID			getImageID() const	{ return odimageid_; }

    void			setPos(const Coord3& tl,const Coord3& br);
    const Coord3&		topLeft() const	    { return pos0_; }
    const Coord3&		bottomRight() const { return pos1_; }

    void			setImageFilename(const char*);
    const char*			getImageFilename() const;

    void			setTransparency(float); // 0-1
    float			getTransparency() const; // returns value 0-1

    bool			getImageInfo(int& w,int& h,int& pixsz) const;
    const unsigned char*	getTextureData() const;
    bool			getTextureDataInfo(TypeSet<Coord3>& coords,
						   TypeSet<Coord>& texcoords,
						   TypeSet<int>& ps ) const;

    void			fillPar(IOPar&) const override;
    bool			usePar(const IOPar&) override;

private:
				~TopBotImage();

    void			setRGBImageFromFile(const char*);
    void			updateCoords();
    void			setRGBImage(const OD::RGBImage&);

    ConstRefMan<mVisTrans>	trans_;
    Coord3			pos0_;
    Coord3			pos1_;
    ImageDef			imagedef_;
    MultiID			odimageid_;

    int				layerid_;
    osgGeo::LayeredTexture*	laytex_;
    osgGeo::TexturePlaneNode*	texplane_;

    static const char*		sKeyTopLeftCoord();
    static const char*		sKeyBottomRightCoord();

};

} // namespace visBase
