/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"
#include "odwindow.h"

#define mBody static_cast<uiDialogBody*>(body_)

uiDialog::Setup::Setup( const uiString& window_title,
			const uiString& dialog_title,
			const HelpKey& help_key )
    : wintitle_(window_title)
    , dlgtitle_(dialog_title)
    , helpkey_(help_key)
{}


uiDialog::Setup::Setup( const uiString& window_title, const HelpKey& help_key )
    : wintitle_(window_title)
    , helpkey_(help_key)
{}


uiDialog::Setup::~Setup()
{}


uiDialog::uiDialog( uiParent* p, const uiDialog::Setup& s )
    : uiMainWin( s.wintitle_, p )
    , applyPushed(this)
    , cancelpushed_(false)
{
    body_= new uiDialogBody( *this, p, s );
    setBody( body_ );
    body_->construct( s.nrstatusflds_, s.menubar_ );
    uiGroup* cw = new uiGroup( body_->uiCentralWidg(), "Dialog central widget");
    cw->setStretch( 2, 2 );
    mBody->setDlgGrp( cw );
    setTitleText( s.dlgtitle_ );
    ctrlstyle_ = OkAndCancel;
}


uiDialog::~uiDialog()
{}


void uiDialog::setButtonText( Button but, const uiString& txt )
{
    switch ( but )
    {
	case OK		: setOkText( txt ); break;
	case CANCEL	: setCancelText( txt ); break;
	case APPLY	: mBody->setApplyText( txt ); break;
	case HELP	: pErrMsg("set help txt but"); break;
	case SAVE	: enableSaveButton( txt ); break;
	case CREDITS	: pErrMsg("set credits txt but"); break;
    }
}


void uiDialog::setCtrlStyle( uiDialog::CtrlStyle cs )
{
    uiString canceltext = uiStrings::sClose();

    switch ( cs )
    {
    case OkAndCancel:
	setOkCancelText( uiStrings::sOk(), uiStrings::sCancel() );
    break;
    case RunAndClose:
	setOkCancelText( toUiString("Run"), canceltext );
    break;
    case CloseOnly:
	    setOkCancelText(
		mBody->finalized() ? canceltext : uiString::emptyString(),
		canceltext );
    break;
    }

    ctrlstyle_ = cs;
}


int uiDialog::go()
{
    return mBody->exec( false );
}


int uiDialog::goMinimized()
{
    return mBody->exec( true );
}


const uiDialog::Setup& uiDialog::setup() const	{ return mBody->getSetup(); }
void uiDialog::reject( CallBacker* cb)		{ mBody->reject( cb ); }
void uiDialog::accept( CallBacker*cb)		{ mBody->accept( cb ); }
void uiDialog::done( DoneResult res )		{ mBody->done( res ); }
void uiDialog::setHSpacing( int s )		{ mBody->setHSpacing(s); }
void uiDialog::setVSpacing( int s )		{ mBody->setVSpacing(s); }
void uiDialog::setBorder( int b )		{ mBody->setBorder(b); }
void uiDialog::setTitleText(const uiString& txt){ mBody->setTitleText(txt); }
void uiDialog::setOkText( const uiString& txt ) { mBody->setOkText(txt); }
void uiDialog::setCancelText(const uiString& t) { mBody->setCancelText(t);}
void uiDialog::enableSaveButton(const uiString& t){ mBody->enableSaveButton(t);}
uiButton* uiDialog::button(Button b)		{ return mBody->button(b); }
const uiButton* uiDialog::button(Button b) const { return mBody->button(b); }
void uiDialog::setSeparator( bool yn )		{ mBody->setSeparator(yn); }
bool uiDialog::separator() const		{ return mBody->separator(); }
void uiDialog::setHelpKey( const HelpKey& key ) { mBody->setHelpKey(key); }
HelpKey uiDialog::helpKey() const		{ return mBody->helpKey(); }
void uiDialog::setVideoKey( const HelpKey& key ) { mBody->setVideoKey(key); }
void uiDialog::setVideoKey( const HelpKey& key, int idx )
						{ mBody->setVideoKey(key,idx); }
HelpKey uiDialog::videoKey(int idx) const	{ return mBody->videoKey(idx); }
int uiDialog::nrVideos() const			{ return mBody->nrVideos(); }
void uiDialog::removeVideo( int idx )		{ mBody->removeVideo(idx); }
int uiDialog::uiResult() const			{ return mBody->uiResult(); }
void uiDialog::setModal( bool yn )		{ mBody->setModal( yn ); }
bool uiDialog::isModal() const			{ return mBody->isModal(); }

void uiDialog::setOkCancelText( const uiString& oktxt, const uiString& cncltxt )
{ mBody->setOkCancelText( oktxt, cncltxt ); }

void uiDialog::setButtonSensitive( Button b, bool s )
    { mBody->setButtonSensitive(b,s); }
void uiDialog::setSaveButtonChecked(bool b)
    { mBody->setSaveButtonChecked(b); }
bool uiDialog::isButtonSensitive( Button b ) const
    { return mBody->isButtonSensitive(b); }
bool uiDialog::saveButtonChecked() const
    { return mBody->saveButtonChecked(); }
bool uiDialog::hasSaveButton() const
    { return mBody->hasSaveButton(); }

uiDialog::TitlePos uiDialog::titlepos_ = CenterWin;
uiDialog::TitlePos uiDialog::titlePos()			{ return titlepos_; }
void uiDialog::setTitlePos( TitlePos p )		{ titlepos_ = p; }

uiGroup* uiDialog::getDlgGroup()
{
    return mBody->getDlgGrp();
}
