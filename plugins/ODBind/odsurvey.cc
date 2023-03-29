/*+
________________________________________________________________________________

 Copyright:  (C) 2022 dGB Beheer B.V.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
________________________________________________________________________________

-*/

#include "odsurvey.h"

#include "ascstream.h"
#include "coordsystem.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "latlong.h"
#include "moddepmgr.h"
#include "odjson.h"
#include "odruncontext.h"
#include "oscommand.h"
#include "plugins.h"
#include "safefileio.h"
#include "survinfo.h"
#include "transl.h"

#include <cstring>
#include <filesystem>

BufferString odSurvey::curbasedir_;
BufferString odSurvey::cursurvey_;

static const char* moddeps[] =
{
    "EarthModel",
//    "Seis",
//    "Well",
    nullptr
};


odSurvey::odSurvey(const char* basedir, const char* surveynm)
    : basedir_(basedir)
    , survey_(surveynm)
{
    activate();
}

odSurvey::~odSurvey()
{}

BufferString odSurvey::type() const
{
    BufferString res;
    if (has2D()) res.add( "2D" );
    if (has3D()) res.add( "3D" );
    return res;
}

void odSurvey::getInfo( OD::JSON::Object& jsobj) const
{
    const auto& info = si();
    jsobj.set( "name", info.name().buf() );
    jsobj.set( "type", type().buf() );
    jsobj.set( "crs", get_crsCode().buf() );
    jsobj.set( "xyunit", info.getXYUnitString(false) );
    jsobj.set( "zunit", info.getZUnitString(false) );
    jsobj.set( "srd", info.seismicReferenceDatum() );
}


void odSurvey::getFeature(OD::JSON::Object& jsobj, bool towgs) const
{
    jsobj.set( "type" , "Feature" );
    auto* info = new OD::JSON::Object;
    getInfo( *info );
    jsobj.set( "properties", info );
    auto* geom = new OD::JSON::Object;
    geom->set( "type", "Polygon" );
    auto* rings = new OD::JSON::Array( false ) ;
    auto* coords = new OD::JSON::Array( false );
    getPoints( *coords, towgs );
    rings->add( coords );
    geom->set( "coordinates", rings );
    jsobj.set( "geometry", geom );
}


void odSurvey::getPoints( OD::JSON::Array& points, bool towgs) const
{
    const TrcKeySampling tk = si().sampling( false ).hsamp_;
    TypeSet<Coord> coords;
    for ( int i=0; i<4; i++ )
	coords += tk.toCoord( tk.corner(i) );

    coords.swap( 2, 3 );
    coords += coords[0];
    makeCoordsList( points, coords, towgs );
}


void odSurvey::makeCoordsList( OD::JSON::Array& points,
			       const TypeSet<Coord>& coords, bool towgs ) const
{
    if ( towgs )
    {
	ConstRefMan<Coords::CoordSystem> coordsys = si().getCoordSystem();
	for ( const auto& coord : coords )
	{
	    auto* point = new OD::JSON::Array(OD::JSON::DataType::String);
	    const LatLong ll( LatLong::transform(coord, true, coordsys) );
	    point->add( strdup(toString(ll.lng_, 6)) );
	    point->add( strdup(toString(ll.lat_, 6)) );
	    points.add(point);
	}
    }
    else
    {
	for ( const auto& coord : coords )
	{
	    auto* point = new OD::JSON::Array(OD::JSON::DataType::Number);
	    point->add( coord.x );
	    point->add( coord.y );
	    points.add( point );
	}
    }
}


bool odSurvey::has2D() const
{
    return si().has2D();
}

bool odSurvey::has3D() const
{
    return si().has3D();
}

BufferString odSurvey::get_crsCode() const
{
    BufferString crscode;
    IOPar iop;
    ConstRefMan<Coords::CoordSystem> crs = si().getCoordSystem();
    if ( crs )
    {
	crs->fillPar( iop );
	crscode = iop.find( IOPar::compKey(sKey::Projection(),sKey::ID()) );
	crscode.replace("`", ":");
    }
    return crscode;
}

BufferString odSurvey::surveyPath() const
{
    FilePath fp(si().getDataDirName(), si().getDirName());
    return fp.fullPath();
}

BufferStringSet* odSurvey::getObjNames( const char* trgrpnm ) const
{
    BufferStringSet* res = nullptr;
    activate();
    const PtrMan<IODir> dbdir = IOM().getDir( trgrpnm );
    if ( dbdir )
    {
	res = new BufferStringSet;
	for ( int idx=0; idx<dbdir->size(); idx++ )
	{
	    const IOObj* ioobj = dbdir->getObjs()[idx];
	    if ( !ioobj )
		continue;

	    if ( !ioobj->isTmp() && ioobj->group() == trgrpnm )
		res->add( ioobj->name() );
	}
    }
    return res;
}


