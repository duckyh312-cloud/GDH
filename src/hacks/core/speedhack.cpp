#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"

GUI_HACK_CREATE("Invisible", "Speedhack", "", false);

class $modify(SpeedhackCCScheduler, cocos2d::CCScheduler) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("cocos2d::CCScheduler::update", geode::Priority::Early); 

        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Invisible").findHackByName("Speedhack");        
        hack.addHookPtr(self.getHook("cocos2d::CCScheduler::update").unwrap());
        
        auto &config = Config::get();
        float value = config.get<float>("invisible.speedhack::value", 1.f); 
        config.set<float>("invisible.speedhack::value", std::clamp(value, 0.25f, 1.f));
    }

    void update(float dt) {
        auto &config = Config::get();
        dt *= config.get<float>("invisible.speedhack::value", 1.f);

        CCScheduler::update(dt);
    }
};