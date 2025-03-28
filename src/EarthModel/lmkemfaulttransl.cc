/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "lmkemfaulttransl.h"

#include "emfault3d.h"
#include "executor.h"
#include "survinfo.h"
#include "od_istream.h"


const char* lmkEMFault3DTranslator::pointtypestr() { return "FAULT_PTYPE"; }
const char* lmkEMFault3DTranslator::xstr()	   { return "FAULT_X"; }
const char* lmkEMFault3DTranslator::ystr()	   { return "FAULT_Y"; }
const char* lmkEMFault3DTranslator::zstr()	   { return "FAULT_Z"; }
const char* lmkEMFault3DTranslator::domainstr()	   { return "FAULT_DOMAIN"; }
const char* lmkEMFault3DTranslator::surveystr()	   { return "FAULT_SURVEY"; }
const char* lmkEMFault3DTranslator::domainunitstr()
				    { return "FAULT_DOMAIN_UNIT"; }
const char* lmkEMFault3DTranslator::distancunitestr()
				    { return "FAULT_DISTANCE_UNIT"; }
const char* lmkEMFault3DTranslator::lineidstr()	{ return "FAULT_LINEID"; }
const char* lmkEMFault3DTranslator::tracestr()	{ return "FAULT_TRACE"; }


#define mWriteFormatRecord(nm)				\
    formatfile << lmkEMFault3DTranslator::nm##str << ' ' <<	\
                  nm##interval.start << ' ' << nm##interval.stop << '\n'


lmkEMFault3DReader::lmkEMFault3DReader( EM::Fault3D& fault_, Conn* conn_,
        const char* formatfilename )
    : Executor("Reading fault ..." )
    , fault( fault_ )
    , conn( conn_ )
    , lastpt( -1 )
    , useinlcrl(false)
    , lastnode(0,0)
    , xinterval(-1,-1)
    , yinterval(-1,-1)
    , zinterval(-1,-1)
    , lineidinterval( -1, -1 )
    , traceinterval( -1, -1 )
    , pointtypeinterval(-1,-1)
    , domaininterval(-1,-1)
    , domainunitinterval(-1,-1)
    , distancuniteinterval(-1,-1)
{
    if ( !formatfilename || !*formatfilename )
	{ msg = tr("No format file name specified"); error = true; return; }

    od_istream formatstrm( formatfilename );
    if ( !formatstrm.isOK() )
	{ msg = tr("Cannot open format file"); error = true; return; }

    while ( formatstrm.isOK() )
    {
	BufferString fieldname; Interval<int> rg;
	formatstrm.getWord( fieldname );
	if ( !formatstrm.isOK() )
	    break;
	formatstrm.get( rg.start_ ).get( rg.stop_ );

	if ( fieldname==lmkEMFault3DTranslator::xstr() )
	    xinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::ystr() )
	    yinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::zstr() )
	    zinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::pointtypestr() )
	    pointtypeinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::domainstr() )
	    domaininterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::domainunitstr() )
	    domainunitinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::distancunitestr() )
	    distancuniteinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::lineidstr() )
	    lineidinterval = rg;
	else if ( fieldname==lmkEMFault3DTranslator::tracestr() )
	    traceinterval = rg;

	if ( !formatstrm.isOK() )
	    break;
    }

    if ( (  xinterval.start_==-1 || xinterval.stop_==-1 ||
	    yinterval.start_==-1 || yinterval.stop_==-1 ) &&
	 lineidinterval.start_!=-1 && lineidinterval.stop_!=-1 &&
	 traceinterval.start_!=-1 && traceinterval.stop_!=-1 )
    {
	useinlcrl = true;
    }
    else if (	xinterval.start_==-1 || xinterval.stop_==-1 ||
		yinterval.start_==-1 || yinterval.stop_==-1 ||
		zinterval.start_==-1 || zinterval.stop_==-1 ||
		pointtypeinterval.start_==-1 || pointtypeinterval.stop_==-1 )
    {
	msg = tr( "%1, %2 and %3 must be provided for reading")
            .arg( lmkEMFault3DTranslator::xstr() )
            .arg( lmkEMFault3DTranslator::ystr() )
            .arg( lmkEMFault3DTranslator::pointtypestr() );
	error = true;
	return;
    }

    if ( !conn->forRead() || !conn->isStream() )
    { msg = Translator::sBadConnection(); error = true; return; }

    error = false;
}


lmkEMFault3DReader::~lmkEMFault3DReader()
{
    delete conn;
}


