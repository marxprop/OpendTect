#ifndef uicolor_h
#define uicolor_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id: uicolor.h,v 1.2 2001-05-16 14:58:36 arend Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "uibutton.h"

class uiObject;

bool  	select( Color&, uiParent* parnt=0, const char* nm=0); 
	/*!< \brief pops a selector box to select a new color 
             \return true if new color selected
	*/


class uiColorInput : public uiPushButton
{
//!< \brief This button will have the selected color as background color.
public:

			uiColorInput(uiObject*,const Color&,
				     const char* seltxt="Select color",
				     const char* buttxt="Color ...");

    const Color&	color() const	{ return color_; }

protected:

    Color		color_;
    BufferString	seltxt_;

    void		pushed(CallBacker*);
};


#endif