void odSurvey::getObjInfos( OD::JSON::Object& jsobj, const char* trgrpnm ) const
{
    activate();
    const PtrMan<IODir> dbdir = IOM().getDir( trgrpnm );
    auto* ids = new OD::JSON::Array( OD::JSON::String);
    auto* nms = new OD::JSON::Array( OD::JSON::String);
    auto* types = new OD::JSON::Array( OD::JSON::String);
    auto* trls = new OD::JSON::Array( OD::JSON::String);
    auto* files = new OD::JSON::Array( OD::JSON::String);
    bool havetype = false;
    if ( dbdir )
    {
	for ( int idx=0; idx<dbdir->size(); idx++ )
	{
	    const IOObj* ioobj = dbdir->getObjs()[idx];
	    if ( !ioobj )
		continue;

	    if ( !ioobj->isTmp() && ioobj->group() == trgrpnm )
	    {
		nms->add( ioobj->name() );
		ids->add( ioobj->key().toString() );
		trls->add( ioobj->translator() );

		BufferString typ;
		if ( ioobj->pars().get(sKey::Type(),typ) && !typ.isEmpty() )
		    havetype = true;
		types->add( typ );
		files->add( ioobj->mainFileName() );
	    }
	}
    }
    jsobj.set( "name", nms );
    jsobj.set( "id", ids );
    jsobj.set( "format", trls );
    if (havetype)
	jsobj.set( "type", types );

    jsobj.set( "files", files );
}


bool odSurvey::isObjPresent( const char* objname, const char* trgrpnm ) const
{
    activate();
    return IOM().isPresent( objname, trgrpnm );
}


IOObj* odSurvey::createObj( const char* objname, const char* trgrpnm,
			    const char* translkey, bool overwrite,
			    BufferString& errmsg ) const
{
    if ( !objname || !trgrpnm || !TranslatorGroup::hasGroup(trgrpnm) )
	return nullptr;

    if ( translkey &&
	 !TranslatorGroup::getGroup(trgrpnm).getTemplate(translkey,true) )
	return nullptr;

    if ( isObjPresent(objname, trgrpnm) )
    {
	if ( overwrite )
	{
	    PtrMan<IOObj> ioobj = IOM().get( objname, trgrpnm );
	    if ( !IOM().implRemove(ioobj->key(), true) )
	    {
		errmsg = "cannot remove existing object.";
		return nullptr;
	    }
	}
	else
	{
	    errmsg = "object already exists and overwrite is disabled.";
	    return nullptr;
	}
    }

    IOObjContext ctxt = TranslatorGroup::getGroup(trgrpnm).ioCtxt();
    ctxt.forread_ = false;
    ctxt.deftransl_ = translkey;
    IOM().to( ctxt.getSelKey() );
    CtxtIOObj ctio( ctxt );
    ctio.setName( objname );
    IOM().getEntry( ctio, false );
    if ( !ctio.ioobj_ || !IOM().commitChanges(*ctio.ioobj_) )
    {
	errmsg = "unable to create new object.";
	return nullptr;
    }

    return ctio.ioobj_;
}


extern "C" { mGlobal(General) const char* setDBMDataSource(const char*, bool); }

void odSurvey::activate() const {
    if (basedir_==curbasedir_ && survey_==cursurvey_)
        return;

    setDBMDataSource( FilePath(basedir_, survey_).fullPath(), true );
    curbasedir_ = basedir_;
    cursurvey_ = survey_;
}

void odSurvey::initModule( const char* odbindfnm )
{
    if ( AreProgramArgsSet() )
	return;

    const std::filesystem::path fp( odbindfnm );
    const int argc = GetArgC() < 0 ? 0 : GetArgC();
    SetBindings( fp.parent_path().string().c_str(), argc, GetArgV(), true,
		 fp.string().c_str() );
    InitBindings( moddeps, false );
}

BufferStringSet* odSurvey::getNames( const char* basedir )
{
    BufferStringSet* dirnms = nullptr;
    if ( IOMan::isValidDataRoot(basedir) )
    {
	dirnms = new BufferStringSet;
	SurveyDiskLocation::listSurveys( *dirnms, basedir );
    }
    return dirnms;
}

BufferStringSet odSurvey::getCommonItems( const BufferStringSet& list1,
					  const BufferStringSet& list2 )
{
    BufferStringSet res;
    for ( const auto* item : list1 )
    {
	if ( list2.isPresent(item->buf()) )
	    res.add( item->buf() );
    }
    return res;
}


