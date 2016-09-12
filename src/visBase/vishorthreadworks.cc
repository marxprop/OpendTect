 /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/


#include "vishorthreadworks.h"
#include "vishorizonsection.h"
#include "vishorizonsectiontile.h"
#include "vishorizonsectiondef.h"

#include "simpnumer.h"
#include "zaxistransform.h"
#include "binidsurface.h"
#include "position.h"


using namespace visBase;


HorizonTileRenderPreparer::HorizonTileRenderPreparer(
    HorizonSection& hrsection, const osg::CullStack* cs, char res )
    : hrsection_( hrsection )
    , tkzs_( cs )
    , hrsectiontiles_( hrsection.tiles_.getData() )
    , nrtiles_( hrsection.tiles_.info().getTotalSz() )
    , nrcoltiles_( hrsection.tiles_.info().getSize(1) )
    , resolution_( res )
    , permutation_( 0 )
{

}


bool HorizonTileRenderPreparer:: doPrepare( int nrthreads )
{
    barrier_.setNrThreads( nrthreads );
    nrthreadsfinishedwithres_ = 0;

    delete [] permutation_;
    permutation_ = 0;
    mTryAlloc( permutation_, od_int64[nrtiles_] );

    for ( int idx=0; idx<nrtiles_; idx++ )
	permutation_[idx] = idx;

    std::random_shuffle( permutation_, permutation_+nrtiles_ );

    return true;
}


bool HorizonTileRenderPreparer::doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( hrsectiontiles_[realidx] )
	    hrsectiontiles_[realidx]->updateAutoResolution( tkzs_ );
    }

    barrier_.waitForAll();

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	HorizonSectionTile* tile = hrsectiontiles_[realidx];
	if ( tile )
	{
	    char res = tile->getActualResolution();
	    if ( res==cNoneResolution )
		res = resolution_;
	    else
		tile->tesselateResolution( res, true );
	}

	addToNrDone( 1 );
    }

    barrier_.waitForAll();
    if ( !shouldContinue() )
	return false;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( hrsectiontiles_[realidx] )
	    hrsectiontiles_[realidx]->ensureGlueTesselated();

	addToNrDone( 1 );
    }

    return true;
}

HorizonTileResolutionTesselator::HorizonTileResolutionTesselator(
    const HorizonSection* hrsection, char res )
    : horsection_( hrsection )
    , nrtiles_( hrsection->tiles_.info().getTotalSz() )
    , resolution_( res )
{
    setName( "Horizon resolution tessellation" );
}


HorizonTileResolutionTesselator::~HorizonTileResolutionTesselator()
{
    for ( int idx=0; idx<hrtiles_.size(); idx++ )
    {
	if ( hrtiles_[idx] )
	    delete hrtiles_[idx];
    }
    hrtiles_.setEmpty();
}

bool HorizonTileResolutionTesselator:: doPrepare( int nrthreads )
{
    return createTiles();
}


bool HorizonTileResolutionTesselator::doWork( 
    od_int64 start, od_int64 stop, int )
{
    mDefineRCRange( horsection_,-> );

    if ( !horsection_->geometry_ )
	return false;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	if ( !hrtiles_[idx] )
	     continue;

	const RowCol& origin = hrtiles_[idx]->origin_;
	TypeSet<Coord3> positions;
	const int nrsidecoords = horsection_->nrcoordspertileside_;
	positions.setCapacity( nrsidecoords*nrsidecoords, false );
	for ( int rowidx=0; rowidx<nrsidecoords ; rowidx++ )
	{
	    const int row = origin.row() + rowidx*rrg.step;
	    const bool rowok = rrg.includes(row, false);
	    const StepInterval<int> geocolrg = 
		horsection_->geometry_->colRange( row );
	    const StepInterval<int> colrg(
		mMAX(geocolrg.start,crg.start),
		mMIN(geocolrg.stop,crg.stop), crg.step );

	    for ( int colidx=0; colidx<nrsidecoords; colidx++ )
	    {
		const int col = origin.col() + colidx*colrg.step;
		Coord3 pos = rowok && colrg.includes(col, false)
		    ? horsection_->geometry_->getKnot(RowCol(row,col),false)
		    : Coord3::udf();
		if ( horsection_->zaxistransform_ )
		    pos.z = horsection_->zaxistransform_->transform( pos );
		positions += pos;
	    }
	}

	hrtiles_[idx]->setPositions( positions );
	hrtiles_[idx]->tesselateResolution( resolution_, false );
	addToNrDone( 1 );
    }

    return true;
}


