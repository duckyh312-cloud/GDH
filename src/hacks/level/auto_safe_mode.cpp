#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"

using namespace geode::prelude;

GUI_HACK_CREATE("Level", "Auto Safe Mode", "Disables progress on levels when enabled unlegit hacks", false);

static bool m_cheatedInCurrentAttempt = false;

class $modify(AutoSafeModeEndLevelLayer, EndLevelLayer) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Auto Safe Mode");
        
        hack.addHookPtr(self.getHook("EndLevelLayer::showLayer").unwrap());
    }

    void showLayer(bool p0) {
        EndLevelLayer::showLayer(p0);

        auto& activeCheats = GDH::Gui::get().getActiveCheats();
        if (activeCheats.empty() && !m_cheatedInCurrentAttempt) return;

        std::string text = "";
        if (activeCheats.empty() && m_cheatedInCurrentAttempt) 
            text += "Cheated but hacks was disabled\n";

        for (const auto& hackID : activeCheats)
            text += hackID + "\n";
        
        text += "Auto Safe Mode (GDH)";

        if (auto* label = geode::Label::create(text, "GoogleSans.fnt"_spr)) {
            label->setScale(0.40f);
            label->setAnchorPoint({0.0f, 0.0f});
            label->setPosition({3.0f, 3.0f});
            label->setColor({255, 128, 128});
            label->setOpacity(180);
            this->addChild(label);
        }
    }
};
class $modify(AutoSafeModePlayLayer, PlayLayer) {
    struct Fields {
        ~Fields() {
            m_cheatedInCurrentAttempt = false;
        }
    };

    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Auto Safe Mode");
        
        // hack.addHookPtr(self.getHook("PlayLayer::init").unwrap());
        hack.addHookPtr(self.getHook("PlayLayer::destroyPlayer").unwrap());
        hack.addHookPtr(self.getHook("PlayLayer::resetLevel").unwrap());
        hack.addHookPtr(self.getHook("PlayLayer::levelComplete").unwrap());
        hack.addHookPtr(self.getHook("PlayLayer::updateProgressbar").unwrap());
    }

    void checkCheats() {
        if (!m_cheatedInCurrentAttempt) {
            auto& activeCheats = GDH::Gui::get().getActiveCheats();
            auto& config = Config::get();

            if (!activeCheats.empty()) {
                m_cheatedInCurrentAttempt = true;
            }
        }
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontRun) {
        if (!PlayLayer::init(level, useReplay, dontRun)) return false;
        m_fields.self();
        checkCheats();
        return true;
    }

    void updateProgressbar() {
        checkCheats();
        PlayLayer::updateProgressbar();
    }

    void resetLevel() {
        m_cheatedInCurrentAttempt = false;
        checkCheats();
        PlayLayer::resetLevel();
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        bool originalTestMode = m_isTestMode;
        
        if (m_cheatedInCurrentAttempt) {
            m_isTestMode = true; 
        }

        PlayLayer::destroyPlayer(player, obj);
        m_isTestMode = originalTestMode;
    }

    void levelComplete() {
        bool originalTestMode = m_isTestMode;
        
        if (m_cheatedInCurrentAttempt) {
            m_isTestMode = true;
        }

        PlayLayer::levelComplete();
        m_isTestMode = originalTestMode;
    }
};