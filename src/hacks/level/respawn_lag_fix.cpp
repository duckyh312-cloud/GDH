#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../../interface/imgui/widget_helper.hpp"

GUI_HACK_CREATE("Level", "Respawn Lag Fix", "Smoothing respawns with a spawn delay", false);

class $modify(RespawnLagFixPauseLayer, PlayLayer) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Respawn Lag Fix");  
        
        hack.setCustomWindowImGui([
            delay = hack.formatAdditionalSetting("delay")
        ]() {
            ImGuiWidgetConfig::DragInt("##RespawnLagFixFrames", delay, 1.f, 2, INT_MAX, 30, "Delay: %d frames");
        });
    }
    
    void resetLevel() {
        PlayLayer::resetLevel();

        auto& config = Config::get();
        m_resumeTimer = config.get<int>("level.respawn_lag_fix::delay", 30);
    }
};