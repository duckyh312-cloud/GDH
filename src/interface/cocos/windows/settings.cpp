#ifdef GEODE_IS_ANDROID
#include <Geode/Geode.hpp>
#include "../hacks_tab.hpp"
#include "../../../core/gui.hpp"
#include "../../../core/config.hpp"
#include "../overlay_button.hpp"

$execute {
    auto& gui = GDH::Gui::get();
    auto& config = Config::get();
    auto& window = gui.getWindow("Settings");

    window.setCustomWindowCocos([&config, &gui](cocos2d::CCNode* node) {
        auto tab = static_cast<HacksTab*>(node);
        tab->addConfigToggle("Hide icon on game", "ui_icon.hide_on_game", true);
        tab->addConfigFloatInput("Icon Size (0.5 - 3)", "ui_icon.scale", 0.5f, 0.25f, 3.f, [](float val) {
            OverlayButton::get()->setSizeScale(val);
        });
        tab->addConfigFloatInput("Min Opacity (0.25 - 1.f)", "ui_icon.minOpacity", 0.8f);
    });
}
#endif