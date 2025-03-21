#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "emposid.h"
#include "uiodvw2dtreeitem.h"

namespace View2D { class Horizon3D; }
class uiODViewer2D;


mExpClass(uiODMain) uiODView2DHor3DParentTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DHor3DParentTreeItem);
public:
				uiODView2DHor3DParentTreeItem();
				~uiODView2DHor3DParentTreeItem();

    bool			showSubMenu() override;
    void			getHor3DVwr2DIDs(const EM::ObjectID&,
						 TypeSet<Vis2DID>&) const;
    void			getLoadedHorizon3Ds(
						 TypeSet<EM::ObjectID>&) const;
    void			removeHorizon3D(const EM::ObjectID&);
    void			addHorizon3Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon3D(const EM::ObjectID&);

private:

    bool			init() override;
    const char*			iconName() const override;
    bool			handleSubMenu(int);
    const char*			parentType() const override
				{ return typeid(uiODView2DTreeTop).name(); }
    void			getNonLoadedTrackedHor3Ds(
						    TypeSet<EM::ObjectID>&);
public:
    void			setupTrackingHorizon3D(const EM::ObjectID&);
};


mExpClass(uiODMain) uiODView2DHor3DTreeItemFactory
    : public uiODView2DTreeItemFactory
{
public:
    const char*		name() const override
			{ return typeid(*this).name(); }

    uiTreeItem*		create() const override
			{ return new uiODView2DHor3DParentTreeItem(); }
    uiTreeItem*		createForVis(const uiODViewer2D&,
				     const Vis2DID&) const override;
};


mExpClass(uiODMain) uiODView2DHor3DTreeItem : public uiODView2DTreeItem
{ mODTextTranslationClass(uiODView2DHor3DTreeItem)
public:
			uiODView2DHor3DTreeItem(const EM::ObjectID&);
			uiODView2DHor3DTreeItem(const Vis2DID&,bool dummy);
			~uiODView2DHor3DTreeItem();

    bool			select() override;
    bool			showSubMenu() override;
    EM::ObjectID		emObjectID() const	{ return emid_; }
    const View2D::Horizon3D*	vw2DObject() const	{ return horview_; }

private:

    bool		init() override;
    const char*		parentType() const override
			{ return typeid(uiODView2DHor3DParentTreeItem).name(); }
    bool		isSelectable() const override		{ return true; }

    void		updateSelSpec(const Attrib::SelSpec*,bool wva) override;
    void		updateCS(const TrcKeyZSampling&,bool upd) override;
    void		checkCB(CallBacker*);
    void		deSelCB(CallBacker*);
    void		emobjAbtToDelCB(CallBacker*);
    void		mousePressInVwrCB(CallBacker*);
    void		mouseReleaseInVwrCB(CallBacker*);
    void		displayMiniCtab();

    void		emobjChangeCB(CallBacker*);
    void		propChgCB(CallBacker*);

    EM::ObjectID	emid_;
    View2D::Horizon3D*	horview_	= nullptr;
    bool		oldactivevolupdated_	= false;
};
