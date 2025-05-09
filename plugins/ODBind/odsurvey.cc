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
    along with this program. If not, see <https://www.gnu.org/licenses/>.
________________________________________________________________________________

-*/

#include "odsurvey.h"

#include "coordsystem.h"
#include "filepath.h"
#include "genc.h"
#include "iodir.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "latlong.h"
#include "moddepmgr.h"
#include "odjson.h"
#include "settings.h"
#include "surveyfile.h"
#include "survinfo.h"
#include "transl.h"

#include <cstring>
#include <filesystem>

BufferString odSurvey::curbasedir_;
BufferString odSurvey::cursurvey_;

static const char* moddeps[] =
{
    "EarthModel",
    "Seis",
    "Well",
    nullptr
};


odSurvey::odSurvey( const char* surveynm, const char* basedir)
    : basedir_(basedir ? basedir : GetSettingsDataDir())
    , survey_(surveynm)
{
    errmsg_ = isValidDataRoot( basedir_.buf() );
    if ( !isOK() )
    {
	errmsg_ = BufferString( "Invalid data root: ", errmsg_ );
	return;
    }

    errmsg_ = isValidSurveyDir( FilePath(basedir_,survey_).fullPath() );
    if ( !isOK() )
    {
	errmsg_ = BufferString( "Named survey does not exist in ", basedir_ );
	return;
    }

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
    jsobj.set( "zdomain", strdup(info.zIsTime() ? "twt" : "depth") );
    jsobj.set( "xyunit", info.getXYUnitString(false) );
    jsobj.set( "zunit", info.getZUnitString(false) );
    jsobj.set( "srd", info.seismicReferenceDatum() );
    jsobj.set( "inl_step", info.inlDistance() );
    jsobj.set( "crl_step", info.crlDistance() );
}


