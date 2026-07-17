#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "../../core/config.hpp"
#include "../../core/replayEngine.hpp"
#include "../../core/gui.hpp"
#include "../../core/utils.hpp"

static float left_over = 0.f;

#ifdef GEODE_IS_WINDOWS 
static int expectedTicks = 5;
uintptr_t offset = 0;

$execute {
    offset = GDH::Utils::PatternScan(geode::base::get(), 0x250000, "F3 0F 10 86 ? ? ? ? F3 44 0F 10 1D");
}

class $modify(ReplayCCScheduler, cocos2d::CCScheduler) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Invisible").findHackByName("TPS");   
        hack.setCheating(true);     
        hack.setEarlyInit(false);

        hack.setHandler([](bool enabled) {
            auto expectedTicksAddr = (uintptr_t)&expectedTicks;
            std::vector<uint8_t> patchBytes;

            // mov rax, expectedTicks (48 B8 + 8 byte address)
            patchBytes.push_back(0x48);
            patchBytes.push_back(0xB8);
            for (int i = 0; i < 8; i ++) {
                patchBytes.push_back((expectedTicksAddr >> (i * 8)) & 0xFF);
            }

            //mov r11d, [rax] (44 8B 18)
            patchBytes.push_back(0x44);
            patchBytes.push_back(0x8B);
            patchBytes.push_back(0x18);

            for (int i = 0; i < 54; i++) {
                patchBytes.push_back(0x90); // NOP
            }

            if (offset != 0) {
                static auto result = geode::Mod::get()->patch((void*)(offset), patchBytes);
                static auto patch = result.isErr() ? nullptr : result.unwrap();
                if (patch) (void)patch->toggle(enabled);
            }
            else {
                geode::log::error("Couldn't find the offset for the TPS bypass");
            }
        });
    }
};
#elif defined(GEODE_IS_ANDROID) 
class $modify(ReplayCCScheduler, cocos2d::CCScheduler) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Invisible").findHackByName("TPS");   
        hack.setCheating(true);     
        hack.addHookPtr(self.getHook("cocos2d::CCScheduler::update").unwrap());
    }

    void update(float dt) {
        auto &engine = GDH::ReplayEngine::get();
        auto &config = Config::get();

        float tps_value = config.get<float>("invisible.tps::value", 240.f);
        float new_dt = 1.f / tps_value;

        unsigned times = static_cast<int>((dt + left_over) / new_dt);  
        if (dt <= 0.f)
            return CCScheduler::update(new_dt);

        auto start = std::chrono::high_resolution_clock::now();
        using namespace std::literals;

        for (unsigned i = 0; i < times; ++i) {
            CCScheduler::update(new_dt);

            if (std::chrono::high_resolution_clock::now() - start > 33.333ms) {         
                times = i + 1;
                break;
            }
        }

        left_over = std::max(0.f, left_over + dt - new_dt * times);
    }
};

#endif

class $modify(ReplayGJBaseGameLayer, GJBaseGameLayer) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Invisible").findHackByName("TPS");       
        hack.setCheating(true); 
        hack.addHookPtr(self.getHook("GJBaseGameLayer::getModifiedDelta").unwrap());
    }

    // void update(float dt) {
    //     auto &config = Config::get();
    //     auto &engine = GDH::ReplayEngine::get();

    //     GJBaseGameLayer::update(dt);
    //     engine.handle_update(this);
    // }

    double getModifiedDelta(float dt) {
        if (m_resumeTimer > 0)
        {
            --m_resumeTimer;
            dt = 0.0;
        }
        
        auto& config = Config::get();
        auto tps = config.get<float>("invisible.tps::value", 240.f);
        auto fixed_dt = 1.0 / static_cast<double>(tps);

        auto timestep = std::min(static_cast<double>(m_gameState.m_timeWarp), 1.0) * fixed_dt;
        auto total_dt = dt + m_extraDelta;
        auto steps = std::round(total_dt / timestep);
        auto new_dt = steps * timestep;
        m_extraDelta = total_dt - new_dt;

        #ifdef GEODE_IS_WINDOWS 
        if (new_dt > 0.0) {
            // float v6 = new_dt * 60.f;
            // expectedTicks = std::max(1.f, std::round(v6 / std::min(m_gameState.m_timeWarp, 1.f) * 4.f));

            float v6 = new_dt * tps;
            expectedTicks = std::max(1.f, std::round(v6 / std::min(m_gameState.m_timeWarp, 1.f)));
        }
        #endif

        return static_cast<double>(new_dt);
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        GDH::ReplayEngine::get().handle_button(down, button, isPlayer1);
    }
    
    // void processCommands(float dt, bool isHalfTick, bool isLastTick) {
    //     GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    // }
        
    void processQueuedButtons(float dt, bool clearInputQueue) {
        GJBaseGameLayer::processQueuedButtons(dt, clearInputQueue);
        
        if (!m_levelEndAnimationStarted) {
            auto &engine = GDH::ReplayEngine::get();
            engine.handle_commands(this);
            engine.handle_update(this);
        }
    }
};

class $modify(ReplayPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        auto& engine = GDH::ReplayEngine::get();
        if (engine.get_actions_size() == 0) {
            engine.replay_name = fmt::format("{}", level->m_levelName);
        }

        return true;
    }

    void resetLevel() {
        auto &config = Config::get();
        auto &engine = GDH::ReplayEngine::get();

        left_over = 0;
        PlayLayer::resetLevel();
        
        engine.handle_reset();  
    }

    void playEndAnimationToPos(cocos2d::CCPoint pos) {
        PlayLayer::playEndAnimationToPos(pos);
        geode::queueInMainThread([]() {
            auto& engine = GDH::ReplayEngine::get();
            if (engine.mode == state::record)
                engine.mode = state::disable;
        });
    }

    void playPlatformerEndAnimationToPos(cocos2d::CCPoint pos, bool instant) {
        PlayLayer::playPlatformerEndAnimationToPos(pos, instant);
        geode::queueInMainThread([]() {
            auto& engine = GDH::ReplayEngine::get();
            if (engine.mode == state::record)
                engine.mode = state::disable;
        });
    }
};

class $modify(ReplayLevelEditorLayer, LevelEditorLayer) {
    void onPlaytest() {
        GDH::ReplayEngine::get().handle_reset();
        LevelEditorLayer::onPlaytest();
    }
};