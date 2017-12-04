#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attribdescid.h"
#include "notify.h"
#include "dbkey.h"
#include "uistring.h"

class BufferStringSet;
class DataPointSet;

namespace Attrib
{

class DescSetup; class SelSpec;

/*!\brief Set of attribute descriptions.  */

mExpClass(AttributeEngine) DescSet : public CallBacker
{ mODTextTranslationClass( Attrib::DescSet )
public:

    explicit		DescSet(bool is2d);
			DescSet(const DescSet&);
			~DescSet();
    DescSet&		operator =(const DescSet&);

    inline bool		isEmpty() const	{ return descs_.isEmpty(); }
    inline int		size() const	{ return descs_.size(); }
    int			indexOf(const char* nm,bool usrref=true) const;
    inline bool		isPresent( const char* nm, bool usrref=true ) const
						{ return indexOf(nm,usrref)>=0;}
    int			indexOf( const DescID& id ) const
						{ return ids_.indexOf( id ); }

    Desc*		desc( int idx )		{ return descs_[idx]; }
    const Desc*		desc( int idx ) const	{ return descs_[idx]; }

    DescSet*		optimizeClone(const DescID& targetid) const;
    DescSet*		optimizeClone(const TypeSet<DescID>&) const;
    DescSet*		optimizeClone(const BufferStringSet&) const;
			/*!< Only clones stuff needed to calculate
			     the attrib with the ids given */
    void		updateInputs();
			/*!< Updates inputs for all descs in descset.
			     Necessary after cloning */

    DescID		addDesc(Desc*,DescID newid=DescID());
    DescID		insertDesc(Desc*,int,DescID newid=DescID());
    void		createAndAddMultOutDescs(const DescID&,
						 const TypeSet<int>&,
						 const BufferStringSet&,
						 TypeSet<DescID>&);
			/*!< Make sure all descs needed to compute
			     attributes with multiple outputs
			     are created and added */

    Desc&		operator[]( int idx )	{ return *desc(idx); }
    const Desc&		operator[]( int idx ) const { return *desc(idx); }
    int			nrDescs(bool inclstored=false,
				bool inclhidden=false) const;

    Desc*		getDesc( const DescID& id )
						{ return gtDesc(id); }
    const Desc*		getDesc( const DescID& id ) const
						{ return gtDesc(id); }
    DescID		getID(const Desc&) const;
    DescID		getID(int) const;
    DescID		getID(const char* ref,bool isusrref,
			      bool mustbestored=false,
			      bool usestorinfo=false) const;
    DescID		getDefaultTargetID() const;
    void		getIds(TypeSet<DescID>&) const;
    void		getStoredIds(TypeSet<DescID>&) const;
    DescID		getStoredID(const DBKey&,int selout=-1,
				    bool add_if_absent=true,
				    bool blindcomp=false,
				    const char* blindcompnm=0) const;
    DescID		defStoredID() const;
    Desc*		getFirstStored(bool usesteering=true) const;
    DBKey		getStoredKey(const DescID&) const;
    void		getStoredNames(BufferStringSet&) const;
    void		getAttribNames(BufferStringSet&,bool inclhidden) const;

    void		removeDesc(const DescID&);
    void		moveDescUpDown(const DescID&,bool);
    void		sortDescSet();
    void		removeAll(bool kpdefault);
    int			removeUnused(bool removestored=false,
				     bool kpdefault=true);
			//!< Removes unused hidden attributes, stored attribs
			//!< if not available or if removestored flag is true.
			//!< Returns total removed.
    bool		isAttribUsed(const DescID&,BufferString&) const;
    bool		isAttribUsed(const DescID&) const;
    void		cleanUpDescsMissingInputs();

    bool		createSteeringDesc(const IOPar&,BufferString,
					   ObjectSet<Desc>&, int& id,
					   uiStringSet* errmsgs=0);
    static Desc*	createDesc(const BufferString&, const IOPar&,
				   const BufferString&,uiStringSet*);
    Desc*		createDesc(const BufferString&, const IOPar&,
				   const BufferString& );
    DescID		createStoredDesc(const DBKey&,int selout,
					 const BufferString& compnm);
    bool		setAllInputDescs(int, const IOPar&,uiStringSet*);
    void		handleStorageOldFormat(IOPar&);
    void		handleOldAttributes(BufferString&,IOPar&,BufferString&,
					    int) const;
    void		handleOldMathExpression(IOPar&,BufferString&,int) const;
    void		handleReferenceInput(Desc*);

			//!<will prepare strings for each desc, format :
			//!<DescID`definition string
    void		fillInAttribColRefs(BufferStringSet&) const;

			//!<will prepare strings for each desc, format :
			//!<Attrib,[stored], [{prestack}]
    void		fillInUIInputList(BufferStringSet&) const;
			//!<Counterpart: will decode the UI string
			//!<and return the corresponding Desc*
    Desc*		getDescFromUIListEntry(const StringPair&);

			//!<will create an empty DataPointSet
    DataPointSet*	createDataPointSet(Attrib::DescSetup,
					   bool withstored=true) const;
    void		fillInSelSpecs(Attrib::DescSetup,
				       TypeSet<Attrib::SelSpec>&) const;

    inline bool		is2D() const		{ return is2d_; }
    bool		hasStoredInMem() const;
    bool		couldBeUsedInAnyDimension() const
			{ return couldbeanydim_; }
    void		setCouldBeUsedInAnyDimension( bool yn )
			{ couldbeanydim_ = yn; }

    bool		exportToDot(const char* nm,const char* fnm) const;

    uiString		errMsg() const;
    static const char*	highestIDStr()		{ return "MaxNrKeys"; }
    static const char*	definitionStr()		{ return "Definition"; }
    static const char*	userRefStr()		{ return "UserRef"; }
    static const char*	inputPrefixStr()	{ return "Input"; }
    static const char*	hiddenStr()		{ return "Hidden"; }
    static const char*	indexStr()		{ return "Index"; }
    static BufferString storedIDErrStr()
				    { return "Parameter 'id' is not correct"; }

    CNotifier<DescSet,DescID>	descAdded;
    CNotifier<DescSet,DescID>	descUserRefChanged;
    CNotifier<DescSet,DescID>	descToBeRemoved;
    CNotifier<DescSet,DescID>	descRemoved;
    Notifier<DescSet>		aboutToBeDeleted;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&,uiStringSet* errmsgs=0);
    bool		useOldSteeringPar(IOPar&,ObjectSet<Desc>&,
					  uiStringSet*);

    static const DescSet& empty2D();
    static const DescSet& empty3D();

protected:

    DescID		getFreeID() const;

    const bool		is2d_;
    ObjectSet<Desc>	descs_;
    TypeSet<DescID>	ids_;
    bool		couldbeanydim_;
    uiString		errmsg_;

    void		usrRefChgCB(CallBacker*);

private:

    Desc*		gtDesc(const DescID&) const;

public:

    DescID		ensureStoredPresent(const DBKey&,int compnr=-1) const;
    DescID		ensureDefStoredPresent() const;
    static uiString	sFactoryEntryNotFound(const char* attrnm);

};

} // namespace Attrib
