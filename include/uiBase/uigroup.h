#ifndef uigroup_H
#define uigroup_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uigroup.h,v 1.11 2001-09-26 14:47:42 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <uiparent.h>
#include <callback.h>
class IOPar;

class uiGroupBody;
class uiParentBody;

class uiGroup;
class uiGroupObjBody;
class uiGroupParentBody;

class uiGroupObj : public uiObject
{ 	
friend class uiGroup;
protected:
			uiGroupObj( uiParent*, const char* nm, bool manage );
private:

//    uiGroupObjBody*	mkbody(uiParent*,const char*);
    uiGroupObjBody*	body_;
};


class uiGroup : public uiParent
{ 	
friend class		uiGroupObjBody;
public:
			uiGroup( uiParent* , const char* nm="uiGroup", 
			    int border=0, int spacing=10, bool manage=true );

    inline uiGroupObj*	uiObj()				    { return grpobj_; }
    inline const uiGroupObj* uiObj() const		    { return grpobj_; }
    inline		operator const uiGroupObj*() const  { return grpobj_; }
    inline		operator uiGroupObj*() 		    { return grpobj_; }
    inline		operator const uiObject&() const    { return *grpobj_; }
    inline		operator uiObject&() 		    { return *grpobj_; }


    void		setSpacing( int ); 
    void		setBorder( int ); 

    uiObject*		hAlignObj();
    void		setHAlignObj( uiObject* o );
    uiObject*		hCentreObj();
    void		setHCentreObj( uiObject* o );

// uiObject methods, delegated to uiObj():

    inline void		display( bool yn = true )
			    { if ( yn ) show(); else hide(); }
    void		show();
    void		hide(bool shrink=false);
    void		setFocus();

    Color               backgroundColor() const;
    void                setBackgroundColor(const Color&);
    void		setSensitive(bool yn=true);
    bool		sensitive() const;

    int			prefHNrPics() const;
    void                setPrefWidth( int w );
    void                setPrefWidthInChar( float w );
    int			prefVNrPics() const;
    void		setPrefHeight( int h );
    void		setPrefHeightInChar( float h );
    void                setStretch( int hor, int ver );

    void		attach( constraintType, int margin=-1);
    void		attach( constraintType, uiObject *oth, int margin=-1);
    void		attach( constraintType t, uiGroup *o, int margin=-1)
			    { attach(t, o->uiObj(), margin); } 

    void 		setFont( const uiFont& );
    const uiFont*	font() const;

    virtual bool	fillPar( IOPar& ) const		{ return true; }
    virtual void	usePar( const IOPar& )		{}

    uiSize		actualSize( bool include_border = true) const;

    void		setCaption( const char* );

    void		shallowRedraw( CallBacker* =0 )		{reDraw(false);}
    void		deepRedraw( CallBacker* =0 )		{reDraw(true); }
    void		reDraw( bool deep );


//
protected:

    uiGroupObj*		grpobj_;
    uiGroupParentBody*	body_;

    virtual void	reDraw_( bool deep )			{}
};

class NotifierAccess;

class uiGroupCreater
{
public:

    virtual uiGroup*		create(uiParent*,NamedNotifierList* =0)	= 0;

};


#endif
