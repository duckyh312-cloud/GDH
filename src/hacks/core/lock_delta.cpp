#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "../../core/config.hpp"
#include "../../core/replayEngine.hpp"
#include "../../core/gui.hpp"
#include "../../core/recorder/recorder.hpp"
#include "../../core/recorder/audio_recorder.hpp"

static float left_over = 0.f;

GUI_HACK_CREATE("Invisible", "Lock Delta", "", true);

class $modify(LockDeltaCCScheduler, cocos2d::CCScheduler) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Invisible").findHackByName("Lock Delta");
    }

    void update(float dt) {
        auto &config = Config::get();
        auto& recorder = GDH::Recorder::get();
        auto &engine = GDH::ReplayEngine::get();

        auto pl = PlayLayer::get();
        if (recorder.is_recording && pl && !pl->m_isPaused) {
            auto& audio = RecorderAudio::get();
            if (audio.is_recording && audio.target_audio_time > audio.get_recorded_time()) {
                return; 
            }

            auto record_dt = 1.f / static_cast<float>(recorder.fps);
            recorder.resize_resolution([this, record_dt] { CCScheduler::update(record_dt); });
            left_over = 0.f;
            return;
        }

        if (!config.get<bool>("invisible.lock_delta", false))
            return CCScheduler::update(dt);

        float tps_value = config.get<float>("invisible.lock_delta::value", 240.f);
        float new_dt = 1.f / tps_value;

        if (dt <= 0.f) {
            CCScheduler::update(new_dt);
            return;
        }

        if (!config.get<bool>("invisible.lock_delta::real_time", true)) {
            CCScheduler::update(new_dt);
            return;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        using namespace std::literals;
        
        unsigned times = static_cast<int>((dt + left_over) / new_dt);  
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

class $modify(LockDeltalayLayer, PlayLayer) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Invisible").findHackByName("Lock Delta");

        hack.addHookPtr(self.getHook("PlayLayer::resetLevel").unwrap());
    }

    void resetLevel() {
        left_over = 0;
        PlayLayer::resetLevel();
    }
};