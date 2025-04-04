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
#include "odsurvey_object.h"

#include "ioman.h"
#include "ioobj.h"
#include "odjson.h"
#include "ranges.h"
#include "trckeyzsampling.h"


odSurveyObject::odSurveyObject( const odSurvey& thesurvey, const char* name,
				const char* tgname )
    : survey_(thesurvey)
    , name_(name)
    , tgname_(tgname)
    , readonly_(true)
    , zistime_(SI().zIsTime())
{
    ConstPtrMan<IOObj> ioobj( ioobj_ptr() );
}


odSurveyObject::odSurveyObject( const odSurvey& thesurvey, const char* name,
				const char* tgname, bool overwrite,
				const char* fmt )
    : survey_(thesurvey)
    , name_(name)
    , tgname_(tgname)
    , overwrite_(overwrite)
    , readonly_(false)
    , zistime_(SI().zIsTime())
{
    survey_.activate();
    if ( tgname )
    {
	ConstPtrMan<IOObj> ioobj( survey_.createObj( name, tgname, fmt,
						     overwrite, errmsg_) );
	if ( !ioobj || !errmsg_.isEmpty() )
	    errmsg_.insertAt( 0, "IO object creation error: " );
	else
	    name_ = ioobj->name();
    }
}


odSurveyObject::~odSurveyObject()
{}


IOObj*	odSurveyObject::ioobj_ptr() const
{
    survey_.activate();
    IOObj* ioobj = IOM().get( name_, tgname_ );
    if ( !ioobj )
    {
	errmsg_ = IOM().message();
	errmsg_.insertAt( 0, "IO object read error - " );
    }

    return ioobj;
}

BufferString odSurveyObject::getName() const
{
    return name_;
}


void odSurveyObject::getFeature( OD::JSON::Object& jsobj, bool towgs ) const
{
    jsobj.set( "type", "Feature" );
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


bool odSurveyObject::canRead() const
{
    if ( !readonly_ )
    {
	errmsg_ = "cannot read, object is write only.";
	return false;
    }

    return true;
}


bool odSurveyObject::canWrite() const
{
    if ( readonly_ )
    {
	errmsg_ = "cannot write, object is read only.";
	return false;
    }

    return true;
}



