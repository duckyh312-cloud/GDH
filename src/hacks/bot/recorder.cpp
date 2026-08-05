#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/UILayer.hpp>
#include "../../core/recorder/recorder.hpp"
#include "../../core/recorder/audio_recorder.hpp"

class $modify(PlayLayer) {
    struct Fields {
        ~Fields() {
            GDH::Recorder::get().stop();
        }
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        m_fields.self();
        
        auto pl = PlayLayer::get();
        auto& recorder = GDH::Recorder::get();

        if (!recorder.isRecording()) {
            auto time = geode::localtime(std::time(nullptr));
            recorder.videoName = fmt::format("{}_{:%Y-%m-%d_%H-%M-%S}.mp4", m_level->m_levelName, time);
        }

        return true;
    }

     void startMusic() {
        PlayLayer::startMusic();

        auto& recorder = GDH::Recorder::get();
        if (recorder.isRecording() && recorder.record_audio) {
            GDH::AudioRecorder::get().start();
        }
    }
};

class $modify(GJBaseGameLayer) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("GJBaseGameLayer::update", geode::Priority::Last); 
    }

    void update(float dt) {       
        GJBaseGameLayer::update(dt);
        
        auto& recorder = GDH::Recorder::get();
        if (!recorder.isRecording()) return;

        if (auto pl = PlayLayer::get()) {
            recorder.handle_recording(dt);
        }
    }
};

class $modify(FMODAudioEngine) {
    int playEffect(gd::string path, float speed, float unknown, float volume) {
        if (GDH::Recorder::get().isRecording() && path == "playSound_01.ogg")
            path = "";

        return FMODAudioEngine::playEffect(path, speed, unknown, volume);
    }
};

class $modify(UILayer) {
    void toggleMenuVisibility(bool visible) {
        if (GDH::Recorder::get().isRecording()) 
            return UILayer::toggleMenuVisibility(false);
        
        UILayer::toggleMenuVisibility(visible);
    }
};