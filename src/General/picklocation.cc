/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
-*/


#include "picklocation.h"
#include "trigonometry.h"
#include "bufstring.h"
#include "survinfo.h"
#include "separstr.h"
#include "posimpexppars.h"
#include <ctype.h>


static char pipechar = '|';
static char newlinechar = '\n';
static const char* sKeyDip = "Dip";


#define mInitPtrs \
      trckey_(TrcKey::udf()) \
    , text_(0)

Pick::Location::Location( double x, double y, double zval )
    : pos_(x,y,zval)
    , mInitPtrs
{
}


Pick::Location::Location( const Coord& c, float zval )
    : pos_(c,zval)
    , mInitPtrs
{
}


Pick::Location::Location( const Coord3& c )
    : pos_(c)
    , mInitPtrs
{
}


Pick::Location::Location( const Coord3& c, const Coord3& d )
    : pos_(c)
    , dir_(d)
    , mInitPtrs
{
}


Pick::Location::Location( const Coord3& c, const Sphere& d )
    : pos_(c)
    , dir_(d)
    , mInitPtrs
{
}

Pick::Location::Location( const Location& oth )
    : mInitPtrs
{
    *this = oth;
}


Pick::Location::~Location()
{
    delete text_;
}


#define mSetMember(memb,typ,val) \
    if ( val && memb ) \
	*memb = *val; \
    else \
    { \
	delete memb; \
	if ( val ) \
	    memb = new typ( *val ); \
	else \
	    memb = 0; \
    }

void Pick::Location::operator=( const Location& oth )
{
    if ( this != &oth )
    {
#	define mCopyMemb( memb, typ ) mSetMember( memb, typ, oth.memb )
	pos_ = oth.pos_;
	dir_ = oth.dir_;
	trckey_ = oth.trckey_;
	mCopyMemb(text_,BufferString)
    }
}


bool Pick::Location::hasTrcKey() const
{
    return !trckey_.isUdf();
}


bool Pick::Location::hasText() const
{
    return text_ && !text_->isEmpty();
}


const TrcKey& Pick::Location::trcKey() const
{
    if ( hasTrcKey() || !hasPos() )
	return trckey_;

    mDefineStaticLocalObject( TrcKey, rettk, );
    rettk.setSurvID( Survey::GM().default3DSurvID() );
    rettk.setPosition( SI().transform(pos_) );
    return rettk;
}


bool Pick::Location::is2D() const
{
    return trcKey().is2D();
}


Pos::SurvID Pick::Location::survID() const
{
    return trcKey().survID();
}


Pos::GeomID Pick::Location::geomID() const
{
    return trcKey().geomID();
}


Pos::TraceID Pick::Location::lineNr() const
{
    return trcKey().lineNr();
}


Pos::TraceID Pick::Location::trcNr() const
{
    return trcKey().trcNr();
}


const BinID& Pick::Location::binID() const
{
    return trcKey().position();
}


const Sphere& Pick::Location::dir() const
{
    return dir_;
}


const BufferString& Pick::Location::text() const
{
    return text_ ? *text_ : BufferString::empty();
}


Pick::Location& Pick::Location::setTrcKey( const TrcKey& tk )
{ trckey_ = tk; return *this; }
Pick::Location& Pick::Location::setDir( const Sphere& sph )
{ dir_ = sph; return *this; }


Pick::Location& Pick::Location::setLineNr( Pos::LineID lnr )
{
    trckey_.setLineNr( lnr );
    return *this;
}


Pick::Location& Pick::Location::setTrcNr( Pos::TraceID tnr )
{
    trckey_.setTrcNr( tnr );
    return *this;
}


Pick::Location& Pick::Location::setBinID( const BinID& bid, bool updcoord )
{
    trckey_.setPosition( bid );
    if ( updcoord )
	setPos( trckey_.getCoord() );
    return *this;
}


Pick::Location& Pick::Location::setSurvID( Pos::SurvID survid, bool updpos )
{
    trckey_.setSurvID( survid );
    if ( updpos )
	trckey_.setFrom( pos_ );
    return *this;
}


Pick::Location& Pick::Location::setText( const char* txt )
{
    if ( !text_ )
    {
	if ( txt && *txt )
	    text_ = new BufferString( txt );
    }
    else if ( !txt || !*txt )
	{ delete text_; text_ = 0; }
    else
	*text_ = txt;

    return *this;
}


bool Pick::Location::getText( const char* idkey, BufferString& val ) const
{ return getKeyedText(idkey,val); }
bool Pick::Location::getKeyedText( const char* idkey, BufferString& val ) const
{
    if ( !text_ || !*text_ )
	{ val.setEmpty(); return false; }

    SeparString sepstr( *text_, '\'' );
    const int strsz = sepstr.size();
    if ( !strsz ) return false;

    for ( int idx=0; idx<strsz; idx+=2 )
    {
	if ( sepstr[idx] != idkey )
	    continue;

	val = sepstr[idx+1];
	return true;
    }

    return false;
}


void Pick::Location::setText( const char* key, const char* txt )
{ setKeyedText(key,txt); }
void Pick::Location::setKeyedText( const char* key, const char* txt )
{
    removeTextKey( key );
    if ( !text_ )
	text_ = new BufferString;
    SeparString sepstr( *text_, '\'' );
    sepstr.add( key );
    sepstr.add( txt );
    *text_ = sepstr;
}


