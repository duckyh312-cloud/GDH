#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/modify/GJGarageLayer.hpp>
#include "../../core/gui.hpp"

GUI_HACK_CREATE("Core", "Unlock Icons", "Unlock All Icons", false);

class $modify(UnlockIconsGameManager, GameManager) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Core").findHackByName("Unlock Icons");       

        hack.addHookPtr(self.getHook("GameManager::isColorUnlocked"));
        hack.addHookPtr(self.getHook("GameManager::isIconUnlocked"));
    }

    bool isColorUnlocked(int key, UnlockType type) {
        return true;
    }
    
    bool isIconUnlocked(int key, IconType type) {
        return true;
    }
};

class $modify(UnlockIconsGameStatsManager, GameStatsManager) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Core").findHackByName("Unlock Icons");        
     
        hack.addHookPtr(self.getHook("GameStatsManager::isStoreItemUnlocked"));
    }
    
    bool isStoreItemUnlocked(int index) {
        return true;
    }
};