#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../../core/gui.hpp"
#include "../../core/keybinds.hpp"
#include "../../interface/imgui/widget_helper.hpp"

GUI_HACK_CREATE("Level", "Frame Stepper", "Allows forwards and backwards stepping frame-by-frame", true);

static bool g_nextFrame = false;

class $modify(FrameStepperCCScheduler, cocos2d::CCScheduler) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("cocos2d::CCScheduler::update", geode::Priority::Late); 

        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Frame Stepper");
        hack.addHookPtr(self.getHook("cocos2d::CCScheduler::update").unwrap());
        hack.setCustomWindowImGui([]() {
            ImGuiWidgetConfig::DrawCustomKeybindButton("frame_stepper::next_frame", "Next Frame");
        });
    }

    void update(float dt) {
        auto pl = PlayLayer::get();
        if (!pl || pl->m_playerDied || pl->m_isPaused || !pl->m_started)
            return CCScheduler::update(dt);

        if (!g_nextFrame) return;
        g_nextFrame = false;
        
        CCScheduler::update(dt);
    }
};

#ifdef GEODE_IS_ANDROID
class $modify(FrameStepperPlayLayer, PlayLayer) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Frame Stepper");
        hack.addHookPtr(self.getHook("PlayLayer::createObjectsFromSetupFinished").unwrap());
    }

    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();

        auto director = cocos2d::CCDirector::sharedDirector();
        auto win_size = director->getWinSize();
        float bottom = director->getScreenBottom();

        auto next_arrow = cocos2d::CCSprite::create("GDH_arrow.png"_spr);  
        next_arrow->setScale(1.f);
        next_arrow->setFlipX(true);

        auto next_step_click = geode::cocos::CCMenuItemExt::createSpriteExtra(next_arrow, [](CCMenuItemSpriteExtra* sender) {
            g_nextFrame = true;
        });       
        
        next_step_click->setPosition({120.f, director->getScreenTop() - next_step_click->getScaledContentHeight() - 5.f});
        next_step_click->setOpacity(200);
        next_step_click->setID("frame_stepper_next_btn"_spr);

        auto m_stepperMenu = cocos2d::CCMenu::create();
        m_stepperMenu->setID("frame_stepper_menu"_spr);
        m_stepperMenu->setPosition(0, 0);
        m_stepperMenu->setZOrder(999);

        m_stepperMenu->addChild(next_step_click);
        m_uiLayer->addChild(m_stepperMenu);
    }
};
#endif

#ifdef GEODE_IS_WINDOWS
$execute {
    auto& kb = GDH::Keybinds::get();
    kb.addCallback("frame_stepper::next_frame", geode::Keybind(cocos2d::KEY_C, KeyboardModifier::None), [](bool repeat) {
        g_nextFrame = true;
    });
}
#endif