void Pick::Location::unSetText( const char* key )
{ removeTextKey(key); }
void Pick::Location::removeTextKey( const char* key )
{
    if ( !text_ )
	return;

    SeparString sepstr( *text_, '\'' );
    for ( int idx=0; idx<sepstr.size(); idx+=2 )
    {
	if ( sepstr[idx] != key )
	    continue;

	SeparString copy( 0, '\'' );
	const int nrkeys = sepstr.size();
	for ( int idy=0; idy<nrkeys; idy++ )
	{
	    if ( idy==idx || idy==idx+1 )
		continue;

	    copy.add( sepstr[idy] );
	}

	sepstr = copy;
	idx-=2;
    }

    (*text_) = sepstr;
}


#define mReadVal(type,readFunc) \
{ \
    if ( !*str ) return mUdf(type); \
    char* endptr = str; mSkipNonBlanks( endptr ); \
    if ( *endptr ) *endptr++ = '\0'; \
    type v = readFunc( str ); \
    str = endptr; mSkipBlanks(str); \
    return v; \
}

static double getNextVal( char*& str )	{ mReadVal( double, toDouble ) }
static int getNextInt( char*& str )	{ mReadVal( int, toInt ) }


bool Pick::Location::fromString( const char* s )
{
    if ( !s || !*s )
	return false;

    if ( *s == '"' )
    {
	s++;

	if ( !text_ )
	    text_ = new BufferString( s );
	else
	    *text_ = s;

	char* start = text_->getCStr();
	char* stop = firstOcc( start, '"' );
	if ( !stop )
	    { delete text_; text_ = 0; }
	else
	{
	    *stop = '\0';
	    s += stop - start + 1;
	    text_->replace( newlinechar, pipechar );
	}
    }
    else if ( text_ )
	{ delete text_; text_ = 0; }

    BufferString bufstr( s );
    char* str = bufstr.getCStr();
    mSkipBlanks(str);

    Coord3 posread;
    posread.x = getNextVal( str );
    posread.y = getNextVal( str );
    posread.z = getNextVal( str );
    if ( posread.isUdf() )
	return false;

    pos_ = posread;

    mSkipBlanks(str);
    const FixedString data( str );
    if ( data.count( '\t' ) > 1 )
    { // Read the direction too before any trace key information
	Coord3 dirread;
	dirread.x = getNextVal( str );
	dirread.y = getNextVal( str );
	dirread.z = getNextVal( str );

	if ( !mIsUdf(dirread.y) )
	{
	    if ( mIsUdf(dirread.z) )
		dirread.z = 0.;
	    dir_ = Sphere( dirread );
	}
    }

    mSkipBlanks(str);

    //Old files: trckey_ left undef
    if ( !hasTrcKey() )
	return true;
    const Pos::SurvID survid( trckey_.survID() );
    if ( survid == TrcKey::cUndefSurvID() || !str )
	return true;

    const int firstkey = getNextInt( str );
    if ( trckey_.is2D() )
    {
	if ( Survey::GM().getGeometry(firstkey) )
	    trckey_.setLineNr( firstkey );
    }
    else
    {
	if ( !Survey::GM().getGeometry3D(survid) )
	    return false;

	trckey_.setLineNr( firstkey ); //No check for valid inline number ?
    }

    trckey_.setTrcNr( getNextInt(str) ); //No check for valid trace number ?

    return !trckey_.pos().isUdf();
}


void Pick::Location::toString( BufferString& str, bool forexport ) const
{
    str.setEmpty();
    if ( text_ && *text_ )
    {
	BufferString txt( *text_ );
	txt.replace( newlinechar, pipechar );
	str.set( "\"" ).add( txt ).add( "\"\t" );
    }

    Coord3 usepos( pos_ );
    if ( forexport )
    {
	mPIEPAdj(Coord,usepos,false);
	if ( mPIEP.haveZChg() )
	{
	    float zval = (float)usepos.z;
	    mPIEPAdj(Z,zval,false);
	    usepos.z = zval;
	}

	usepos.z = usepos.z * SI().showZ2UserFactor();
    }

    str.add( usepos.x ).add( od_tab ).add( usepos.y );
    str.add( od_tab ).add( usepos.z );
    if ( hasDir() )
	str.add( od_tab ).add( dir_.radius ).add( od_tab )
	   .add( dir_.theta ).add( od_tab ).add( dir_.phi );

    if ( trckey_.isUdf() || trckey_.position().isUdf() )
	return;

    //actually both calls return the same, but for the clarity
    if ( trckey_.is2D() )
	str.add( od_tab ).add( trckey_.geomID() );
    else
	str.add( od_tab ).add( trckey_.lineNr() );

    str.add( od_tab ).add( trckey_.trcNr() );
}


void Pick::Location::setDip( float inldip, float crldip )
{
    SeparString dipvaluetext;
    dipvaluetext += ::toString( inldip );
    dipvaluetext += ::toString( crldip );
    setKeyedText( sKeyDip, dipvaluetext.buf() );
}


float Pick::Location::inlDip() const
{
    BufferString dipvaluetext;
    getKeyedText( sKeyDip, dipvaluetext );
    const SeparString dipstr( dipvaluetext );
    return dipstr.getFValue( 0 );
}


float Pick::Location::crlDip() const
{
    BufferString dipvaluetext;
    getKeyedText( sKeyDip, dipvaluetext );
    const SeparString dipstr( dipvaluetext );
    return dipstr.getFValue( 1 );
}
