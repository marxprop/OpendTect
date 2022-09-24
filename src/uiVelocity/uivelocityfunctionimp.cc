/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivelocityfunctionimp.h"

#include "file.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"
#include "uicombobox.h"
#include "uitaskrunner.h"

#include "binidvalset.h"
#include "ctxtioobj.h"
#include "oddirs.h"
#include "picksettr.h"
#include "od_istream.h"
#include "tabledef.h"
#include "velocityfunctionascio.h"
#include "velocityfunctionstored.h"
#include "uiveldesc.h"
#include "od_helpids.h"

namespace Vel
{

uiImportVelFunc::uiImportVelFunc( uiParent* p )
    : uiDialog( p,uiDialog::Setup(tr("Import Velocity Function"),
				  mNoDlgTitle,
				  mODHelpKey(mImportVelFuncHelpID) )
			    .modal(false))
    , fd_( *FunctionAscIO::getDesc() )
{
    setVideoKey( mODVideoKey(mImportVelFuncHelpID) );
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );

    uiVelocityDesc::Setup su;
    su.desc_.type_ = VelocityDesc::Interval;
    typefld_ = new uiVelocityDesc( this, &su );
    typefld_->attach( alignedBelow, inpfld_ );
    typefld_->typeChangeNotifier().notify(
	    mCB(this,uiImportVelFunc,velTypeChangeCB) );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, typefld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
					mODHelpKey(mImportVelFuncParsHelpID) );
    dataselfld_->attach( alignedBelow, typefld_ );
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( alignedBelow, dataselfld_ );

    IOObjContext ctxt = StoredFunctionSource::ioContext();
    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			      uiStrings::phrOutput( uiStrings::sVelocity() ) );
    outfld_->attach( alignedBelow, dataselfld_ );
    outfld_->attach( ensureBelow, sep );

    velTypeChangeCB( 0 );
}


uiImportVelFunc::~uiImportVelFunc()
{
    delete &fd_;
}


#define mVel 2

void uiImportVelFunc::velTypeChangeCB( CallBacker* )
{
    VelocityDesc desc;
    typefld_->get( desc, false );
    fd_.bodyinfos_[mVel]->setName( VelocityDesc::getTypeString(desc.type_) );
}


void uiImportVelFunc::formatSel( CallBacker* )
{
    FunctionAscIO::updateDesc( fd_ );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiImportVelFunc::acceptOK( CallBacker* )
{
    BinIDValueSet bidvalset( 0, true);
    VelocityDesc desc;
    if ( !typefld_->get( desc, true ) )
	return false;

    if ( !*inpfld_->fileName() )
	mErrRet( uiStrings::phrSelect(uiStrings::phrInput(uiStrings::sFile())) )

    od_istream strm( inpfld_->fileName() );
    if ( !strm.isOK() )
	 mErrRet( uiStrings::sCantOpenInpFile() );

    const od_int64 filesize = File::getKbSize( inpfld_->fileName() );
    FunctionAscIO velascio( fd_, strm, filesize ? filesize : -1 );

    velascio.setOutput( bidvalset );
    bool success;
    if ( filesize>2 )
    {
	uiTaskRunner taskrunner( this );
	success = TaskRunner::execute( &taskrunner, velascio );
    }
    else
	success = velascio.execute();


    if ( !success )
	mErrRet( uiStrings::phrCannotRead( toUiString(inpfld_->fileName())) );

    const IOObj* ioobj = outfld_->ioobj();
    if ( !ioobj )
	mErrRet( outfld_->isEmpty() ? uiStrings::phrSelect(uiStrings::sOutput())
				    : uiStrings::sEmptyString() )

    RefMan<StoredFunctionSource> functions = new StoredFunctionSource;
    functions->setData( bidvalset, desc, true ); //set ZisT
    if ( !functions->store( ioobj->key() ) )
	mErrRet( tr("Cannot store velocity functions") );

    uiString msg = tr("Velocity Function successfully imported."
		      "\n\nDo you want to import more Velocity Functions?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

} // namespace Vel