bool HorizonTileResolutionTesselator::createTiles()
{
    mDefineRCRange( horsection_,-> );

    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return false;

    RowCol origin = RowCol( rrg.start, crg.start );
    const int nrrows = nrBlocks( rrg.nrSteps()+1,
				 horsection_->nrcoordspertileside_, 1 );
    const int nrcols = nrBlocks( crg.nrSteps()+1,
				 horsection_->nrcoordspertileside_, 1 );

    for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
    {
	for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	{
	    const RowCol step(rrg.step,crg.step);
	    const RowCol tileorigin(horsection_->origin_.row() +
		tilerowidx*horsection_->tilesidesize_*step.row(),
		horsection_->origin_.col() +
		tilecolidx*horsection_->tilesidesize_*step.col() );

	    HorizonSectionTile* tile = 
		new HorizonSectionTile( *horsection_, tileorigin );
	    tile->setResolution( resolution_ );
	    hrtiles_ += tile;
	}
    }
    
    return true;
}


bool HorizonTileResolutionTesselator::getTitleCoordinates( int idx, 
    TypeSet<Coord3>& coords ) const
{
    if ( idx>=0 && idx<hrtiles_.size() )
	return hrtiles_[idx]->getResolutionCoordinates( coords );

    return false;
}


bool HorizonTileResolutionTesselator::getTitleNormals( 
    int idx, TypeSet<Coord3>& normals ) const
{
    if ( idx>=0 && idx<hrtiles_.size() )
	return hrtiles_[idx]->getResolutionNormals( normals );

    return false;
}


bool HorizonTileResolutionTesselator::getTitlePrimitiveSet( int idx, 
    TypeSet<int>& ps, GeometryType type ) const
{
    if ( idx>=0 && idx<hrtiles_.size() )
	return hrtiles_[idx]->getResolutionPrimitiveSet(resolution_,ps,type);

    return false;
}


TileTesselator::TileTesselator( HorizonSectionTile* tile, char res )
    : tile_( tile ), res_( res )
{}


int TileTesselator::nextStep()
{
    if ( tile_ )
	tile_->tesselateResolution( res_, false );

    return SequentialTask::Finished();
}


TileGlueTesselator::TileGlueTesselator( HorizonSectionTile* tile )
    : tile_( tile )
{}


int TileGlueTesselator::nextStep()
{
    if ( tile_ )
	tile_->ensureGlueTesselated();
    return SequentialTask::Finished();
}



HorizonSectionTilePosSetup::HorizonSectionTilePosSetup(
    ObjectSet<HorizonSectionTile> tiles, HorizonSection* horsection,
    StepInterval<int>rrg,StepInterval<int>crg )
    : hrtiles_( tiles )
    , rrg_( rrg )
    , crg_( crg )
    , geo_( 0 )
    , horsection_( horsection )
{

    if ( horsection_ )
    {
	zaxistransform_ = horsection_->getZAxisTransform();
	nrcrdspertileside_ = horsection_->nrcoordspertileside_;
	lowestresidx_ = horsection_->lowestresidx_;
	geo_ = horsection_->geometry_;
    }

    if ( zaxistransform_ ) zaxistransform_->ref();
    setName( BufferString( "Creating horizon surface..." ) );
}


HorizonSectionTilePosSetup::~HorizonSectionTilePosSetup()
{
    if ( zaxistransform_ ) zaxistransform_->unRef();

}


bool HorizonSectionTilePosSetup::doWork( od_int64 start, od_int64 stop,
					 int threadid )
{
    if ( !geo_ )
	return false;

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	if ( !hrtiles_[idx] )
	     continue;

	const RowCol& origin = hrtiles_[idx]->origin_;
	TypeSet<Coord3> positions;
	positions.setCapacity( (nrcrdspertileside_)*(nrcrdspertileside_),
			       false );
	for ( int rowidx=0; rowidx<nrcrdspertileside_ ; rowidx++ )
	{
	    const int row = origin.row() + rowidx*rrg_.step;
	    const bool rowok = rrg_.includes(row, false);
	    const StepInterval<int> geocolrg = geo_->colRange( row );
	    const StepInterval<int> colrg(
		mMAX(geocolrg.start,crg_.start),
		mMIN(geocolrg.stop,crg_.stop), crg_.step );

	    for ( int colidx=0; colidx<nrcrdspertileside_ ; colidx++ )
	    {
		const int col = origin.col() + colidx*colrg.step;
		Coord3 pos = rowok && colrg.includes(col, false)
		    ? geo_->getKnot(RowCol(row,col),false)
		    : Coord3::udf();
		if ( zaxistransform_ )
		    pos.z = zaxistransform_->transform( pos );
		positions += pos;
	    }
	}

	hrtiles_[idx]->setPositions( positions );
	hrtiles_[idx]->tesselateResolution( lowestresidx_, false );
	addToNrDone( 1 );
    }

    return true;
}


bool HorizonSectionTilePosSetup::doFinish( bool sucess )
{
    if ( sucess && horsection_ )
    {
	horsection_->forceupdate_ =  true;
    }

    return sucess;
}

