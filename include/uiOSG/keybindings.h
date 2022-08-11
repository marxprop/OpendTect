#pragma once

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		Dec 2002
________________________________________________________________________

*/

#include "uiosgmod.h"
#include "bufstringset.h"

#include <osgGeo/TrackballManipulator>


/*!
\brief Class for setting keybindings.

  Each binding is a BufferString. This string contains the several keys
  separated by a back-quote.  e.g. zoom = "Left`Control"
*/

mExpClass(uiOSG) KeyBindings
{
public:
				KeyBindings(const char* nm=0)
				    : name_(nm) {};

    BufferString		name_;

    BufferString		zoom_;
    BufferString		rotate_;
    BufferString		pan_;


    static StringView		sName();
    static StringView		sRotate();
    static StringView		sPan();
    static StringView		sZoom();

    static StringView		sControl();
    static StringView		sShift();
    static StringView		sRight();
    static StringView		sLeft();
    static StringView		sMiddle();
    static StringView		sNone();

    static StringView		sSettingsKey();
};


/*!
\brief Event button.
*/

mExpClass(uiOSG) EventButton
{
public:
				EventButton() {}

    BufferString		mousebut_;
    BufferString		keybut_;
};


/*!
\brief Manages keybindings.
*/

mExpClass(uiOSG) KeyBindMan
{
public:
				KeyBindMan();
				~KeyBindMan();

    void			setTrackballManipulator(
						osgGeo::TrackballManipulator*);

    void			setKeyBindings(const char*);
    void			getAllKeyBindings(BufferStringSet&);
    const char*			getCurrentKeyBindings() const
				{ return curkeybinding_; }

protected:

    void			updateTrackballKeyBindings();

    ObjectSet<KeyBindings>	keyset_;
    BufferString		curkeybinding_;

    EventButton			zoom_;
    EventButton			pan_;
    EventButton			rotate_;

    osg::ref_ptr<osgGeo::TrackballManipulator> manipulator_;
};

