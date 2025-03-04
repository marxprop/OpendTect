/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiamplspectrum.h"

#include "uiaxishandlerbase.h"
#include "uibutton.h"
#include "uifiledlg.h"
#include "uifuncdispbase.h"
#include "uifunctiondisplayserver.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uispinbox.h"

#include "arrayndimpl.h"
#include "arrayndwrapper.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "filepath.h"
#include "flatposdata.h"
#include "fourier.h"
#include "od_ostream.h"
#include "oddirs.h"


uiAmplSpectrum::uiAmplSpectrum( uiParent* p, const uiAmplSpectrum::Setup& setup)
    : uiMainWin( p, tr("Amplitude Spectrum"), 0, false, false )
    , setup_(setup)
{
    if ( !setup_.caption_.isEmpty() )
	setCaption( setup_.caption_ );

    uiFuncDispBase::Setup su;
    su.fillbelow(false).canvaswidth(300).canvasheight(300);
    su.noy2axis(true).noy2gridline(true).curvzvaly(8);
    disp_ = GetFunctionDisplayServer().createFunctionDisplay( this, su );
    uiString rangestr = SI().zIsTime()	? !setup_.iscepstrum_ ?
					    tr("Frequency (Hz)") :
					    tr("Quefrency (sec)")
					: !setup_.iscepstrum_ ?
					    SI().zInMeter() ?
						tr("Wavenumber (cycles/km)") :
						tr("Wavenumber (cycles/kft)")
					    : SI().zInMeter() ?
						tr("Navewumber (km)") :
						tr("Navewumber (kft)");
    disp_->xAxis()->setCaption( rangestr );
    disp_->yAxis(false)->setCaption( tr("Power (dB)") );
    mAttachCB( disp_->mouseMoveNotifier(), uiAmplSpectrum::valChgd );

    dispparamgrp_ = new uiGroup( this, "Display Params Group" );
    dispparamgrp_->attach( alignedBelow, disp_->uiobj() );
    uiString disptitle = tr("%1 range").arg(rangestr);
    rangefld_ = new uiGenInput( dispparamgrp_, disptitle, FloatInpIntervalSpec()
			.setName(BufferString("range start"),0)
			.setName(BufferString("range stop"),1) );
    mAttachCB( rangefld_->valueChanged, uiAmplSpectrum::dispRangeChgd );
    stepfld_ = new uiLabeledSpinBox( dispparamgrp_, tr("Gridline step"));
    stepfld_->box()->setNrDecimals( 0 );
    stepfld_->attach( rightOf, rangefld_ );
    mAttachCB( stepfld_->box()->valueChanging, uiAmplSpectrum::dispRangeChgd );

    uiString lbl =  SI().zIsTime() ?
		    uiStrings::phrJoinStrings(uiStrings::sValue(),
		    tr("(%1, power)").arg(uiStrings::sFrequency(true))) :
		    uiStrings::phrJoinStrings(uiStrings::sValue(),
		    tr("(%1, power)").arg(uiStrings::sWaveNumber(true)));
    valfld_ = new uiGenInput( dispparamgrp_, lbl, FloatInpIntervalSpec() );
    valfld_->attach( alignedBelow, rangefld_ );
    valfld_->setNrDecimals( 2 );

    normfld_ = new uiCheckBox( dispparamgrp_, tr("Normalize") );
    normfld_->attach( rightOf, valfld_ );
    normfld_->setChecked( true );

    powerdbfld_ = new uiCheckBox( dispparamgrp_, tr("dB scale") );
    powerdbfld_->attach( rightOf, normfld_ );
    powerdbfld_->setChecked( true );

    mAttachCB( normfld_->activated, uiAmplSpectrum::putDispData );
    mAttachCB( powerdbfld_->activated, uiAmplSpectrum::putDispData );

    exportfld_ = new uiPushButton( this, uiStrings::sExport(), false );
    mAttachCB( exportfld_->activated, uiAmplSpectrum::exportCB );
    exportfld_->attach( rightAlignedBelow, disp_->uiobj() );
    exportfld_->attach( ensureBelow, dispparamgrp_ );

    if ( !setup_.iscepstrum_ )
    {
	auto* cepbut = new uiPushButton( this, tr("Display cepstrum"), false );
	mAttachCB(cepbut->activated, uiAmplSpectrum::ceptrumCB);
	cepbut->attach( leftOf, exportfld_ );
    }
}


uiAmplSpectrum::~uiAmplSpectrum()
{
    detachAllNotifiers();
    delete data_;
    delete timedomain_;
    delete freqdomain_;
    delete freqdomainsum_;
    delete specvals_;
    delete fft_;
}


bool uiAmplSpectrum::setDataPackID( const DataPackID& dpid,
				    const DataPackMgr::MgrID& dmid,
				    int version )
{
    ConstRefMan<DataPack> dp = DPM( dmid ).get<DataPack>( dpid );
    return dp ? setDataPack( *dp.ptr(), version ) : false;
}


