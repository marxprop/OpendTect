#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "position.h"
#include "prestackgather.h"
#include "datapack.h"

namespace FlatView { class Viewer; };
namespace PreStack { class Gather; }
template <class T> class SamplingData;


//!Gather display

namespace PreStackView
{

mExpClass(uiPreStackProcessing) Viewer2DGatherPainter
{
public:
				Viewer2DGatherPainter(FlatView::Viewer&);
				~Viewer2DGatherPainter();

    mDeprecated("No longer used")
    void			setVDGather(DataPackID);
    mDeprecated("No longer used")
    void			setWVAGather(DataPackID);
    void			setVDGather(const PreStack::Gather*);
    void			setWVAGather(const PreStack::Gather*);
    void			setColorTableClipRate(float);
    float			colorTableClipRate() const;

    TrcKey			getTrcKey() const;
    bool			hasData() const
				{ return inputwvagather_ || inputvdgather_; }

protected:
    void			ensureHasZDomain(const PreStack::Gather&);
    void			getGatherRange(const PreStack::Gather*,
					       SamplingData<float>&,
					       int& nrsamp) const;

    FlatView::Viewer&			viewer_;
    ConstRefMan<PreStack::Gather>	inputwvagather_;
    ConstRefMan<PreStack::Gather>	inputvdgather_;
};

} // namespace PreStackView
