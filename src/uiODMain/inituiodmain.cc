/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "procdescdata.h"
#include "odsession.h"

#include "uiodcontourtreeitem.h"
#include "uiodmain.h"
#include "uiodscenemgr.h"
#include "uiodvolproctreeitem.h"

mDefModInitFn(uiODMain)
{
    mIfNotFirstTime( return );

    ODSessionTranslatorGroup::initClass();
    dgbODSessionTranslator::initClass();
    uiKeyBindingSettingsGroup::initClass();
    VolProc::uiDataTreeItem::initClass();
    uiContourTreeItem::initClass();

#ifdef __win__
    ePDD().add( ePDD().sKeyODExecNm(), uiODMain::sODDesc(),
						ProcDesc::DataEntry::ODv6 );
#endif
}
