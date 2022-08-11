#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Sep 2010
________________________________________________________________________


-*/

#include "stratmod.h"

#include "elasticpropsel.h"
#include "propertyref.h"
#include "manobjectset.h"

class od_istream;
class od_ostream;

namespace Strat
{
class Layer;
class LayerSequence;
class RefTree;

/*!\brief A model consisting of layer sequences.

  The sequences will use the PropertyRefSelection managed by this object.

 */

mExpClass(Strat) LayerModel
{ mIsContainer( LayerModel, ObjectSet<LayerSequence>, seqs_ )
public:

				LayerModel();
				LayerModel( const LayerModel& lm )
							{ *this = lm; }
    virtual			~LayerModel();
    LayerModel&			operator =(const LayerModel&);

    bool			isEmpty() const;
    bool			isValid() const;
    int				size() const	{ return seqs_.size(); }
    LayerSequence&		sequence( int idx )	  { return *seqs_[idx];}
    const LayerSequence&	sequence( int idx ) const { return *seqs_[idx];}
    int				nrLayers() const;
    Interval<float>		zRange() const;

    void			setEmpty();
    LayerSequence&		addSequence();
    LayerSequence&		addSequence(const LayerSequence&);
				//!< Does a match of props
    void			removeSequence(int);

    PropertyRefSelection&	propertyRefs()		{ return proprefs_; }
    const PropertyRefSelection& propertyRefs() const	{ return proprefs_; }
    void			prepareUse() const;

    void			setElasticPropSel(const ElasticPropSelection&);
    const ElasticPropSelection& elasticPropSel() const
				{ return elasticpropsel_; }

    const RefTree&		refTree() const;

    bool			readHeader(od_istream&,PropertyRefSelection&,
					   int& nrseq,bool& mathpreserve);
    bool			read(od_istream&);
    bool			write(od_ostream&,int modnr=0,
					bool mathpreserve=false) const;

    static const char*		sKeyNrSeqs()	{ return "Nr Sequences"; }
    static StringView		defSVelStr()	{ return "DefaultSVel"; }

protected:

    PropertyRefSelection	proprefs_;
    ElasticPropSelection	elasticpropsel_;

};

mDefContainerSwapFunction( Strat, LayerModel )


/*!\brief set of related LayerModels that are edits of an original
  (the first one). Each model has a description. The suite is never empty. */

mExpClass(Strat) LayerModelSuite : public CallBacker
{
public:

			LayerModelSuite();
    virtual		~LayerModelSuite()	{}

    int			size() const		{ return mdls_.size(); }
    LayerModel&		get( int idx )		{ return *mdls_.get(idx); }
    const LayerModel&	get( int idx ) const	{ return *mdls_.get(idx); }
    LayerModel&		baseModel()		{ return *mdls_.first(); }
    const LayerModel&	baseModel() const	{ return *mdls_.first(); }

    BufferString	desc( int idx ) const	{ return descs_.get(idx); }
    uiString		uiDesc( int idx ) const { return uidescs_.get(idx); }
    void		setDesc(int,const char*,const uiString&);

    int			curIdx() const		{ return curidx_; }
    LayerModel&		getCurrent()		{ return get( curidx_ ); }
    const LayerModel&	getCurrent() const	{ return get( curidx_ ); }
    void		setCurIdx(int);

    void		addModel(const char*,const uiString&);
    void		removeModel(int);
    void		setElasticPropSel(const ElasticPropSelection&);

    bool		curIsEdited() const	{ return curidx_ > 0; }
    bool		hasEditedData() const;
    void		clearEditedData();
    void		prepareEditing();
    void		touch(int ctyp=-1)	{ modelChanged.trigger(ctyp); }

    Notifier<LayerModelSuite> curChanged;
    CNotifier<LayerModelSuite,bool> baseChanged;
			//!< passes whether there was base data before change
    CNotifier<LayerModelSuite,bool> editingChanged;
			//!< passes whether there was edited data before change
    CNotifier<LayerModelSuite,int> modelChanged;
			//!< The content of the current model was changed

private:

    ManagedObjectSet<LayerModel> mdls_;
    int			curidx_ = 0;
    BufferStringSet	descs_;
    uiStringSet		uidescs_;

public:

    void		setEmpty();
			//!< keeps empty base model
    void		setBaseModel(LayerModel*,bool setascurrent=false);
			//!< will clear editing

};

} // namespace Strat
