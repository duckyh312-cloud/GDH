#include <Geode/Geode.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../../core/replayEngine.hpp"
#include "../../interface/imgui/widget_helper.hpp"
#include <imgui-cocos.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../../interface/cocos/hack_settings_popup.hpp"

GUI_HACK_CREATE("Level", "Dual Clicks", "", true);

class $modify(DualClicksGJBaseGameLayer, GJBaseGameLayer) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Dual Clicks");       
        
        (void) self.setHookPriority("GJBaseGameLayer::handleButton", geode::Priority::Early); 
        hack.addHookPtr(self.getHook("GJBaseGameLayer::handleButton").unwrap());

        hack.setCustomWindowImGui([] {
            ImGuiWidgetConfig::Checkbox("Invert click for the opposite player", "level.dual_clicks::invert_click", false);
        });

        hack.setCustomWindowCocos([](cocos2d::CCNode* popupNode) {
            auto* popup = static_cast<HackSettingsPopup*>(popupNode);
            popup->addConfigToggle("Invert click for the opposite player", "level.dual_clicks::invert_click", false);
        });
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        
        if (GDH::ReplayEngine::get().mode == state::play) return;
        
        if (m_level->m_twoPlayerMode && m_gameState.m_isDualMode) {
            auto& config = Config::get();
            bool invert = config.get<bool>("level.dual_clicks::invert_click", false);
            GJBaseGameLayer::handleButton(invert ? !down : down, button, !isPlayer1);
        }
    }
};