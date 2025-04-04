#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"

#include "pickretriever.h"
#include "uiattrdesced.h"

class BinIDValueSet;
class calcFingParsObject;

class uiAttrSel;
class uiButtonGroup;
class uiFPAdvancedDlg;
class uiGenInput;
class uiIOObjSel;
class uiLabel;
class uiRadioButton;
class uiSeis2DLineSel;
class uiTable;
class uiToolButton;

/*! \brief FingerPrint Attribute description editor */

mExpClass(uiAttributes) uiFingerPrintAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiFingerPrintAttrib);
public:
			uiFingerPrintAttrib(uiParent*,bool);
			~uiFingerPrintAttrib();

protected:

    uiTable*		table_;
    uiButtonGroup*	refgrp_;
    uiRadioButton*	refposbut_;
    uiRadioButton*	picksetbut_;
    uiToolButton*	getposbut_;
    uiGenInput*		statsfld_;
    uiGenInput*		refposfld_	= nullptr;
    uiGenInput*		refpos2dfld_;
    uiGenInput*		refposzfld_;
    uiIOObjSel*		picksetfld_;
    uiLabel*		manlbl_;
    uiSeis2DLineSel*	linefld_	= nullptr;

    ObjectSet<uiAttrSel> attribflds_;

    uiFPAdvancedDlg*	advanceddlg_;
    calcFingParsObject* calcobj_;

    void		insertRowCB(CallBacker*);
    void		deleteRowCB(CallBacker*);
    void		initTable(int);

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    BinIDValueSet*	createValuesBinIDSet(uiString&) const;
    BinID		get2DRefPos() const;

    WeakPtr<PickRetriever> pickretriever_;
    void		getPosPush(CallBacker*);
    void		pickRetrieved(CallBacker*);

    void		calcPush(CallBacker*);
    void		getAdvancedPush(CallBacker*);
    void		refSel(CallBacker*);

    bool		areUIParsOK() override;

			mDeclReqAttribUIFns
};
