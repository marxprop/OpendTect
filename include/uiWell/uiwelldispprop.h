#ifndef uiwelldispprop_h
#define uiwelldispprop_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Dec 2008
 RCS:           $Id: uiwelldispprop.h,v 1.1 2008-12-05 14:43:58 cvsbert Exp $
________________________________________________________________________

-*/


#include "uigroup.h"
#include "welldisp.h"

class uiSpinBox;
class uiCheckBox;
class uiColorInput;
class uiGenInput;


class uiWellDispProperties : public uiGroup
{
public:

    class Setup
    {
    public:
			Setup( const char* sztxt=0, const char* coltxt=0 )
			    : sztxt_(sztxt ? sztxt : "Line thickness")
			    , coltxt_(sztxt ? sztxt : "Line color")	{}

	mDefSetupMemb(BufferString,sztxt)
	mDefSetupMemb(BufferString,coltxt)
    };

    			uiWellDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::BasicProps&);

    Well::DisplayProperties::BasicProps& props()	{ return props_; }

    void		putOnScreen();
    void		getFromScreen();

protected:

    virtual void	doPutOnScreen()			{}
    virtual void	doGetFromScreen()		{}

    Well::DisplayProperties::BasicProps&	props_;

    uiColorInput*	colfld_;
    uiSpinBox*		szfld_;

};


class uiWellTrackDispProperties : public uiWellDispProperties
{
public:
    			uiWellTrackDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Track&);

    Well::DisplayProperties::Track&	trackprops()
	{ return static_cast<Well::DisplayProperties::Track&>(props_); }

protected:

    virtual void	doPutOnScreen();
    virtual void	doGetFromScreen();

    uiCheckBox*		dispabovefld_;
    uiCheckBox*		dispbelowfld_;
};


class uiWellMarkersDispProperties : public uiWellDispProperties
{
public:
    			uiWellMarkersDispProperties(uiParent*,const Setup&,
					Well::DisplayProperties::Markers&);

    Well::DisplayProperties::Markers&	mrkprops()
	{ return static_cast<Well::DisplayProperties::Markers&>(props_); }

protected:

    virtual void	doPutOnScreen();
    virtual void	doGetFromScreen();

    uiGenInput*		circfld_;

};



#endif
