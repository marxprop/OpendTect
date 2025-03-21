#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiosgmod.h"

#include "enums.h"

class NotifierAccess;

namespace OD
{
    mGlobal(uiOSG) bool useQOpenGL();
    mGlobal(uiOSG) void setOSGTimerCallbacks(const NotifierAccess&,
					     const NotifierAccess&);

    enum class WheelMode : std::int8_t { Never=0, Always=1, OnHover=2 };
				mDeclareNameSpaceEnumUtils(uiOSG,WheelMode)
    enum class StereoType : std::int8_t { None, RedCyan, QuadBuffer };
				mDeclareNameSpaceEnumUtils(uiOSG,StereoType)

} // namespace OD
