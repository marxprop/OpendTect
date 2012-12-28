#ifndef uitextureattrib_h
#define uitextureattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        P.F.M. de Groot
 Date:          September 2012
 RCS:           $Id: uitextureattrib.h 27530 2012-11-19 09:49:13Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiLabel;
class uiSteeringSel;
class uiStepOutSel;
class uiPushButton;
class SeisTrcBuf;
class CubeSampling;
class LineKey;


class uiTextureAttrib : public uiAttrDescEd
{
public:
			uiTextureAttrib(uiParent*,bool);
    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiSteeringSel*	steerfld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		actionfld_;
    uiGenInput*		glcmsizefld_;
    uiGenInput*		globalminfld_;
    uiGenInput*		globalmaxfld_;
    uiPushButton*	analysebut_;

    void		analyseCB(CallBacker*); 
    void		readSampAttrib(CubeSampling&,int,LineKey&);
    void		setMinMaxVal(const SeisTrcBuf&);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);
				mDeclReqAttribUIFns
};

#endif
