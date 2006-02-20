/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2001
-*/

static const char* rcsID = "$Id: attribdescsettr.cc,v 1.3 2006-02-20 18:49:48 cvsbert Exp $";

#include "attribdescsettr.h"
#include "attrfact.h"
#include "attribdescset.h"
#include "bufstringset.h"
#include "ioobj.h"
#include "iopar.h"
#include "conn.h"
#include "ptrman.h"


const IOObjContext& AttribDescSetTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Attr;
    }

    return *ctxt;
}


int AttribDescSetTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Attribute definitions",key) ) return 1;
    return 0;
}


bool AttribDescSetTranslator::retrieve( Attrib::DescSet& ads,
					const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    PtrMan<AttribDescSetTranslator> tr
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->getTranslator());
    if ( !tr ) { bs = "Selected object is not an Attribute Set"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( ads, *conn );
    bool rv = bs == "";
    if ( rv ) bs = tr->warningMsg();
    return rv;
}


bool AttribDescSetTranslator::store( const Attrib::DescSet& ads,
				     const IOObj* ioobj, BufferString& bs )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    PtrMan<AttribDescSetTranslator> tr
	= dynamic_cast<AttribDescSetTranslator*>(ioobj->getTranslator());
    if ( !tr ) { bs = "Selected object is not an Attribute Set"; return false; }
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
	{ bs = "Cannot open "; bs += ioobj->fullUserExpr(false); return false; }
    bs = tr->write( ads, *conn );
    return bs == "";
}


const char* dgbAttribDescSetTranslator::read( Attrib::DescSet& ads, Conn& conn )
{
    warningmsg = "";
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    IOPar iopar, bupar;
    iopar.read( ((StreamConn&)conn).iStream(), mTranslGroupName(AttribDescSet));
    ads.fillPar( bupar );
    ads.removeAll();
    BufferStringSet parseerrmsgs;
    ads.usePar( iopar, &parseerrmsgs );

    if ( !ads.nrDescs() )
    {
	ads.usePar( bupar );
	return "Could not find any attribute definitions in file";
    }
    
    if ( parseerrmsgs.size() )
    {
	warningmsg = *parseerrmsgs[0];
	const int nrdispl = parseerrmsgs.size() > 3 ? 4 : parseerrmsgs.size();
	for ( int idx=1; idx<nrdispl; idx++ )
	{
	    warningmsg += "\n";
	    warningmsg += *parseerrmsgs[idx];
	}
	if ( parseerrmsgs.size() > 4 )
	{
	    warningmsg += "\n";
	    warningmsg += "[More warnings omitted]";
	}
    }

    return 0;
}


const char* dgbAttribDescSetTranslator::write( const Attrib::DescSet& ads,
						Conn& conn )
{
    warningmsg = "";
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    IOPar iopar( "Attribute Descriptions" );
    ads.fillPar( iopar );
    if ( !iopar.write( ((StreamConn&)conn).oStream(),
		mTranslGroupName(AttribDescSet) ) )
	return "Cannot write attributes to file";

    return 0;
}
