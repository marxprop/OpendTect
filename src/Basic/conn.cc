/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Connections
-*/

static const char* rcsID = "$Id: conn.cc,v 1.5 2001-05-31 12:55:11 windev Exp $";

#include "conn.h"
#include "strmprov.h"
#include "strmoper.h"
#include <fstream>


DefineEnumNames(StreamConn,Type,0,"Type")
	{ "File", "Device", "Command", 0 };


StreamConn::StreamConn()
	: mine(true)
	, nrretries(0)
	, retrydelay(0)
	, state_(Bad)
{
    fname = new char[1024];
    *fname = '\0';
}


StreamConn::StreamConn( istream* s )
	: mine(true), fname(0)
	, nrretries(0)
	, retrydelay(0)
	, state_(Read)
{
    sd.istrm = s;
    (void)bad();
}


StreamConn::StreamConn( ostream* s )
	: mine(true), fname(0)
	, nrretries(0)
	, retrydelay(0)
	, state_(Write)
{
    sd.ostrm = s;
    (void)bad();
}


StreamConn::StreamConn( const StreamData& strmdta )
	: sd(strmdta), mine(true), fname(0)
	, nrretries(0)
	, retrydelay(0)
{
    if		( !sd.usable() )	state_ = Bad;
    else if	( sd.istrm )		state_ = Read;
    else if	( sd.ostrm )		state_ = Write;
}


StreamConn::StreamConn( istream& s )
	: mine(false), fname(0)
	, nrretries(0)
	, retrydelay(0)
	, state_(Read)
{
    sd.istrm = &s;
    (void)bad();
}


StreamConn::StreamConn( ostream& s )
	: mine(false), fname(0)
	, nrretries(0)
	, retrydelay(0)
	, state_(Write)
{
    sd.ostrm = &s;
    (void)bad();
}


StreamConn::StreamConn( const char* nm, State s )
	: mine(false), fname(0)
	, nrretries(0)
	, retrydelay(0)
	, state_(s)
{
    if ( nm && *nm )
    {
	fname = new char[ strlen(nm) + 1 ];
	strcpy( fname, nm );
    }

    switch ( state_ )
    {
    case Read:
	if ( !fname ) sd.istrm = &cin;
	else
	{
	    StreamProvider sp( fname );
	    sd = sp.makeIStream();
	}
    break;
    case Write:
	if ( !fname ) sd.ostrm = &cout;
	else
	{
	    StreamProvider sp( fname );
	    sd = sp.makeOStream();
	}
    break;
    default:
    break;
    }
}


StreamConn::~StreamConn()
{
    delete [] fname;
    close();
}


bool StreamConn::bad() const
{
    if ( state_ == Bad ) return YES;
    if ( !sd.usable() ) (State&)state_ = Bad;
    return state_ == Bad;
}


void StreamConn::clearErr()
{
    if ( forWrite() ) { oStream().flush(); oStream().clear(); }
    if ( forRead() ) iStream().clear();
}


void StreamConn::close()
{
    if ( mine )
	sd.close();
    else if ( state_ == Read && sd.istrm && sd.istrm != &cin )
    {
	ifstream* s = dynamic_cast<ifstream*>( sd.istrm );
	if ( s ) s->close();
    }
    else if ( state_ == Write && sd.ostrm
	   && sd.ostrm != &cout && sd.ostrm != &cerr )
    {
	ofstream* s = dynamic_cast<ofstream*>( sd.ostrm );
	if ( s ) s->close();
    }
}


bool StreamConn::doIO( void* ptr, unsigned int nrbytes )
{
    if ( forWrite()
      && !writeWithRetry( oStream(), ptr, nrbytes, nrRetries(), retryDelay() ) )
	return false;

    if ( forRead() )
	return readWithRetry( iStream(), ptr,nrbytes,nrRetries(),retryDelay() );

    return true;
}
