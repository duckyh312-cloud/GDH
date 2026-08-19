#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../../core/config.hpp"
#include "../../interface/cocos/overlay_button.hpp"

class $modify(OverlayButtonVisiblity, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        if (Config::get().get<bool>("ui_icon.hide_on_game", true))
            OverlayButton::get()->setVisible(false);

        return true;
    }

    void pauseGame(bool unfocused) {
        PlayLayer::pauseGame(unfocused);

        if (Config::get().get<bool>("ui_icon.hide_on_game", true))
            OverlayButton::get()->setVisible(true);
    }
    
    void resume() {
        PlayLayer::resume();
        
        if (Config::get().get<bool>("ui_icon.hide_on_game", true))
            OverlayButton::get()->setVisible(false);
    }
};