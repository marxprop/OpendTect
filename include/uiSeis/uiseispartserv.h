#ifndef uiseispartserv_h
#define uiseispartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiapplserv.h"
#include "multiid.h"
#include "uistring.h"

class BufferStringSet;
class TrcKeyZSampling;
class IOPar;
class SeisTrcBuf;
class uiFlatViewWin;
class uiSeisFileMan;
class uiSeisPreStackMan;
class uiSeisWvltMan;

namespace PosInfo { class Line2DData; }
namespace Geometry { class RandomLine; }

/*!
\brief Seismic User Interface Part Server
*/

mExpClass(uiSeis) uiSeisPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiSeisPartServer);
public:
			uiSeisPartServer(uiApplService&);
			~uiSeisPartServer();

    const char*		name() const			{ return "Seismics"; }

    bool		importSeis(int opt);
    bool		exportSeis(int opt);

    bool		select2DSeis(MultiID&);
    bool		select2DLines(TypeSet<Pos::GeomID>&,int& action);
    static void		get2DStoredAttribs(const char* linenm,
				       BufferStringSet& attribs,int steerpol=2);
    void		get2DZdomainAttribs(const char* linenm,
					    const char* zdomainstr,
					    BufferStringSet& attribs);
    bool		create2DOutput(const MultiID&,const char* linekey,
				       TrcKeyZSampling&,SeisTrcBuf&);
    void		getStoredGathersList(bool for3d,BufferStringSet&) const;
    void		storeRlnAs2DLine(const Geometry::RandomLine&) const;

    void		processTime2Depth() const;
    void		processVelConv() const;
    void		createMultiCubeDataStore() const;

    void		manageSeismics(int opt,bool modal=false);
    void		managePreLoad();
    void		importWavelets();
    void		exportWavelets();
    void		manageWavelets();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		ioSeis(int,bool);
    void		survChangedCB(CallBacker*);

    uiSeisFileMan*	man2dseisdlg_;
    uiSeisFileMan*	man3dseisdlg_;
    uiSeisPreStackMan*	man2dprestkdlg_;
    uiSeisPreStackMan*	man3dprestkdlg_;
    uiSeisWvltMan*	manwvltdlg_;
};

#endif