int lmkEMFault3DReader::nextStep()
{
    if ( error )
	return ErrorOccurred();

    od_istream& strm = ((StreamConn*)conn)->iStream();

    char buf[] = " ";
    BufferString buffer;

    while ( strm.isOK() && buf[0]!='\n' )
    {
	strm.getBin(buf,1);
	buffer += buf;
    }

    if ( !strm.isOK() )
	return Finished();

    int reqlen = mMAX(pointtypeinterval.stop_, zinterval.stop_ );
    if ( useinlcrl )
    {
	reqlen = mMAX(reqlen, lineidinterval.stop_ );
	reqlen = mMAX(reqlen, traceinterval.stop_ );
    }
    else
    {
	reqlen = mMAX(reqlen, xinterval.stop_ );
	reqlen = mMAX(reqlen, yinterval.stop_ );
    }

    reqlen = mMAX(reqlen, domaininterval.stop_ );
    reqlen = mMAX(reqlen, domainunitinterval.stop_ );
    reqlen = mMAX(reqlen, distancuniteinterval.stop_ );

    if ( buffer.size()<reqlen ) { return ErrorOccurred(); }

    Coord3 pos;

    if ( useinlcrl )
    {
	BufferString str(&buffer[lineidinterval.start_-1]);
	str[lineidinterval.width()+1] = 0;
	int inl = str.toInt();

	str = &buffer[traceinterval.start_-1];
	str[traceinterval.width()+1] = 0;
	int crl = str.toInt();

	Coord coord = SI().transform( BinID(inl, crl ));

        pos.y_ = coord.x_;
        pos.x_ = coord.y_;
    }
    else
    {
	BufferString str(&buffer[xinterval.start_-1]);
	str[xinterval.width()+1] = 0;
        pos.x_ = str.toDouble();

	str = &buffer[yinterval.start_-1];
	str[yinterval.width()+1] = 0;
        pos.y_ = str.toDouble();
    }

    BufferString str = &buffer[zinterval.start_-1];
    str[zinterval.width()+1] = 0;
    pos.z_ = str.toDouble();

    str = &buffer[pointtypeinterval.start_-1];
    str[pointtypeinterval.width()+1] = 0;
    int pt = str.toInt();

    float zfac = 0.001;
    if ( domainunitinterval.start_ != -1 )
    {
	str = &buffer[domainunitinterval.start_-1];
	str[domainunitinterval.width()+1] = '\0';
	if ( str == "m" )
	    zfac = 1;
	else if ( str == "ft" )
	    zfac = mFromFeetFactorF;
	else if ( str != "ms" )
	    { msg = tr("Unknown time unit"); return ErrorOccurred(); }

	str = &buffer[domaininterval.start_-1];
	str[domaininterval.width()+1] = 0;
	const bool ist = str == "TIME";
	if ( (ist && SI().zIsTime()) || (ist && !SI().zIsTime()) )
	{
	    msg = tr("Z domain is not equal to survey domain");
	    return ErrorOccurred();
	}
    }
    pos.z_ *= zfac;

    RowCol newnode( lastnode.row(), lastnode.col()+1 );
    if ( pt==mLMK_START_PT && lastpt!=-1 && lastpt!=mLMK_END_PT )
	return ErrorOccurred();

    if ( lastpt==mLMK_END_PT )
    {
	if ( pt!=mLMK_START_PT )
	    return ErrorOccurred();

	newnode.row()++; newnode.col() = 0;
	fault.geometry().insertStick( newnode.row(), newnode.col(), pos,
			Coord3(0,1,0), false );
    }
    else
    {
	const EM::SubID subid( newnode.toInt64() );
	fault.geometry().insertKnot( subid, pos, false );
    }

    lastnode = newnode;
    lastpt = pt;

    return MoreToDo();
}


uiString lmkEMFault3DReader::uiMessage() const
{
    return msg.isEmpty() ? tr("Reading Fault") : msg;
}



// lmkEMFault3DTranslator
lmkEMFault3DTranslator::lmkEMFault3DTranslator( const char* nm, const char* unm)
    : EMSurfaceTranslator(nm,unm)
{}


lmkEMFault3DTranslator::~lmkEMFault3DTranslator()
{
}


Executor* lmkEMFault3DTranslator::reader( EM::Fault3D& hor, Conn* conn,
					const char* formatfilename )
{
    return new lmkEMFault3DReader( hor, conn, formatfilename );
}


Executor* lmkEMFault3DTranslator::writer( const EM::Fault3D& hor, Conn* conn,
					const char* formatfilename )
{
    return 0;
}