void odSurvey::getInfos( OD::JSON::Array& jsarr, const char* basedir,
			 const BufferStringSet& fornames )
{
    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getNames( basedir );
    if ( fornames.isEmpty() )
 	nms = *allnms;
    else
	nms = getCommonItems( *allnms, fornames );

    jsarr.setEmpty();
    for ( const auto* nm : nms )
    {
	odSurvey survey( basedir, *nm );
	OD::JSON::Object info;
	survey.getInfo( info );
 	if ( info.isEmpty() )
 	    continue;

 	jsarr.add( info.clone() );
     }
}


void odSurvey::getFeatures( OD::JSON::Object& jsobj, const char* basedir,
			    const BufferStringSet& fornames )
{
    BufferStringSet nms;
    PtrMan<BufferStringSet> allnms = getNames( basedir );
    if ( fornames.isEmpty() )
 	nms = *allnms;
    else
	nms = getCommonItems( *allnms, fornames );

    jsobj.setEmpty();
    auto* features = new OD::JSON::Array( true );
    for ( const auto* nm : nms )
    {
	odSurvey survey( basedir, *nm );
	OD::JSON::Object info;
	survey.getFeature( info );
 	if ( info.isEmpty() )
 	    continue;

 	features->add( info.clone() );
     }

    jsobj.set( "type", "FeatureCollection" );
    jsobj.set( "features", features );
}


hSurvey survey_new( const char* basedir, const char* surveynm )
{
    return new odSurvey( basedir, surveynm );
}

void survey_del( hSurvey self )
{
    delete reinterpret_cast<odSurvey*>(self);
}

void survey_bin( hSurvey self, double x, double y, int* iline, int* xline )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    if ( p && iline && xline )
    {
	auto b2c = p->si().binID2Coord();
	const IdxPair binpos = b2c.transformBack( Coord(x, y) );
	*iline = binpos.first;
	*xline = binpos.second;
    }
}

void survey_bincoords( hSurvey self, double x, double y,
		       double* iline, double* xline )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    if ( p && iline && xline )
    {
	auto b2c = p->si().binID2Coord();
	const Coord coord = b2c.transformBackNoSnap( Coord(x, y) );
	*iline = coord.x;
	*xline = coord.y;
    }
}

void survey_coords( hSurvey self, int iline, int xline, double* x, double* y )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    if ( p && x && y )
    {
	const Coord coord = p->si().transform( BinID(iline, xline) );
	*x = coord.x;
	*y = coord.y;
    }
}

const char* survey_crs( hSurvey self )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    return strdup( p->get_crsCode().buf() );
}

const char* survey_feature( hSurvey self )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    OD::JSON::Object jsobj;
    p->getFeature( jsobj );
    return strdup( jsobj.dumpJSon().buf() );
}

const char* survey_features( const char* basedir, const hStringSet forsurveys )
{
    const auto* p = reinterpret_cast<BufferStringSet*>(forsurveys);
    OD::JSON::Object jsobj;
    odSurvey::getFeatures( jsobj, basedir, *p );
    return strdup( jsobj.dumpJSon().buf() );
}

bool survey_has2d( hSurvey self )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    return p->has2D();
}

bool survey_has3d( hSurvey self )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    return p->has3D();
}

bool survey_hasobject( hSurvey self, const char* objname, const char* trgrpnm )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    return p->isObjPresent( objname, trgrpnm );
}

const char* survey_info( hSurvey self )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    OD::JSON::Object jsobj;
    p->getInfo( jsobj );
    return strdup( jsobj.dumpJSon().buf() );
}

const char* survey_infos( const char* basedir, const hStringSet forsurveys )
{
    const auto* p = reinterpret_cast<BufferStringSet*>(forsurveys);
    OD::JSON::Array jsarr( true );
    odSurvey::getInfos( jsarr, basedir, *p );
    return strdup( jsarr.dumpJSon().buf() );
}

hStringSet survey_names( const char* basedir )
{
    return odSurvey::getNames( basedir );
}

const char* survey_path( hSurvey self )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    return strdup( p->surveyPath().buf() );
}

const char* survey_survtype( hSurvey self )
{
    const auto* p = reinterpret_cast<odSurvey*>(self);
    return strdup( p->type().buf() );
}

void initModule( const char* odbindfnm )
{
    odSurvey::initModule( odbindfnm );
}

void exitModule()
{
    IOM().applicationClosing.trigger();
    CloseBindings();
}