bool uiAmplSpectrum::setDataPack( const DataPack& dp, int version )
{
    BufferString dpversionnm;

    mDynamicCastGet(const VolumeDataPack*,voldp,&dp)
    mDynamicCastGet(const FlatDataPack*,fdp,&dp)
    if ( voldp )
    {
	if ( voldp->isEmpty() )
	    return false;

	dpversionnm = voldp->getComponentName( version );
	setup_.nyqvistspspace_ = voldp->zRange().step_;
	setData( voldp->data(version) );
    }
    else if ( fdp )
    {
	dpversionnm = fdp->name();
	mDynamicCastGet(const MapDataPack*,mdp,fdp);
	const Array2D<float>& data = mdp ? mdp->rawData() : fdp->data();
	const FlatPosData& posdata = mdp ? mdp->rawPosData() : fdp->posData();
	setup_.nyqvistspspace_ = (float) (posdata.range(false).step_);
	setData( data );
    }
    else
	return false;

    setCaption( tr( "Amplitude Spectrum for %1" ).arg( dpversionnm ) );

    return true;
}


void uiAmplSpectrum::setData( const Array2D<float>& arr2d )
{
    Array3DWrapper<float> arr3d( const_cast<Array2D<float>&>(arr2d) );
    arr3d.setDimMap( 0, 0 );
    arr3d.setDimMap( 1, 2 );
    arr3d.init();
    setData( arr3d );
}


void uiAmplSpectrum::setData( const Array1D<float>& arr1d )
{
    Array3DWrapper<float> arr3d( const_cast<Array1D<float>&>(arr1d) );
    arr3d.setDimMap( 0, 2 );
    arr3d.init();
    setData( arr3d );
}


void uiAmplSpectrum::setData( const float* array, int size )
{
    Array1DImpl<float> arr1d( size );
    OD::memCopy( arr1d.getData(), array, sizeof(float)*size );
    setData( arr1d );
}


void uiAmplSpectrum::setData( const Array3D<float>& array )
{
    auto* data = new Array3DImpl<float>( array.info() );
    data->copyFrom( array );
    data_ = data;

    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    const int sz2 = array.info().getSize( 2 );

    nrtrcs_ = sz0 * sz1;
    initFFT( sz2 );
    if ( !fft_ )
    {
	uiMSG().error( tr("Cannot initialize FFT") );
	return;
    }

    compute( array );
    putDispData(0);
}


void uiAmplSpectrum::initFFT( int nrsamples )
{
    fft_ = Fourier::CC::createDefault();
    if ( !fft_ ) return;

    const int fftsz = fft_->getFastSize( nrsamples );
    fft_->setInputInfo( Array1DInfoImpl(fftsz) );
    fft_->setDir( true );

    timedomain_ = new Array1DImpl<float_complex>( fftsz );
    freqdomain_ = new Array1DImpl<float_complex>( fftsz );
    freqdomainsum_ = new Array1DImpl<float>( fftsz );

    for ( int idx=0; idx<fftsz; idx++)
    {
	timedomain_->set( idx, 0 );
	freqdomainsum_->set( idx,0 );
    }
}


bool uiAmplSpectrum::compute( const Array3D<float>& array )
{
    if ( !timedomain_ || !freqdomain_ ) return false;

    const int fftsz = timedomain_->info().getSize(0);

    const int sz0 = array.info().getSize( 0 );
    const int sz1 = array.info().getSize( 1 );
    const int sz2 = array.info().getSize( 2 );

    const int start = (fftsz-sz2) / 2;
    for ( int idx=0; idx<sz0; idx++ )
    {
	for ( int idy=0; idy<sz1; idy++ )
	{
	    for ( int idz=0; idz<sz2; idz++ )
	    {
		const float val = array.get( idx, idy, idz );
		timedomain_->set( start+idz, mIsUdf(val) ? 0 : val );
	    }

	    fft_->setInput( timedomain_->getData() );
	    fft_->setOutput( freqdomain_->getData() );
	    fft_->run( true );

	    for ( int idz=0; idz<fftsz; idz++ )
		freqdomainsum_->arr()[idz] += abs(freqdomain_->get(idz));
	}
    }

    const int freqsz = freqdomainsum_->info().getSize(0) / 2;
    delete specvals_;
    specvals_ = new Array1DImpl<float>( freqsz );
    maxspecval_ = 0.f;
    for ( int idx=0; idx<freqsz; idx++ )
    {
	const float val = freqdomainsum_->get( idx )/nrtrcs_;
	specvals_->set( idx, val );
	if ( val > maxspecval_ )
	    maxspecval_ = val;
    }

    return true;
}


