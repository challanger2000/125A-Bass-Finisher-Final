#pragma once

#include "vstgui/lib/cview.h"

namespace HighGainGuitarFinisher {

class SteelPanelView final :
    public VSTGUI::CView {
public:
    explicit SteelPanelView(
        const VSTGUI::CRect& size);

    SteelPanelView(
        const SteelPanelView& other);

    void draw(
        VSTGUI::CDrawContext* context) override;
};


class DemoBadgeView final :
    public VSTGUI::CView {
public:
    explicit DemoBadgeView(
        const VSTGUI::CRect& size);

    DemoBadgeView(
        const DemoBadgeView& other);

    VSTGUI::CBaseObject* newCopy() const override {
        return new DemoBadgeView(*this);
    }

    void draw(
        VSTGUI::CDrawContext* context) override;

private:
    bool demo_ {false};
};

} // namespace HighGainGuitarFinisher
