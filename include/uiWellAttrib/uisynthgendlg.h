#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "uidialog.h"
#include "uigroup.h"
#include "uistring.h"

class SynthGenParams;
namespace StratSynth { class DataMgr; }

class uiComboBox;
class uiFullSynthSeisSel;
class uiGenInput;
class uiListBox;
class uiLabeledComboBox;
class uiPushButton;

mExpClass(uiWellAttrib) uiSynthParsGrp : public uiGroup
{ mODTextTranslationClass(uiSynthParsGrp);
public:
			uiSynthParsGrp(uiParent*,StratSynth::DataMgr&);
			~uiSynthParsGrp();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

    CNotifier<uiSynthParsGrp,SynthID> synthAdded;
    CNotifier<uiSynthParsGrp,SynthID> synthSelected;

    static bool		getNewElPropSel(uiParent*,StratSynth::DataMgr&);

    enum ScaleRes { CANCELLED, ALLDONE, IGNORED, MARKED, SCALED, SCALEERROR };
    static ScaleRes checkUnscaledWavelets(uiParent*,StratSynth::DataMgr&);
    static bool		isOK(ScaleRes);

private:

    void			initGrp(CallBacker*);
    void			addSyntheticsCB(CallBacker*);
    void			newSynthSelCB(CallBacker*);
    void			updateSyntheticsCB(CallBacker*);
    void			removeSyntheticsCB(CallBacker*);
    void			scaleSyntheticsCB(CallBacker*);
    void			elPropSelCB(CallBacker*);
    void			newCB(CallBacker*);
    void			openCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			expSynthCB(CallBacker*);
    void			needScaleCB(CallBacker*);
    void			newWvltCB(CallBacker*);
    void			scalingDoneCB(CallBacker*);
    void			typeChgCB(CallBacker*);
    void			parsChangedCB(CallBacker*);
    void			nameChangedCB(CallBacker*);

    bool			getFromScreen(SynthGenParams&);
    void			putToScreen();

    bool			confirmSave();
    bool			doSave(const char* fnm);
    void			getPSNames(BufferStringSet&);
    void			getAttributeInpNames(BufferStringSet&);
    void			getFilteringInpNames(BufferStringSet&, bool);
    void			forwardInputNames(const SynthGenParams*);
    bool			prepareSyntheticToBeRemoved();
    bool			checkSyntheticPars(const SynthGenParams&,
						   bool isupdate);

    static bool			haveUserScaleWavelet(uiParent*,
						     const StratSynth::DataMgr&,
						     const MultiID& unscaledid,
						     MultiID& scaleid);
    static ScaleRes		doScaling(uiParent*,StratSynth::DataMgr&,
					  const TypeSet<MultiID>& wvltids);

    StratSynth::DataMgr&	stratsynth_;
    uiListBox*			synthnmlb_;
    uiPushButton*		updatefld_;
    uiPushButton*		removefld_;
    uiPushButton*		scalefld_;

    uiFullSynthSeisSel*		synthselgrp_;
    uiPushButton*		addnewfld_;

    BufferString		lastsavedfnm_;
    bool			parschanged_ = false;
    bool			namechanged_ = false;

};


mExpClass(uiWellAttrib) uiSynthGenDlg : public uiDialog
{ mODTextTranslationClass(uiSynthGenDlg);
public:
				uiSynthGenDlg(uiParent*,StratSynth::DataMgr&);
				~uiSynthGenDlg();

    uiSynthParsGrp*		grp()		{ return uisynthparsgrp_; }
    const uiSynthParsGrp*	grp() const	{ return uisynthparsgrp_; }

private:

    uiSynthParsGrp*		uisynthparsgrp_;
};
