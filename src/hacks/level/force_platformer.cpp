#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../../core/gui.hpp"

GUI_HACK_CREATE("Level", "Force Platformer", "Enables platformer mode on all levels", false);

class $modify(ForcePlatformerGJBL, GJBaseGameLayer) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Force Platformer");
        hack.addHookPtr(self.getHook("GJBaseGameLayer::loadLevelSettings"));
    }

    void loadLevelSettings() {
        if (PlayLayer::get() || m_levelSettings->m_platformerMode) {
            int levelLength = m_levelSettings->m_level->m_levelLength;
            bool platformerMode = m_levelSettings->m_platformerMode;

            m_levelSettings->m_platformerMode = true;
            GJBaseGameLayer::loadLevelSettings();
            m_levelSettings->m_level->m_levelLength = levelLength;
            m_levelSettings->m_platformerMode = platformerMode;
            return;
        }

        GJBaseGameLayer::loadLevelSettings();
    }
};

class $modify(ForcePlatformerPlayLayer, PlayLayer) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Force Platformer");
        hack.addHookPtr(self.getHook("PlayLayer::resetLevel"));
    }
    
    void resetLevel() {
        if (!m_isPlatformer) this->loadLevelSettings();
        PlayLayer::resetLevel();
    }
};