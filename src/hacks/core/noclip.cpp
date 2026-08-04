#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../../core/utils.hpp"
#include "../../interface/imgui/widget_helper.hpp"
#include "../../interface/cocos/hack_settings_popup.hpp"
#include "imgui.h"

GUI_HACK_CREATE("Core", "Noclip", "The player will be invincible to obstacles", true);

class $modify(NoclipPlayLayer, PlayLayer) {
    struct Fields {
        CCLayerColor* tint;
    };

    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Core").findHackByName("Noclip");        
        
        hack.addHookPtr(self.getHook("PlayLayer::destroyPlayer").unwrap());

        hack.setCustomWindowImGui([
            tintOnDeathKey = hack.formatAdditionalSetting("tint_on_death"),
            opacityKey = hack.formatAdditionalSetting("tint_opacity"),
            fadeKey = hack.formatAdditionalSetting("tint_fade"),
            colorKey = hack.formatAdditionalSetting("tint_color"),
            p1 = hack.formatAdditionalSetting("p1"),
            p2 = hack.formatAdditionalSetting("p2")
        ]{
            ImGuiWidgetConfig::Checkbox("Tint On Death", tintOnDeathKey, false);
            ImGuiWidgetConfig::DragInt("##Tint Opacity", opacityKey, 1.f, 0, 255, 100, "Tint Opacity: %d");
            ImGuiWidgetConfig::DragFloat("##Tint Fade", fadeKey, 0.01f, 0, 1.f, 0.35f, "Tint Fade: %.2f");
            ImGuiWidgetConfig::ColorEdit3Hex("Tint Color", colorKey, "FF00000");
            ImGui::Separator();
            ImGuiWidgetConfig::Checkbox("Player 1", p1, true);
            ImGuiWidgetConfig::Checkbox("Player 2", p2, true);
        });
        
        hack.setCustomWindowCocos([
            tintOnDeathKey = hack.formatAdditionalSetting("tint_on_death"),
            opacityKey = hack.formatAdditionalSetting("tint_opacity"),
            fadeKey = hack.formatAdditionalSetting("tint_fade"),
            colorKey = hack.formatAdditionalSetting("tint_color"),
            p1 = hack.formatAdditionalSetting("p1"),
            p2 = hack.formatAdditionalSetting("p2")
        ](cocos2d::CCNode* popupNode) {
            auto* popup = static_cast<HackSettingsPopup*>(popupNode);
            popup->addConfigToggle("Tint On Death", tintOnDeathKey, false);
            popup->addConfigColor3Hex("Tint Color", colorKey, "FF00000");
            popup->addConfigIntInput("Opacity", opacityKey, 0, 255, 100);
            popup->addConfigFloatInput("Fade Speed", fadeKey, 0.f, 1.f, 0.35f);
            popup->addSeparator();
            popup->addConfigToggle("Player 1", p1, true);
            popup->addConfigToggle("Player 2", p2, true);
        });
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        auto fields = m_fields.self();
        auto& config = Config::get();

        fields->tint = CCLayerColor::create();
        fields->tint->setZOrder(1000);
        fields->tint->setColor(
            GDH::Utils::hexToColor(
                config.get<std::string>("core.noclip::tint_color", "FF00000")
            )
        );
        m_uiLayer->addChild(fields->tint);

        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        if (m_anticheatSpike && obj == m_anticheatSpike)
            return PlayLayer::destroyPlayer(player, obj);

        auto& config = Config::get();

        bool shouldDestroy = config.get<bool>("core.noclip", false) ? ((player == m_player1 && !config.get<bool>("core.noclip::p1", true)) || 
            (player == m_player2 && !config.get<bool>("core.noclip::p2", true))) : true;
            
        if (shouldDestroy) PlayLayer::destroyPlayer(player, obj);

        if (config.get<bool>("core.noclip::tint_on_death", false)) {
            auto fields = m_fields.self();
            fields->tint->stopAllActions();
            fields->tint->setOpacity(config.get<int>("core.noclip::tint_opacity", 100));
            fields->tint->runAction(cocos2d::CCFadeTo::create(config.get<float>("core.noclip::tint_fade", 0.35f), 0));
        }
    }
};