#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../../core/keybinds.hpp"
#include "../../interface/imgui/widget_helper.hpp"
#include "../../core/ringBuffer.hpp"
#include <imgui-cocos.hpp>
#include "frame_stepper.hpp"
#include "../../interface/cocos/hack_settings_popup.hpp"

GUI_HACK_CREATE("Level", "Frame Stepper", "Allows forwards and backwards stepping frame-by-frame", true);

static bool g_nextFrame = false;
static bool g_prevFrame = false;
static bool g_pause = true;

bool FrameStepper::g_back = false;

static RingBuffer<CheckpointObject*> g_checkpoints(240, [](CheckpointObject* cp) {
    if (cp) cp->release();
}, [](CheckpointObject* cp) {
    if (cp) cp->release();
});

class $modify(FrameStepperCCScheduler, cocos2d::CCScheduler) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("cocos2d::CCScheduler::update", geode::Priority::Late); 

        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Frame Stepper");
        hack.addHookPtr(self.getHook("cocos2d::CCScheduler::update").unwrap());

        hack.setHandler([](bool enabled) {
            if (enabled) {
                g_prevFrame = false;
                g_pause = true;
                FrameStepper::g_back = false;
            }
        });

        hack.setCustomWindowImGui([]() {
            ImGuiWidgetConfig::DrawCustomKeybindButton("frame_stepper::step_key", "Step");
            ImGuiWidgetConfig::DrawCustomKeybindButton("frame_stepper::pause_key", "Pause");
            ImGuiWidgetConfig::DrawCustomKeybindButton("frame_stepper::back_key", "Back");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGuiWidgetConfig::Checkbox("Backward Stepping", "frame_stepper::backward", false);
            ImGuiWidgetConfig::DragInt("##BackFrameLength", "frame_stepper::back_frames_length", 1, 1, 1000, 240, "Back Frames Length: %d");
        });

        hack.setCustomWindowCocos([](cocos2d::CCNode* popupNode) {
            auto* popup = static_cast<HackSettingsPopup*>(popupNode);
            popup->addConfigToggle("Backward Stepping", "frame_stepper::backward", false);
            popup->addConfigIntInput("Back Frames Length", "frame_stepper::back_frames_length", 1, 1000, 240);
        });
    }

    void update(float dt) {
        auto pl = PlayLayer::get();
        if (!pl || pl->m_isPaused || !pl->m_started)
            return CCScheduler::update(dt);

        if (g_prevFrame) {
            g_prevFrame = false;

            if (!g_checkpoints.empty()) {
                FrameStepper::g_back = true;
                pl->resetLevel();
                FrameStepper::g_back = false;

                CheckpointObject* last = nullptr;
                g_checkpoints.pop_back(last);
                if (last) last->release();

                g_nextFrame = true;
            }
        }

        if (g_pause) {
            if (g_nextFrame) {
                g_nextFrame = false;
                CCScheduler::update(dt);
            }
            return;
        }

        CCScheduler::update(dt);
    }
};

static size_t g_lastFramesLength = 0;
class $modify(FrameStepperGJBaseGameLayer, GJBaseGameLayer) {
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (!Config::get().get<bool>("frame_stepper::backward", false)) return;

        auto pl = PlayLayer::get();
        if (!pl || !pl->m_started || pl->m_playerDied) return;

        auto& config = Config::get();
        auto framesLength = static_cast<size_t>(config.get<int>("frame_stepper::back_frames_lengtht", 240));

        if (g_lastFramesLength != framesLength) {
            g_checkpoints.init(framesLength, [](CheckpointObject* cp) {
                if (cp) cp->release();
            }, [](CheckpointObject* cp) {
                if (cp) cp->release();
            });
            g_lastFramesLength = framesLength;
        }

        auto cp = pl->createCheckpoint();
        cp->retain(); 
        g_checkpoints.push(cp);
    }
};

class $modify(FrameStepperPlayLayer, PlayLayer) {
    struct Fields {
        ~Fields() {
            g_checkpoints.clear();
        }
    };

    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Frame Stepper");
        
        hack.addHookPtr(self.getHook("PlayLayer::updateAttempts").unwrap());
        hack.addHookPtr(self.getHook("PlayLayer::resetLevel").unwrap());
        #ifdef GEODE_IS_ANDROID
        hack.addHookPtr(self.getHook("PlayLayer::createObjectsFromSetupFinished").unwrap());
        #endif
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        m_fields.self();
        return true;
    }

    void updateAttempts() {
        if (FrameStepper::g_back) return;

        PlayLayer::updateAttempts();
    }

    void resetLevel() {
        CheckpointObject* last = nullptr;
        bool testmode = m_isTestMode;

        if (FrameStepper::g_back && g_checkpoints.back(last) && last) {
            m_checkpointArray->addObject(last);
            m_isTestMode = true;
        }

        PlayLayer::resetLevel();

        if (FrameStepper::g_back && !g_checkpoints.empty()) {
            m_isTestMode = testmode;
            removeCheckpoint(false);
        }
    }
    
    #ifdef GEODE_IS_ANDROID
    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();

        auto director = cocos2d::CCDirector::sharedDirector();
        auto next_arrow = cocos2d::CCSprite::create("GDH_arrow.png"_spr);  
        next_arrow->setScale(1.f);
        next_arrow->setFlipX(true);

        auto next_step_click = geode::cocos::CCMenuItemExt::createSpriteExtra(next_arrow, [](CCMenuItemSpriteExtra* sender) {
            g_pause = true;
            g_nextFrame = true;
        });       
        
        next_step_click->setPosition({120.f, director->getScreenTop() - next_step_click->getScaledContentHeight() - 5.f});
        next_step_click->setOpacity(200);
        next_step_click->setID("frame_stepper_next_btn"_spr);

        auto back_arrow = cocos2d::CCSprite::create("GDH_arrow.png"_spr);  
        back_arrow->setScale(1.f);

        
        auto back_arrow_click = geode::cocos::CCMenuItemExt::createSpriteExtra(back_arrow, [](CCMenuItemSpriteExtra* sender) {
            g_pause = true;
            g_prevFrame = true;
        });  

        back_arrow_click->setPosition({120.f, director->getScreenTop() - back_arrow_click->getScaledContentHeight() * 2 - 10.f});
        back_arrow_click->setOpacity(200);
        back_arrow_click->setID("frame_stepper_back_btn"_spr);

        auto m_stepperMenu = cocos2d::CCMenu::create();
        m_stepperMenu->setID("frame_stepper_menu"_spr);
        m_stepperMenu->setPosition(0, 0);
        m_stepperMenu->setZOrder(999);

        m_stepperMenu->addChild(next_step_click);
        if (Config::get().get<bool>("frame_stepper::backward", false)) {
            m_stepperMenu->addChild(back_arrow_click);
        }
        m_uiLayer->addChild(m_stepperMenu);
    }
    #endif
};

#ifdef GEODE_IS_WINDOWS
$execute {
    auto& kb = GDH::Keybinds::get();
    kb.addCallback("frame_stepper::step_key", geode::Keybind(cocos2d::KEY_G, KeyboardModifier::None), [](bool repeat) {
        g_pause = true;
        g_nextFrame = true;
    });

    kb.addCallback("frame_stepper::pause_key", geode::Keybind(cocos2d::KEY_H, KeyboardModifier::None), [](bool repeat) {
        if (repeat) return;
        g_pause = !g_pause;
    });

    kb.addCallback("frame_stepper::back_key", geode::Keybind(cocos2d::KEY_F, KeyboardModifier::None), [](bool repeat) {
        g_pause = true;
        g_prevFrame = true;
    });
}
#endif