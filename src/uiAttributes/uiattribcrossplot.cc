/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2003
 RCS:           $Id: uiattribcrossplot.cc,v 1.10 2008-02-20 12:46:03 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattribcrossplot.h"
#include "uidatapointset.h"
#include "seisioobjinfo.h"
#include "attribsel.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "posprovider.h"
#include "posrandomfilter.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "keystrs.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"

#include "uimsg.h"
#include "uilistbox.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uiposdataedit.h"
#include "uiposprovider.h"

using namespace Attrib;

uiAttribCrossPlot::uiAttribCrossPlot( uiParent* p, const Attrib::DescSet& d )
	: uiDialog(p,uiDialog::Setup("Attribute cross-plotting",
		     "Select attributes and locations for cross-plot"
		     ,"101.3.0"))
	, ads_(d)
{
    uiLabeledListBox* llb = new uiLabeledListBox( this,
	    					  "Attributes to calculate" );
    attrsfld_ = llb->box();
    Attrib::SelInfo attrinf( &ads_, 0, ads_.is2D() );
    for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
    {
	attrsfld_->addItem( attrinf.attrnms.get(idx), false );
	attrdefs_.add( attrinf.attrnms.get(idx) );
    }
    for ( int idx=0; idx<attrinf.ioobjids.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids.get(idx) ) );
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinf.ioobjnms.get(idx).buf();
	    attrsfld_->addItem(
		    SeisIOObjInfo::defKey2DispName(defkey,ioobjnm) );
	    attrdefs_.add( defkey );
	}
    }
    if ( !attrsfld_->isEmpty() )
	attrsfld_->setCurrentItem( int(0) );
    attrsfld_->setMultiSelect( true );

    uiPosProvider::Setup su( "Select locations by", true );
    su.choicetype( uiPosProvider::Setup::All ).withz(true).is2d( ads_.is2D() );
    posprovfld_ = new uiPosProvider( this, su );
    posprovfld_->attach( alignedBelow, llb );
}


uiAttribCrossPlot::~uiAttribCrossPlot()
{
}


#define mDPM DPM(DataPackMgr::PointID)

#define mErrRet(s) \
{ \
    if ( dps ) mDPM.release(dps->id()); \
    if ( s ) uiMSG().error(s); return false; \
}

bool uiAttribCrossPlot::acceptOK( CallBacker* )
{
    DataPointSet* dps = 0;
    PtrMan<Pos::Provider> prov = posprovfld_->createProvider();
    if ( !prov )
	mErrRet("Internal: no Pos::Provider")

    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<attrdefs_.size(); idx++ )
    {
	if ( attrsfld_->isSelected(idx) )
	    dcds += new DataColDef( attrsfld_->textOfItem(idx),
		    		    attrdefs_.get(idx) );
    }
    if ( dcds.isEmpty() )
	mErrRet("Please select at least one attribute to evaluate")

    dps = new DataPointSet( *prov, dcds );
    if ( dps->size() < 1 )
	mErrRet("No positions selected")

    dps->setName( "Attribute cross-plot" );
    mDPM.add( dps );

    Pos::RandomFilter3D filt; filt.passratio_ = 0.1;
    filt.initialize();
    ObjectSet<const Pos::Filter> filters; filters += &filt;
    dps->use( filters, true );

    Attrib::EngineMan aem;
    PtrMan<Executor> ex = aem.getTableExtractor( *dps, ads_ );
    uiExecutor uiex( this, *ex );
    if ( !uiex.go() )
	mErrRet(0)

    uiDataPointSet* dlg = new uiDataPointSet( this, *dps,
			uiDataPointSet::Setup("Attribute data").modal(false) );
    return dlg->go() ? true : false;
}
