/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.cc,v 1.21 2007-01-19 16:16:05 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidockwin.h"
#include "uigroup.h"
#include "uiparentbody.h"

#ifdef USEQT4
# include <q3dockwindow.h>
# define mQDockWindow Q3DockWindow
#else
# include <qdockwindow.h>
# define mQDockWindow QDockWindow
#endif

#if defined( __mac__ ) || defined( __win__ )
# define _redraw_hack_
# include "timer.h"
#endif


class uiDockWinBody : public uiParentBody
		    , public mQDockWindow
{
public:
			uiDockWinBody( uiDockWin& handle, uiParent* parnt,
				       const char* nm );

    virtual		~uiDockWinBody();
    void		construct();

#define mHANDLE_OBJ     uiDockWin
#define mQWIDGET_BASE   mQDockWindow
#define mQWIDGET_BODY   mQDockWindow
#define UIBASEBODY_ONLY
#define UIPARENT_BODY_CENTR_WIDGET
#include                "i_uiobjqtbody.h"

protected:

    virtual void	finalise();

#ifdef _redraw_hack_

// the doc windows are not correctly drawn on the mac at pop-up

    virtual void        resizeEvent( QResizeEvent* ev )
    {
	mQDockWindow::resizeEvent(ev);

	if ( redrtimer.isActive() ) redrtimer.stop();
	redrtimer.start( 300, true );
    }

// not at moves...
    virtual void        moveEvent( QMoveEvent* ev )
    {
	mQDockWindow::moveEvent(ev);

	if ( redrtimer.isActive() ) redrtimer.stop();
	redrtimer.start( 300, true );
    }

    void		redrTimTick( CallBacker* cb )
    {
	//TODO: check this out.  Are scenes deleted or 'just' hidden??
	if ( isHidden() ) return;

	hide();
	show();
    }

    static Timer	redrtimer;
    CallBack		mycallback_;
#endif

};

#ifdef _redraw_hack_
Timer uiDockWinBody::redrtimer;
#endif


uiDockWinBody::uiDockWinBody( uiDockWin& handle__, uiParent* parnt, 
			      const char* nm )
	: uiParentBody( nm )
	, mQDockWindow( InDock, parnt && parnt->pbody() ?  
					parnt->pbody()->qwidget() : 0, nm ) 
	, handle_( handle__ )
	, initing( true )
	, centralWidget_( 0 )
#ifdef _redraw_hack_
	, mycallback_( mCB(this, uiDockWinBody, redrTimTick) )
#endif
{
    if ( *nm ) setCaption( nm );
#ifdef _redraw_hack_
    redrtimer.tick.notify( mycallback_ );
    redrtimer.start( 500, true );
#endif
}

void uiDockWinBody::construct()
{
    centralWidget_ = new uiGroup( &handle(), "uiDockWin central widget" );
    setWidget( centralWidget_->body()->qwidget() ); 

    centralWidget_->setIsMain(true);
    centralWidget_->setBorder(0);
    centralWidget_->setStretch(2,2);

    initing = false;
}


uiDockWinBody::~uiDockWinBody( )
{
#ifdef _redraw_hack_
    redrtimer.tick.remove( mycallback_ );
#endif
    delete centralWidget_; centralWidget_ = 0;
}

void uiDockWinBody::finalise()
    { centralWidget_->finalise();  finaliseChildren(); }


uiDockWin::uiDockWin( uiParent* parnt, const char* nm )
    : uiParent( nm, 0 )
    , body_( 0 )
{ 
    body_= new uiDockWinBody( *this, parnt, nm ); 
    setBody( body_ );
    body_->construct();
}


uiDockWin::~uiDockWin()
{ 
    delete body_; 
}


void uiDockWin::setDockName( const char* nm )
{ body_->qwidget()->setName( nm ); }

const char* uiDockWin::getDockName() const
{ return body_->qwidget()->name(); }


uiGroup* uiDockWin::topGroup()	    	   
{ 
    return body_->uiCentralWidg(); 
}


uiObject* uiDockWin::mainobject()
{ 
    return body_->uiCentralWidg()->mainObject(); 
}

void uiDockWin::setHorStretchable( bool yn )
{ body_->setHorizontallyStretchable( yn ); }

bool uiDockWin::isHorStretchable() const
{ return body_->isHorizontallyStretchable(); }

void uiDockWin::setVerStretchable( bool yn )
{ body_->setVerticallyStretchable( yn ); }

bool uiDockWin::isVerStretchable() const
{ return body_->isVerticallyStretchable(); }

void uiDockWin::setResizeEnabled( bool yn )
{ body_->setResizeEnabled( yn ); }

bool uiDockWin::isResizeEnabled() const
{ return body_->isResizeEnabled(); }

void uiDockWin::setCloseMode( CloseMode mode )
{ body_->setCloseMode( (int)mode ); }

uiDockWin::CloseMode uiDockWin::closeMode() const
{ return (uiDockWin::CloseMode)body_->closeMode(); }

void uiDockWin::dock()		{ body_->dock(); }
void uiDockWin::undock()	{ body_->undock(); }

mQDockWindow* uiDockWin::qwidget()
{ return body_; }