void odSurvey::getFeature(OD::JSON::Object& jsobj, bool towgs) const
{
    jsobj.set( "type" , "Feature" );
    auto* info = new OD::JSON::Object;
    getInfo( *info );
    jsobj.set( "properties", info );
    auto* geom = new OD::JSON::Object;
    geom->set( "type", "Polygon" );
    auto* rings = new OD::JSON::Array( false );
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
	    const LatLong ll( LatLong::transform(coord, true, coordsys.ptr()) );
	    point->add( strdup(toString(ll.lng_, 0, 'f', 6)) );
	    point->add( strdup(toString(ll.lat_, 0, 'f', 6)) );
	    points.add(point);
	}
    }
    else
    {
	for ( const auto& coord : coords )
	{
	    auto* point = new OD::JSON::Array(OD::JSON::DataType::Number);
            point->add( coord.x_ );
            point->add( coord.y_ );
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
    if ( !trgrpnm )
    {
	errmsg_ = BufferString( "translator group name required" );
	return res;
    }

    if ( !activate() )
	return res;

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


void odSurvey::getObjInfoByID( const MultiID& mid,
			       OD::JSON::Object& jsobj ) const
{
    if ( !activate() )
	return;

    if ( mid.isUdf() || !IOM().isPresent(mid) )
    {
	errmsg_ = BufferString( "invalid database ID" );
	return;
    }

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj || !IOM().to(ioobj->key()) )
    {
	errmsg_ = BufferString( "invalid database object" );
	return;
    }

    if ( ioobj->isTmp() )
	return;

    jsobj.set( sKey::ID(), ioobj->key().toString() );
    jsobj.set( sKey::Name(), ioobj->name() );
    jsobj.set( sKey::Format(), ioobj->translator() );
    jsobj.set( TranslatorGroup::sKeyTransGrp(), ioobj->group() );
    const FilePath fp( ioobj->mainFileName() );
    jsobj.set( sKey::FileName(), fp );
    BufferString typ;
    if ( ioobj->pars().get(sKey::Type(),typ) && !typ.isEmpty() )
	jsobj.set( sKey::Type(), typ );
}


void odSurvey::getObjInfo( const char* objname, const char* trgrpnm,
			   OD::JSON::Object& jsobj ) const
{
    if (!objname )
    {
	errmsg_ = BufferString( "object name required" );
	return;
    }

    if ( !activate() )
	return;

    if ( trgrpnm && !TranslatorGroup::hasGroup(trgrpnm) )
    {
	errmsg_ = BufferString( "translator group not found");
	return;
    }

    PtrMan<IOObj> ioobj = IOM().get( objname, trgrpnm );
    if ( !ioobj || !IOM().to(ioobj->key()) )
    {
	errmsg_ = BufferString( "object not found" );
	return;
    }

    if ( ioobj->isTmp() )
	return;

    getObjInfoByID( ioobj->key(), jsobj );
}


void odSurvey::getObjInfos( const char* trgrpnm, bool all,
			    OD::JSON::Array& jsarr ) const
{
    jsarr.setEmpty();
    if ( !trgrpnm )
    {
	errmsg_ = BufferString( "translator group name required" );
	return;
    }

    if ( !activate() )
	return;

    const PtrMan<IODir> dbdir = IOM().getDir( trgrpnm );
    if ( dbdir )
    {
	for ( int idx=0; idx<dbdir->size(); idx++ )
	{
	    const IOObj* ioobj = dbdir->getObjs()[idx];
	    if ( !ioobj )
		continue;

	    if ( !ioobj->isTmp() &&
			(all || (!all && ioobj->group() == trgrpnm)) )
	    {
		OD::JSON::Object info;
		getObjInfoByID( ioobj->key(), info );
		if ( info.isEmpty() )
		    continue;

		jsarr.add( info.clone() );
	    }
	}
    }
}


bool odSurvey::isObjPresent( const char* objname, const char* trgrpnm ) const
{
    return activate() && IOM().isPresent( objname, trgrpnm );
}


IOObj* odSurvey::createObj( const char* objname, const char* trgrpnm,
			    const char* translkey, bool overwrite,
			    BufferString& errmsg ) const
{
    errmsg.setEmpty();
    if ( !objname || !trgrpnm || !TranslatorGroup::hasGroup(trgrpnm) )
	return nullptr;

    if ( translkey &&
	 !TranslatorGroup::getGroup(trgrpnm).getTemplate(translkey,true) )
	return nullptr;

    IOObjContext ctxt = TranslatorGroup::getGroup(trgrpnm).ioCtxt();
    ctxt.forread_ = false;
    ctxt.deftransl_ = translkey;
    IOM().to( ctxt.getSelKey() );
    if ( isObjPresent(objname, trgrpnm) )
    {
	if ( overwrite )
	{
	    PtrMan<IOObj> ioobj = IOM().get( objname, trgrpnm );
	    if ( !IOM().implRemove(ioobj->key(), true) )
	    {
		errmsg = BufferString( "cannot remove existing object - ",
				       objname );
		return nullptr;
	    }
	}
	else
	{
	    errmsg = BufferString( objname,
			       " already exists and overwrite is disabled." );
	    return nullptr;
	}
    }

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


void odSurvey::createObj( const char* objname, const char* trgrpnm,
			  const char* translkey, bool overwrite ) const
{
    PtrMan<IOObj> ioobj = createObj( objname, trgrpnm, translkey, overwrite,
				     errmsg_ );
}


void odSurvey::removeObj( const char* objname, const char* trgrpnm ) const
{
    if ( !objname )
    {
	errmsg_ = "object name required";
	return;
    }

    if ( !activate() )
	return;

    if ( trgrpnm && !TranslatorGroup::hasGroup(trgrpnm) )
    {
	errmsg_ = "translator group not found";
	return;
    }

    PtrMan<IOObj> ioobj = IOM().get( objname, trgrpnm );
    if ( !ioobj || !IOM().to(ioobj->key()) ||
				!IOM().implRemove(ioobj->key(), true) )
	errmsg_ = "cannot remove object";
}


void odSurvey::removeObjByID( const MultiID& mid ) const
{
    if ( mid.isUdf() )
	errmsg_ = "invalid database id";
    else if ( !activate() || !IOM().to(mid) ||
					!IOM().implRemove(mid, true) )
	errmsg_ = "cannot remove object";
}


extern "C" { mGlobal(General) const char* setDBMDataSource(const char*, bool); }

bool odSurvey::activate() const
{
    if( DBG::isOn(DBG_PROGSTART) )
    {
	BufferString msg( "Activate: ", survey_ );
	DBG::message( msg );
    }

    const bool hasiom = IOMan::isOK();
    if ( basedir_==curbasedir_ && survey_==cursurvey_ && hasiom )
	return true;

    const char* uirv = setDBMDataSource(
			  FilePath(basedir_, survey_).fullPath(), hasiom );
    if ( uirv && *uirv )
    {
	errmsg_ = BufferString( "survey activation failed: ", uirv );
	return false;
    }

    curbasedir_ = basedir_;
    cursurvey_ = survey_;

    return hasiom ? true : InitBindings( moddeps, false );
}


bool odSurvey::initModule( const char* odbindfnm )
{
    if ( AreProgramArgsSet() )
	return true;

    const std::filesystem::path fp( odbindfnm );
    const int argc = GetArgC() < 0 ? 0 : GetArgC();
    SetBindings( fp.parent_path().string().c_str(), argc, GetArgV(), true,
		 fp.string().c_str() );
    return true;
}

BufferStringSet* odSurvey::getNames( const char* base )
{
    BufferStringSet* dirnms = nullptr;
    BufferString basedir( base ? base : GetSettingsDataDir() );
    if ( !basedir.isEmpty() && IOMan::isValidDataRoot(basedir).isOK() )
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
    const BufferStringSet& srclist = list1.size()<list2.size() ? list1 : list2;
    const BufferStringSet& chklist = list1.size()<list2.size() ? list2 : list1;
    for ( const auto* item : srclist )
    {
	if ( chklist.isPresent(item->buf()) )
	    res.add( item->buf() );
    }
    return res;
}


void odSurvey::getInfos( OD::JSON::Array& jsarr,
			 const BufferStringSet& fornames, const char* base )
{
    BufferStringSet nms;
    jsarr.setEmpty();
    BufferString basedir( base ? base : GetSettingsDataDir() );
    if ( basedir.isEmpty() || !IOMan::isValidDataRoot(basedir).isOK() )
	return;

    PtrMan<BufferStringSet> allnms = getNames( basedir );
    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = getCommonItems( *allnms, fornames );

    for ( const auto* nm : nms )
    {
	odSurvey survey( *nm, basedir );
	OD::JSON::Object info;
	survey.getInfo( info );
	if ( info.isEmpty() )
	    continue;

	jsarr.add( info.clone() );
     }
}


void odSurvey::getFeatures( OD::JSON::Object& jsobj,
			    const BufferStringSet& fornames, const char* base )
{
    BufferStringSet nms;
    jsobj.setEmpty();
    BufferString basedir( base ? base : GetSettingsDataDir() );
    PtrMan<BufferStringSet> allnms = getNames( basedir );
    if ( basedir.isEmpty() || !IOMan::isValidDataRoot(basedir).isOK() )
	return;

    if ( fornames.isEmpty() )
	nms = *allnms;
    else
	nms = getCommonItems( *allnms, fornames );

    auto* features = new OD::JSON::Array( true );
    for ( const auto* nm : nms )
    {
	odSurvey survey( *nm, basedir );
	OD::JSON::Object info;
	survey.getFeature( info );
	if ( info.isEmpty() )
	    continue;

	features->add( info.clone() );
     }

    jsobj.set( "type", "FeatureCollection" );
    jsobj.set( "features", features );
}


TrcKeySampling odSurvey::tkFromRanges( const int32_t inlrg[3],
				       const int32_t crlrg[3] )
{
    StepInterval<int> linerg( inlrg[0], inlrg[1], inlrg[2] );
    StepInterval<int> trcrg( crlrg[0], crlrg[1], crlrg[2] );
    TrcKeySampling tk;
    tk.setLineRange( linerg );
    tk.setTrcRange( trcrg );
    return tk;
}


TrcKeyZSampling odSurvey::tkzFromRanges( const int32_t inlrg[3],
					 const int32_t crlrg[3],
					 const float zrg[3], bool zistime )
{
    StepInterval<float> z_rg( zrg[0], zrg[1], zrg[2] );
    const float zscale = zistime ? ZDomain::Time().userFactor()
				    : ZDomain::Depth().userFactor();
    z_rg.scale( 1.0/zscale );
    TrcKeyZSampling tkz;
    tkz.hsamp_ = tkFromRanges( inlrg, crlrg );
    tkz.zsamp_ = z_rg;
    return tkz;
}


hSurvey survey_new( const char* surveynm, const char* basedir )
{
    return new odSurvey( surveynm, basedir );
}

void survey_del( hSurvey self )
{
    delete static_cast<odSurvey*>(self);
}

void survey_bin( hSurvey self, double x, double y, int* iline, int* xline )
{
    const auto* p = static_cast<odSurvey*>(self);
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
    const auto* p = static_cast<odSurvey*>(self);
    if ( p && iline && xline )
    {
	auto b2c = p->si().binID2Coord();
	const Coord coord = b2c.transformBackNoSnap( Coord(x, y) );
        *iline = coord.x_;
        *xline = coord.y_;
    }
}

void survey_coords( hSurvey self, int iline, int xline, double* x, double* y )
{
    const auto* p = static_cast<odSurvey*>(self);
    if ( p && x && y )
    {
	const Coord coord = p->si().transform( BinID(iline, xline) );
        *x = coord.x_;
        *y = coord.y_;
    }
}

const char* survey_crs( hSurvey self )
{
    const auto* p = static_cast<odSurvey*>(self);
    return strdup( p->get_crsCode().buf() );
}

const char* survey_feature( hSurvey self )
{
    const auto* p = static_cast<odSurvey*>(self);
    OD::JSON::Object jsobj;
    p->getFeature( jsobj );
    return strdup( jsobj.dumpJSon().buf() );
}

const char* survey_features( const hStringSet forsurveys, const char* basedir )
{
    const auto* p = static_cast<BufferStringSet*>(forsurveys);
    OD::JSON::Object jsobj;
    odSurvey::getFeatures( jsobj, *p, basedir );
    return strdup( jsobj.dumpJSon().buf() );
}

bool survey_has2d( hSurvey self )
{
    const auto* p = static_cast<odSurvey*>(self);
    return p->has2D();
}

bool survey_has3d( hSurvey self )
{
    const auto* p = static_cast<odSurvey*>(self);
    return p->has3D();
}

bool survey_hasobject( hSurvey self, const char* objname, const char* trgrpnm )
{
    const auto* p = static_cast<odSurvey*>(self);
    return p->isObjPresent( objname, trgrpnm );
}

const char* survey_getobjinfo( hSurvey self, const char* objname,
			       const char* trgrpnm )
{
    const auto* p = static_cast<odSurvey*>(self);
    OD::JSON::Object jsobj;
    p->getObjInfo( objname, trgrpnm, jsobj );
    return strdup( jsobj.dumpJSon().buf() );
}

const char* survey_getobjinfobyid( hSurvey self, const char* id )
{
    const auto* p = static_cast<odSurvey*>(self);
    OD::JSON::Object jsobj;
    MultiID mid;
    mid.fromString( id );
    p->getObjInfoByID( mid, jsobj );
    return strdup( jsobj.dumpJSon().buf() );
}

const char* survey_getobjinfos( hSurvey self, const char* trgrpnm, bool all )
{
    const auto* p = static_cast<odSurvey*>(self);
    OD::JSON::Array jsarr( true );
    p->getObjInfos( trgrpnm, all, jsarr );
    return strdup( jsarr.dumpJSon().buf() );
}

void survey_removeobj( hSurvey self, const char* objname, const char* trgrpnm )
{
    const auto* p = static_cast<odSurvey*>(self);
    p->removeObj( objname, trgrpnm );
}

void survey_removeobjbyid( hSurvey self, const char* id )
{
    const auto* p = static_cast<odSurvey*>(self);
    MultiID mid;
    mid.fromString( id );
    p->removeObjByID( mid );
}

void survey_createobj( hSurvey self, const char* objname, const char* trgrpnm,
		       const char* translkey, bool overwrite)
{
    const auto* p = static_cast<odSurvey*>(self);
    p->createObj( objname, trgrpnm, translkey, overwrite );
}

hStringSet survey_getobjnames( hSurvey self, const char* trgrpnm )
{
    const auto* p = static_cast<odSurvey*>(self);
    return p->getObjNames( trgrpnm );
}

hStringSet survey_trgroups()
{
    const ObjectSet<TranslatorGroup>& grps = TranslatorGroup::groups();
    auto* res = new BufferStringSet;
    for ( const auto* grp : grps )
	res->add( BufferString(grp->groupName().buf(), " - ",
			       grp->typeName().getString()) );
    return res;
}

const char* survey_info( hSurvey self )
{
    const auto* p = static_cast<odSurvey*>(self);
    OD::JSON::Object jsobj;
    p->getInfo( jsobj );
    return strdup( jsobj.dumpJSon().buf() );
}

const char* survey_infos( const hStringSet forsurveys, const char* basedir )
{
    const auto* p = static_cast<BufferStringSet*>(forsurveys);
    OD::JSON::Array jsarr( true );
    odSurvey::getInfos( jsarr, *p, basedir );
    return strdup( jsarr.dumpJSon().buf() );
}

hStringSet survey_names( const char* basedir )
{
    return odSurvey::getNames( basedir );
}

const char* survey_path( hSurvey self )
{
    const auto* p = static_cast<odSurvey*>(self);
    return strdup( p->surveyPath().buf() );
}

const char* survey_survtype( hSurvey self )
{
    const auto* p = static_cast<odSurvey*>(self);
    return strdup( p->type().buf() );
}


const char* survey_errmsg( hSurvey self )
{
    const auto* p = static_cast<odSurvey*>(self);
    return p ? strdup( p->errMsg().buf() ) : nullptr;
}


bool survey_isok( hSurvey self )
{
    const auto* p = static_cast<odSurvey*>(self);
    return p ? p->isOK() : false;
}

void survey_zrange( hSurvey self, float* zrg )
{
    const auto* p = static_cast<odSurvey*>(self);
    if ( !p ) return;

    p->activate();
    StepInterval<float> z = SI().zRange();
    z.scale( SI().showZ2UserFactor() );
    zrg[0] = z.start_;
    zrg[1] = z.stop_;
    zrg[2] = z.step_;
}


void survey_inlrange( hSurvey self, int32_t* rg )
{
    const auto* p = static_cast<odSurvey*>(self);
    if ( !p ) return;

    p->activate();
    auto r = SI().inlRange();
    rg[0] = r.start_;
    rg[1] = r.stop_;
    rg[2] = r.step_;
}


void survey_crlrange( hSurvey self, int32_t* rg )
{
    const auto* p = static_cast<odSurvey*>(self);
    if ( !p ) return;

    p->activate();
    auto r = SI().crlRange();
    rg[0] = r.start_;
    rg[1] = r.stop_;
    rg[2] = r.step_;
}


bool initModule( const char* odbindfnm )
{
    return odSurvey::initModule( odbindfnm );
}


void exitModule()
{
    if ( IOMan::isOK() )
	IOM().applicationClosing.trigger();

    CloseBindings();
}



const char* isValidSurveyDir( const char* loc )
{
    mDeclStaticString( ret );
    const uiRetVal uirv = IOMan::isValidSurveyDir( loc );
    if ( uirv.isOK() )
	return nullptr;

    ret.set( uirv.getText().buf() );
    return ret.buf();
}


const char* isValidDataRoot( const char* loc )
{
    mDeclStaticString( ret );
    const uiRetVal uirv = IOMan::isValidDataRoot( loc );
    if ( uirv.isOK() )
	return nullptr;

    ret.set( uirv.getText().buf() );
    return ret.buf();
}


const char* survey_createtemp( const char* surveynm, const char* bsedir )
{
    EmptyTempSurvey tempsurvey( surveynm, bsedir, true, false );
    if ( !tempsurvey.isOK() )
    {
	mDeclStaticString( errmsg );
	errmsg.set( tempsurvey.errMsg().getText().buf() );
	return errmsg.buf();
    }

    return nullptr;
}
