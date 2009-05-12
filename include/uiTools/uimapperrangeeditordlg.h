
#ifndef uimapperrangeeditordlg_h
#define uimapperrangeeditordlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditordlg.h,v 1.6 2009-05-12 09:08:54 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include "datapack.h"

class uiPushButton;
class uiMapperRangeEditor;
namespace ColTab { struct MapperSetup; class Sequence; };

mClass uiMultiMapperRangeEditWin : public uiDialog
{
public:
    					uiMultiMapperRangeEditWin(uiParent*,
						int n,DataPackMgr::ID dmid);
					~uiMultiMapperRangeEditWin();

    uiMapperRangeEditor*		getuiMapperRangeEditor(int);
    void				setDataPackID(int,DataPack::ID);
    void				setColTabMapperSetup(int,
	    					const ColTab::MapperSetup&);
    void				setColTabSeq(int,
	    					const ColTab::Sequence&);
    int					activeAttrbID()
    					{ return activeattrbid_; }
    const ColTab::MapperSetup&		activeMapperSetup()
    					{ return *activectbmapper_; }
    Notifier<uiMultiMapperRangeEditWin> 	rangeChange;

protected:

    uiPushButton*			statbut_;
    ObjectSet<uiMapperRangeEditor>	mapperrgeditors_;
    int					activeattrbid_;
    const ColTab::MapperSetup*        	activectbmapper_;
    DataPackMgr&                	dpm_; 
    TypeSet<DataPack::ID>   		datapackids_;
    
    void				rangeChanged(CallBacker*);   
    void				showStatDlg(CallBacker*);   
    void				dataPackDeleted(CallBacker*); 
};

#endif
