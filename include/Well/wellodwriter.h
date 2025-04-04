#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "uistring.h"
#include "wellwriteaccess.h"
#include "wellio.h"

#include "od_iosfwd.h"

class DataBuffer;
class IOObj;
class uiString;
class ascostream;


namespace Well
{
class Data;
class Log;

/*!\brief Writes Well::Data to OpendTect file storage. */

mExpClass(Well) odWriter : public odIO
			 , public WriteAccess
{
mODTextTranslationClass(Well::odWriter)
public:
			odWriter(const IOObj&,const Data&,uiString& errmsg);
			odWriter(const char* fnm,const Data&,uiString& errmsg);
			~odWriter();

    static const char*	sKeyLogStorage()		{ return "Log storage";}

private:

    bool		needsInfoAndTrackCombined() const override
			{ return true; }

    bool		put() const override;
    bool		putInfo() const override;
    bool		putTrack() const override;
    bool		putLogs() const override;
    bool		putMarkers() const override;
    bool		putD2T() const override;
    bool		putCSMdl() const override;
    bool		putDispProps() const override;
    bool		putLog(const Log&) const override;
    bool		putDefLogs() const override;
    bool		swapLogs(const Log&,const Log&) const override;
    bool		renameLog(const char* oldnm,
				  const char* newnm) override;

    const uiString&	errMsg() const override { return odIO::errMsg(); }

    bool		putInfo(od_ostream&) const;
    bool		putTrack(od_ostream&) const;
    bool		putMarkers(od_ostream&) const;
    bool		putDefLogs(od_ostream&) const;
    bool		putD2T(od_ostream&) const;
    bool		putCSMdl(od_ostream&) const;
    bool		putDispProps(od_ostream&) const;

    void		setBinaryWriteLogs( bool yn )	{ binwrlogs_ = yn; }

    bool		binwrlogs_;

    bool		isFunctional() const override;

    bool		putLog(od_ostream&,const Log&,
				  const DataBuffer* databuf = nullptr) const;
    int			getLogIndex(const char* lognm ) const;
    bool		wrLogHdr(od_ostream&,const Log&) const;
    bool		wrLogData(od_ostream&,const Log&,
				  const DataBuffer* databuf = nullptr) const;
    DataBuffer*		getLogBuffer(od_istream&) const;
    bool		wrHdr(od_ostream&,const char*) const;
    bool		doPutD2T(bool) const;
    bool		doPutD2T(od_ostream&,bool) const;

    void		init();

    void		setStrmErrMsg(od_stream&,const uiString&) const;
    uiString		startWriteStr() const;

};

} // namespace Well