void uiAmplSpectrum::putDispData( CallBacker* cb )
{
    const bool isnormalized = normfld_->isChecked();
    const bool dbscale = powerdbfld_->isChecked();
    const int fftsz = freqdomainsum_->info().getSize(0) / 2;
    Array1DImpl<float> dbspecvals( fftsz );
    for ( int idx=0; idx<fftsz; idx++ )
    {
	float val = specvals_->get( idx );
	if ( isnormalized )
	    val /= maxspecval_;

	if ( dbscale )
	    val = 20 * Math::Log10( val > 0. ? val : mDefEpsF );

	dbspecvals.set( idx, val );
    }

    const float freqfact = SI().zIsTime() ?
				    setup_.iscepstrum_ ? 1000.f : 1.f
				    : 1000.f;
    const float maxfreq = fft_->getNyqvist( setup_.nyqvistspspace_ ) * freqfact;
    posrange_.set( 0, maxfreq );
    rangefld_->setValue( posrange_ );
    const float step = (posrange_.stop_-posrange_.start_)/25.f;
    stepfld_->box()->setInterval( posrange_.start_, posrange_.stop_, step );
    stepfld_->box()->setValue( maxfreq/5 );
    disp_->yAxis(false)->setCaption( dbscale ? tr("Power (dB)")
					     : tr("Amplitude") );
    disp_->setVals( posrange_, dbspecvals.arr(), fftsz );
    dispRangeChgd( nullptr );
}


void uiAmplSpectrum::getSpectrumData(Array1DImpl<float>& data, bool normalized)
{
    if ( !specvals_ ) return;
    const int datasz = specvals_->info().getSize(0);
    data.setSize( datasz );
    for ( int idx=0; idx<datasz; idx++ )
	data.set(idx, specvals_->get(idx) / (normalized ? maxspecval_ : 1.0f));
}


void uiAmplSpectrum::dispRangeChgd( CallBacker* )
{
    StepInterval<float> rg = rangefld_->getFInterval();
    rg.step_ = stepfld_->box()->getFValue();
    if ( mIsZero(rg.step_,mDefEpsF) || rg.step_<0 )
	rg.step_ = ( rg.stop_ - rg.start_ )/5;

    const float dstart = rg.start_ - posrange_.start_;
    const float dstop = rg.stop_ - posrange_.stop_;
    const bool startok = mIsZero(dstart,1e-5) || dstart > 0;
    const bool stopok = mIsZero(dstop,1e-5) || dstop < 0;
    if ( !startok || !stopok || rg.isRev() )
    {
	rg.setInterval( posrange_ );
	rg.step_ = ( rg.stop_ - rg.start_ )/5;
	rangefld_->setValue( posrange_ );
    }

    disp_->xAxis()->setRange( rg );
    disp_->draw();
}


void uiAmplSpectrum::exportCB( CallBacker* )
{
    BufferString caption = this->caption().getFullString();
    cleanupString( caption.getCStr(), false, false, true );
    FilePath outfnm( GetExportToDir(), caption );
    outfnm.setExtension( "dat" );
    uiFileDialog dlg( this, false, outfnm.fullPath() );
    if ( !dlg.go() )
	return;

    const BufferString fnm = dlg.fileName();
    od_ostream strm( fnm.buf() );
    if ( strm.isBad() )
    {
	uiMSG().error( uiStrings::phrCannotOpen(uiStrings::phrOutput(
		       uiStrings::phrJoinStrings(uiStrings::sFile(),tr("%1")
		       .arg(fnm)))) );
	return;
    }

    disp_->dump( strm, false );

    if ( strm.isBad() )
    {
	uiMSG().error( uiStrings::phrCannotWrite(tr("values to: %1")
				 .arg(fnm)) );
	return;
    }

    uiMSG().message( tr("Values written to: %1").arg(fnm) );
}


void uiAmplSpectrum::valChgd( CallBacker* cb )
{
    if ( !specvals_ )
	return;

    mCBCapsuleUnpack(const Geom::PointF&,mousepos,cb);
    const Geom::PointF pos = disp_->mapToValue( mousepos );
    const float xpos = pos.x_;
    const float ypos = pos.y_;
    const bool disp = disp_->xAxis()->range().includes(xpos,true) &&
		      disp_->yAxis(false)->range().includes(ypos,true);
    if ( !disp )
	return;

    const float ratio = (xpos-posrange_.start_)/posrange_.width();
    const float specsize = mCast( float, specvals_->info().getSize(0) );
    const int specidx = mNINT32( ratio * specsize );
    const TypeSet<float>& specvals = disp_->yVals();
    if ( !specvals.validIdx(specidx) )
	return;

    const float yval = specvals[specidx];
    valfld_->setValue( xpos, 0 );
    valfld_->setValue( yval, 1 );
    valfld_->setNrDecimals( 2 );
}


void uiAmplSpectrum::ceptrumCB( CallBacker* )
{
    if ( !specvals_ )
	return;

    auto* ampl = new uiAmplSpectrum( this,
		uiAmplSpectrum::Setup(tr("Amplitude Cepstrum"), true, 1  ) );
    ampl->setDeleteOnClose( true );
    ampl->setData( *specvals_ );
    ampl->show();
}
