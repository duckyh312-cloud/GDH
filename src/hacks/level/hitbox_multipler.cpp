#include <Geode/Geode.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/OBB2D.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../../interface/imgui/widget_helper.hpp"
#include "../../interface/cocos/hack_settings_popup.hpp"

GUI_HACK_CREATE("Level", "Hitbox Multiplier", "Multiplies object hitbox sizes", false);

static GameObject* g_rotatedObj = nullptr;
static std::vector<GameObject*> g_affectedObjs;

static float getMultiplier(GameObject* obj) {
    auto gjbgl = GJBaseGameLayer::get();
    auto& config = Config::get();

    if (!gjbgl || !config.get<bool>("level.hitbox_multiplier", false)) return 1.f;

    if (typeinfo_cast<PlayerObject*>(obj))
        return config.get<float>("level.hitbox_multiplier::player", 1.f);
    if (obj->m_objectType == GameObjectType::Hazard || obj->m_objectType == GameObjectType::AnimatedHazard)
        return config.get<float>("level.hitbox_multiplier::hazard", 1.f);
    if (obj->m_objectType == GameObjectType::Solid || obj->m_objectType == GameObjectType::Slope)
        return config.get<float>("level.hitbox_multiplier::solid", 1.f);

    return 1.f;
}

class $modify(HitboxMultiplierGameObject, GameObject) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Level").findHackByName("Hitbox Multiplier");
        hack.addHookPtr(self.getHook("GameObject::getObjectRect").unwrap());
        hack.addHookPtr(self.getHook("GameObject::updateOrientedBox").unwrap());

        hack.setCustomWindowImGui([
            pKey = hack.formatAdditionalSetting("player"),
            sKey = hack.formatAdditionalSetting("solid"),
            hKey = hack.formatAdditionalSetting("hazard")
        ] {
            ImGuiWidgetConfig::DragFloat("##Player Multiplier", pKey, 0.05f, 0.f, 10.f, 1.f, "Player Multiplier: %.2fx");
            ImGuiWidgetConfig::DragFloat("##Solid Multiplier", sKey, 0.05f, 0.f, 10.f, 1.f, "Solid Multiplier: %.2fx");
            ImGuiWidgetConfig::DragFloat("##Hazard Multiplier", hKey, 0.05f, 0.f, 10.f, 1.f, "Hazard Multiplier: %.2fx");
        });

        hack.setCustomWindowCocos([
            pKey = hack.formatAdditionalSetting("player"),
            sKey = hack.formatAdditionalSetting("solid"),
            hKey = hack.formatAdditionalSetting("hazard")
        ](cocos2d::CCNode* popupNode) {
            auto* popup = static_cast<HackSettingsPopup*>(popupNode);
            popup->addConfigFloatInput("Player", pKey, 0.f, 10.f, 1.f);
            popup->addConfigFloatInput("Solid", sKey, 0.f, 10.f, 1.f);
            popup->addConfigFloatInput("Hazard", hKey, 0.f, 10.f, 1.f);
        });
    }

    struct Fields { float m_origRadius = -1.f; };

    cocos2d::CCRect getObjectRect(float x, float y) {
        g_affectedObjs.push_back(this);
        if (m_fields->m_origRadius < 0) m_fields->m_origRadius = m_objectRadius;

        float mult = getMultiplier(this);
        m_objectRadius = m_fields->m_origRadius * mult;
        return GameObject::getObjectRect(x * mult, y * mult);
    }

    void updateOrientedBox() {
        g_rotatedObj = this;
        GameObject::updateOrientedBox();
    }
};

static void forceHitboxRecalculation() {
    for (auto obj : g_affectedObjs) {
        obj->setObjectRectDirty(true);
        obj->setOrientedRectDirty(true);
        obj->m_objectRadius = static_cast<HitboxMultiplierGameObject*>(obj)->m_fields->m_origRadius;
    }
    g_affectedObjs.clear();
}

static void resetState() {
    g_affectedObjs.clear();
    g_rotatedObj = nullptr;
}

class $modify(HitboxMultiplierOBB2D, OBB2D) {
    static void onModify(auto& self) {
        auto& hack = GDH::Gui::get().getWindow("Level").findHackByName("Hitbox Multiplier");
    }

    void calculateWithCenter(cocos2d::CCPoint center, float width, float height, float rotation) {
        if (!g_rotatedObj || !GJBaseGameLayer::get() || !Config::get().get<bool>("level.hitbox_multiplier", false)) {
            g_rotatedObj = nullptr;
            return OBB2D::calculateWithCenter(center, width, height, rotation);
        }

        g_affectedObjs.push_back(g_rotatedObj);
        float mult = getMultiplier(g_rotatedObj);
        OBB2D::calculateWithCenter(center, width * mult, height * mult, rotation);
        g_rotatedObj = nullptr;
    }
};

class $modify(HitboxMultiplierPlayLayer, PlayLayer) {
    struct Fields { ~Fields() { resetState(); } };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        m_fields.self();

        resetState();
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void resetLevel() { forceHitboxRecalculation(); PlayLayer::resetLevel(); }
};

class $modify(HitboxMultiplierEditorLayer, LevelEditorLayer) {
    struct Fields { ~Fields() { resetState(); } };

    bool init(GJGameLevel* level, bool noUI) {
        m_fields.self();

        resetState();
        bool result = LevelEditorLayer::init(level, noUI);
        resetState();
        return result;
    }

    void onPlaytest() { forceHitboxRecalculation(); LevelEditorLayer::onPlaytest(); }
};