#ifndef uiprogressbar_H
#define uiprogressbar_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiprogressbar.h,v 1.3 2001-05-16 14:58:47 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
class QProgressBar;
template <class T> class i_QObjWrapper;
mTemplTypeDefT(i_QObjWrapper,QProgressBar,i_QProgressBar)

class uiProgressBar : public uiWrapObj<i_QProgressBar>
{
public:

                        uiProgressBar( uiObject*,const char* nm="ProgressBar", 
				       int totalSteps=100, int progress=0);

    virtual bool        isSingleLine() const { return true; }

    void		setProgress( int );
    int			Progress();
    void		setTotalSteps( int );
    int			totalSteps();

protected:

    const QWidget*	qWidget_() const;
};

#endif
