#ifndef uivolprocchain_h
#define uivolprocchain_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: uivolprocchain.h,v 1.3 2008-08-05 20:01:29 cvskris Exp $
________________________________________________________________________


-*/

#include "iopar.h"
#include "factory.h"
#include "uidialog.h"

class uiListBox;
class uiButton;
class uiMenuItem;
class uiGenInput;

namespace VolProc
{

class Chain;
class Step;

class uiStepDialog : public uiDialog
{
public:
    			uiStepDialog(uiParent*,const uiDialog::Setup&,
				     Step*);

    bool		acceptOK(CallBacker*);
protected:

    uiGenInput*		namefld_;
    Step*		step_;
};


class uiChain : public uiDialog
{
public:
    mDefineFactory2ParamInClass( uiStepDialog, uiParent*, Step*, factory );

				uiChain(uiParent*,Chain&);

protected:
    bool			acceptOK(CallBacker*);
    bool			doSave();
    bool			doSaveAs();
    void			updateList();
    void			updateButtons();
    void			showPropDialog(int);

    void			newButtonCB(CallBacker*) {}
    void			loadButtonCB(CallBacker*);
    void			saveButtonCB(CallBacker*);
    void			saveAsButtonCB(CallBacker*);

    void			factoryClickCB(CallBacker*);
    void			stepClickCB(CallBacker*);
    void			stepDoubleClickCB(CallBacker*);
    void			addStepCB(CallBacker*);
    void			removeStepCB(CallBacker*);
    void			moveUpCB(CallBacker*);
    void			moveDownCB(CallBacker*);
    void			propertiesCB(CallBacker*);

    IOPar			restorepar_;

    Chain&			chain_;

    uiMenuItem*			newmenu_;
    uiMenuItem*			loadmenu_;
    uiMenuItem*			savemenu_;
    uiMenuItem*			saveasmenu_;

    uiListBox*			factorylist_;
    uiButton*			addstepbutton_;
    uiButton*			removestepbutton_;
    uiListBox*			steplist_;
    uiButton*			moveupbutton_;
    uiButton*			movedownbutton_;
    uiButton*			propertiesbutton_;
};


}; //namespace

#endif
