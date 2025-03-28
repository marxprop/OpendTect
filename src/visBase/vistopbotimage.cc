/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vistopbotimage.h"

#include "filespec.h"
#include "imagedeftr.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "odimage.h"
#include "settingsaccess.h"
#include "vismaterial.h"

#include <osgGeo/TexturePlane>
#include <osgGeo/LayeredTexture>
#include <osg/Image>
#include <osgDB/ReadFile>

mCreateFactoryEntry( visBase::TopBotImage )

namespace visBase
{

const char* TopBotImage::sKeyTopLeftCoord()		{ return "TopLeft"; }
const char*	TopBotImage::sKeyBottomRightCoord()	{ return "BotRight"; }


TopBotImage::TopBotImage()
    : VisualObjectImpl(true)
    , laytex_(new osgGeo::LayeredTexture)
    , texplane_(new osgGeo::TexturePlaneNode)
{
    ref();
    refOsgPtr( laytex_ );
    refOsgPtr( texplane_ );
    layerid_ = laytex_->addDataLayer();
    laytex_->addProcess( new osgGeo::IdentityLayerProcess(*laytex_, layerid_) );
    laytex_->allowShaders( SettingsAccess().doesUserWantShading(false) );
    texplane_->setLayeredTexture( laytex_ );
    addChild( texplane_ );

    setTransparency( 0.0 );
    unRefNoDelete();
}


TopBotImage::~TopBotImage()
{
    unRefOsgPtr( laytex_ );
    unRefOsgPtr( texplane_ );
}


void TopBotImage::setImageID( const MultiID& id )
{
    odimageid_ = id;
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
	return;

    imagedef_.setBaseDir( IOM().rootDir().fullPath() );
    ODImageDefTranslator::readDef( imagedef_, *ioobj );
    setRGBImageFromFile( getImageFilename() );
    const Coord3 tlcrd( imagedef_.tlcoord_.coord(), topLeft().z_ );
    const Coord3 brcrd( imagedef_.brcoord_.coord(), bottomRight().z_ );
    setPos( tlcrd, brcrd );
}


void TopBotImage::setPos( const Coord3& c1, const Coord3& c2 )
{
    pos0_ = c1;
    pos1_ = c2;
    updateCoords();
}


void TopBotImage::updateCoords()
{
    Coord3 pos0, pos1;
    Transformation::transform( trans_.ptr(), pos0_, pos0 );
    Transformation::transform( trans_.ptr(), pos1_, pos1 );

    const Coord3 dif = pos1 - pos0;
    texplane_->setWidth( osg::Vec3(dif.x_, -dif.y_, 0.0) );

    const Coord3 avg = 0.5 * (pos0+pos1);
    texplane_->setCenter( osg::Vec3(avg.x_, avg.y_, avg.z_) );
}


void TopBotImage::setDisplayTransformation( const mVisTrans* trans )
{
    trans_ = trans;
    updateCoords();
}


const mVisTrans* TopBotImage::getDisplayTransformation() const
{
    return trans_.ptr();
}


void TopBotImage::setTransparency( float val )
{ getMaterial()->setTransparency( val ); }


float TopBotImage::getTransparency() const
{ return getMaterial()->getTransparency(); }


void TopBotImage::setImageFilename( const char* fnm )
{
    imagedef_.setFileName( fnm );
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile( fnm );
    laytex_->setDataLayerImage( layerid_, image );
    texplane_->setTextureBrickSize( laytex_->maxTextureSize() );
}


const char* TopBotImage::getImageFilename() const
{
    return imagedef_.getFileName();
}


void TopBotImage::setRGBImageFromFile( const char* fnm )
{
    uiString errmsg;
    PtrMan<OD::RGBImage> rgbimg =
			 OD::RGBImageLoader::loadRGBImage( fnm, errmsg );
    if ( !rgbimg )
    {
	pErrMsg( errmsg.getFullString() );
	return;
    }

    setRGBImage( *rgbimg );
}


void TopBotImage::setRGBImage( const OD::RGBImage& rgbimg )
{
    const int totsz = rgbimg.getSize(true) * rgbimg.getSize(false) * 4;
    unsigned char* imgdata = new unsigned char[totsz];
    OD::memCopy( imgdata, rgbimg.getData(), totsz );
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->setImage( rgbimg.getSize(true), rgbimg.getSize(false), 1,
		     GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE, imgdata,
		     osg::Image::NO_DELETE );
    image->flipVertical();
    laytex_->setDataLayerImage( layerid_, image );
    texplane_->setTextureBrickSize( laytex_->maxTextureSize() );
}


bool TopBotImage::getImageInfo( int& width, int& height, int& pixsz ) const
{
    const osg::Image* image = laytex_ ? laytex_->getCompositeTextureImage()
				      : nullptr;
    if ( !image )
	return false;

    width = image->s();
    height = image->t();
    pixsz = image->getPixelSizeInBits() / 8;
    return true;
}


unsigned const char* TopBotImage::getTextureData() const
{
    const osg::Image* image = laytex_ ? laytex_->getCompositeTextureImage()
				      : nullptr;
    return image ? image->data() : nullptr;
}


bool TopBotImage::getTextureDataInfo( TypeSet<Coord3>& coords,
				      TypeSet<Coord>& texcoords,
				      TypeSet<int>& ps ) const
{
    const std::vector<osg::Geometry*>& geometries = texplane_->getGeometries();
    Interval<float> xrg( mUdf(float), -mUdf(float) );
    Interval<float> yrg( mUdf(float), -mUdf(float) );
    Interval<float> zrg( mUdf(float), -mUdf(float) );
    Interval<float> texxrg( mUdf(float), -mUdf(float) );
    Interval<float> texyrg( mUdf(float), -mUdf(float) );
    for ( unsigned int gidx=0; gidx<geometries.size(); gidx++ )
    {
	const osg::Array* vertarr = geometries[gidx]->getVertexArray();
	const osg::Vec3Array* vertcoords =
			dynamic_cast<const osg::Vec3Array*>( vertarr );
	if ( !vertcoords )
	    continue;

	for ( unsigned int idx=0; idx<vertcoords->size(); idx++ )
	{
	    xrg.include( vertcoords->at(idx)[0], false );
	    yrg.include( vertcoords->at(idx)[1], false );
	    zrg.include( vertcoords->at(idx)[2], false );
	}

	osg::ref_ptr<const osg::Vec2Array> osgcoords =
			    texplane_->getCompositeTextureCoords( gidx );
	if ( !osgcoords.valid() )
	    continue;

	for ( unsigned int idx=0; idx<osgcoords->size(); idx++ )
	{
	    texxrg.include( osgcoords->at(idx)[0], false );
	    texyrg.include( osgcoords->at(idx)[1], false );
	}
    }

    coords.setEmpty();
    Coord3 crd; crd.z_ = sCast(double,zrg.start_);
    crd.setXY( xrg.start_, yrg.start_ ); coords += crd;
    crd.setXY( xrg.stop_, yrg.start_ ); coords += crd;
    crd.setXY( xrg.stop_, yrg.stop_ ); coords += crd;
    crd.setXY( xrg.start_, yrg.stop_ ); coords += crd;

    texcoords.setEmpty();
    Coord texcrd;
    texcrd.setXY( texxrg.start_, texyrg.start_ ); texcoords += texcrd;
    texcrd.setXY( texxrg.stop_, texyrg.start_ ); texcoords += texcrd;
    texcrd.setXY( texxrg.stop_, texyrg.stop_ ); texcoords += texcrd;
    texcrd.setXY( texxrg.start_, texyrg.stop_ ); texcoords += texcrd;

    ps.setEmpty();
    for ( int idx=0; idx<4; idx++ )
	ps += idx;

    return true;
}


void TopBotImage::fillPar( IOPar& iopar ) const
{
    VisualObjectImpl::fillPar( iopar );

    if ( odimageid_.isUdf() ) // Legacy
	imagedef_.fillPar( iopar );
    else
	iopar.set( sKey::ID(), odimageid_ );

    iopar.set( sKeyTopLeftCoord(), pos0_ );
    iopar.set( sKeyBottomRightCoord(), pos1_ );
    iopar.set( sKey::Transparency(), getTransparency() );
}


bool TopBotImage::usePar( const IOPar& iopar )
{
    VisualObjectImpl::usePar( iopar );

    MultiID odimageid;
    if ( iopar.get(sKey::ID(),odimageid) && !odimageid.isUdf() )
	setImageID( odimageid );
    else
    {
	imagedef_.usePar( iopar );
	setRGBImageFromFile( imagedef_.getFileName() );
    }

    Coord3 ltpos;
    Coord3 brpos;
    iopar.get( sKeyTopLeftCoord(), ltpos );
    iopar.get( sKeyBottomRightCoord(), brpos );
    setPos( ltpos, brpos );

    float transparency = 0.f;
    if ( iopar.get(sKey::Transparency(),transparency) )
	setTransparency( transparency );

    return true;
}

} // namespace visBase
