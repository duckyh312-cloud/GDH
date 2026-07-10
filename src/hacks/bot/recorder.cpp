#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../../core/recorder/recorder.hpp"

class $modify(GJBaseGameLayer) {
    static void onModify(auto& self) {
        (void) self.setHookPriority("GJBaseGameLayer::update", geode::Priority::Last); 
    }

    void update(float dt) {       
        GJBaseGameLayer::update(dt);
        
        auto& recorder = GDH::Recorder::get();
        if (!recorder.is_recording) return;

        if (auto pl = PlayLayer::get()) {
            recorder.handle_recording(dt);
        }
    }
};