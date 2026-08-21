#include <Geode/Geode.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/HardStreak.hpp>

#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../../interface/imgui/widget_helper.hpp"
#include "../../interface/cocos/hack_settings_popup.hpp"
#include "../../core/utils.hpp"
#include "imgui.h"

using namespace geode::prelude;

GUI_HACK_CREATE("Level", "Show Trajectory", "Draws the predicted paths of a player for continuous button holds/release", true);

namespace TrajectoryManager {
    static PlayerObject* dummy_jump = nullptr;
    static PlayerObject* dummy_normal = nullptr;

    static bool is_simulating = false;
    static bool simulation_dead = false;
    static bool p1_holding = false;
    static bool p2_holding = false;

    static float dt_step = 0.0f;
    static cocos2d::CCDrawNode* draw_node = nullptr;

    static void createDrawNode(cocos2d::CCLayer* layer, cocos2d::CCNode* debugNode) {
        auto* parent = debugNode ? debugNode->getParent() : layer;
        if (!parent) return;

        draw_node = cocos2d::CCDrawNode::create();
        draw_node->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
        draw_node->setZOrder(9999);
        draw_node->setID("trajectory-node"_spr);
        draw_node->m_bUseArea = false;
        parent->addChild(draw_node);
    }

    static PlayerObject* createVirtualPlayer(PlayLayer* layer) {
        if (!layer) return nullptr;
        auto* dummy = PlayerObject::create(1, 1, layer, layer, true);
        if (dummy) {
            dummy->setPosition({0, 105});
            dummy->setVisible(false);
            dummy->setID("trajectory-fake-player"_spr);
            if (layer->m_objectLayer) {
                layer->m_objectLayer->addChild(dummy);
            }
        }
        return dummy;
    }

    static void clearCollisionLogs(PlayerObject* player) {
        if (!player) return;
        if (player->m_collisionLogTop) player->m_collisionLogTop->removeAllObjects();
        if (player->m_collisionLogBottom) player->m_collisionLogBottom->removeAllObjects();
        if (player->m_collisionLogLeft) player->m_collisionLogLeft->removeAllObjects();
        if (player->m_collisionLogRight) player->m_collisionLogRight->removeAllObjects();
    }

    static void renderHitboxOutline(const cocos2d::CCRect& rect, const cocos2d::ccColor4F& border) {
        if (!draw_node) return;
        cocos2d::CCPoint vertices[4] = {
            {rect.getMinX(), rect.getMinY()},
            {rect.getMinX(), rect.getMaxY()},
            {rect.getMaxX(), rect.getMaxY()},
            {rect.getMaxX(), rect.getMinY()}
        };
        draw_node->drawPolygon(vertices, 4, {0.f, 0.f, 0.f, 0.f}, 0.25f, border);
    }

    static void simulatePath(PlayLayer* layer, PlayerObject* simPlayer, PlayerObject* sourcePlayer, bool isJumpInput) {
        if (!layer || !simPlayer || !sourcePlayer) return;

        bool isSecondPlayer = (sourcePlayer == layer->m_player2);
        simPlayer->copyAttributes(sourcePlayer);
        simPlayer->m_gravityMod = sourcePlayer->m_gravityMod;
        simPlayer->m_isSideways = sourcePlayer->m_isSideways;

        // dashing stuff
        simPlayer->m_isDashing = sourcePlayer->m_isDashing;
        simPlayer->m_dashX = sourcePlayer->m_dashX;
        simPlayer->m_dashY = sourcePlayer->m_dashY;
        simPlayer->m_dashAngle = sourcePlayer->m_dashAngle;

        bool isCurrentlyHolding = isSecondPlayer ? p2_holding : p1_holding;
        if (isCurrentlyHolding) {
            isJumpInput ? simPlayer->pushButton(PlayerButton::Jump) : simPlayer->releaseButton(PlayerButton::Jump);
        }

        simPlayer->m_isOnGround = sourcePlayer->m_isOnGround;
        simPlayer->setVisible(false);
        simulation_dead = false;

        auto& config = Config::get();
        int iterations = config.get<int>("level.show_trajectory::iterations", 300);

        auto jumpColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_trajectory::jumpColor", "#00ff00FF"));
        auto normalColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_trajectory::normalColor", "#ff0000FF"));

        auto playerInnerColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::playerInnerColor", "#003FFFFF"));
        auto playerColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::playerColor", "#FF0000FF"));

        cocos2d::ccColor4F lineColor = isJumpInput ? jumpColor : normalColor;
        bool inputApplied = false;

        for (int i = 0; i < iterations; ++i) {
            cocos2d::CCPoint currentPos = simPlayer->getPosition();
            clearCollisionLogs(simPlayer);

            layer->checkCollisions(simPlayer, dt_step, false);
            if (simulation_dead) break;

            if (!inputApplied && !isCurrentlyHolding) {
                inputApplied = true;
                isJumpInput ? simPlayer->pushButton(PlayerButton::Jump) : simPlayer->releaseButton(PlayerButton::Jump);
            }

            simPlayer->update(dt_step);
            draw_node->drawSegment(currentPos, simPlayer->getPosition(), 0.65f, lineColor);
        }

        renderHitboxOutline(simPlayer->getObjectRect(), playerColor);
        renderHitboxOutline(simPlayer->getObjectRect(0.25f, 0.25f), playerInnerColor);
    }

    static void runTrajectorySimulation(PlayLayer* layer) {
        if (!layer || !draw_node) return;

        auto& config = Config::get();
        if (!config.get<bool>("level.show_trajectory", false)) {
            draw_node->clear();
            return;
        }

        if (!dummy_jump || !dummy_normal) return;

        is_simulating = true;
        draw_node->clear();

        if (layer->m_player1) {
            simulatePath(layer, dummy_jump, layer->m_player1, true);
            simulatePath(layer, dummy_normal, layer->m_player1, false);
        }

        if (layer->m_gameState.m_isDualMode && layer->m_player2) {
            simulatePath(layer, dummy_jump, layer->m_player2, true);
            simulatePath(layer, dummy_normal, layer->m_player2, false);
        }

        is_simulating = false;
    }

    static void reset() {
        dummy_jump = nullptr;
        dummy_normal = nullptr;
        is_simulating = false;
        simulation_dead = false;
    }
}

class $modify(ShowTrajectoryPlayLayer, PlayLayer) {
    struct Fields {
        ~Fields() {
            TrajectoryManager::reset();
        }
    };
    
    static void onModify(auto& self) {
        (void) self.setHookPriority("PlayLayer::destroyPlayer", geode::Priority::Early); 
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        m_fields.self();
        TrajectoryManager::dummy_jump = TrajectoryManager::createVirtualPlayer(this);
        TrajectoryManager::dummy_normal = TrajectoryManager::createVirtualPlayer(this);
        TrajectoryManager::createDrawNode(this, m_debugDrawNode);

        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (object != m_anticheatSpike && TrajectoryManager::is_simulating) {
            if (player == TrajectoryManager::dummy_jump || player == TrajectoryManager::dummy_normal) {
                TrajectoryManager::simulation_dead = true;
                return;
            }
        }
        PlayLayer::destroyPlayer(player, object);
    }

    void playEndAnimationToPos(cocos2d::CCPoint position) {
        if (TrajectoryManager::is_simulating) return;
        PlayLayer::playEndAnimationToPos(position);
    }
};

class $modify(ShowTrajectoryBaseGameLayer, GJBaseGameLayer) {
    void updateCamera(float dt) {
        if (auto pl = PlayLayer::get()) 
            TrajectoryManager::runTrajectorySimulation(pl);
        
        GJBaseGameLayer::updateCamera(dt);
    }
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& holding = isPlayer1 ? TrajectoryManager::p1_holding : TrajectoryManager::p2_holding;
        if (button == 1) holding = down;

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    bool canBeActivatedByPlayer(PlayerObject* player, EffectGameObject* object) {
        if (TrajectoryManager::is_simulating) return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(player, object);
    }

    void collisionCheckObjects(PlayerObject* object, gd::vector<GameObject*>* objects, int objectCount, float dt) {
        if (TrajectoryManager::is_simulating) {
            gd::vector<GameObject*> solidPhysicsOnly;
            solidPhysicsOnly.reserve(objectCount);
            
            for (int i = 0; i < objectCount; ++i) {
                auto obj = objects->at(i);
                if (!obj) continue;
                if (obj->m_objectType == GameObjectType::Solid ||
                    obj->m_objectType == GameObjectType::Hazard ||
                    obj->m_objectType == GameObjectType::AnimatedHazard ||
                    obj->m_objectType == GameObjectType::Slope) {
                    solidPhysicsOnly.push_back(obj);
                }
            }
            GJBaseGameLayer::collisionCheckObjects(object, &solidPhysicsOnly, static_cast<int>(solidPhysicsOnly.size()), dt);
            return;
        }
        GJBaseGameLayer::collisionCheckObjects(object, objects, objectCount, dt);
    }

    void flipGravity(PlayerObject* object, bool flip, bool noEffects) {
        if (TrajectoryManager::is_simulating) return;
        GJBaseGameLayer::flipGravity(object, flip, noEffects);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* object) {
        if (TrajectoryManager::is_simulating) return;
        GJBaseGameLayer::playerTouchedRing(player, object);
    }
};

class $modify(ShowTrajectoryPlayerObject, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);
        if (!TrajectoryManager::is_simulating) {
            if (auto pl = PlayLayer::get()) {
                float timeWarp = pl->m_gameState.m_timeWarp;
                TrajectoryManager::dt_step = (timeWarp > 0.001f) ? (dt / timeWarp) : dt;
            }
        }
    }
    
    void stopDashing() {
        if (!TrajectoryManager::is_simulating)
            return PlayerObject::stopDashing();

        bool orig = m_playEffects;

        m_playEffects = false;
        PlayerObject::stopDashing();
        m_playEffects = orig;
    }
    
    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (TrajectoryManager::is_simulating) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }

    void ringJump(RingObject* object, bool skipCheck) {
        if (TrajectoryManager::is_simulating) return;
        PlayerObject::ringJump(object, skipCheck);
    }

    void incrementJumps() {
        if (TrajectoryManager::is_simulating) return;
        PlayerObject::incrementJumps();
    }
};

class $modify(ShowTrajectoryEffectGameObject, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* layer, int uniqueID, const gd::vector<int>* remapKeys) {
        if (TrajectoryManager::is_simulating) return;
        EffectGameObject::triggerObject(layer, uniqueID, remapKeys);
    }
};

class $modify(ShowTrajectoryGameObject, GameObject) {
    void playShineEffect() {
        if (TrajectoryManager::is_simulating) return;
        GameObject::playShineEffect();
    }
};

class $modify(ShowTrajectoryHardStreak, HardStreak) {
    void addPoint(cocos2d::CCPoint point) {
        if (TrajectoryManager::is_simulating) return;
        HardStreak::addPoint(point);
    }
};

class $modify(ShowTrajectoryLevelEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool noUI) {
        if (!LevelEditorLayer::init(level, noUI)) return false;
        TrajectoryManager::reset();
        return true;
    }
};

$execute {
    auto& config = Config::get();
    auto& gui = GDH::Gui::get();
    auto& hack = gui.getWindow("Level").findHackByName("Show Trajectory");

    hack.setCustomWindowImGui([
        iterationsKey = hack.formatAdditionalSetting("iterations"),
        cJumpKey = hack.formatAdditionalSetting("jumpColor"),
        cNormalKey = hack.formatAdditionalSetting("normalColor"),
        cPlayerKey = hack.formatAdditionalSetting("playerColor"),
        cPlayerInnerKey = hack.formatAdditionalSetting("playerInnerColor")
    ]{
        ImGuiWidgetConfig::DragInt("##Trajectory_Iterations", iterationsKey, 1, 10, 2000, 300, "Iterations: %d");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Line Colors:");

        ImGuiWidgetConfig::ColorEdit4Hex("Jump Trajectory", cJumpKey, "#00ff00FF");
        ImGuiWidgetConfig::ColorEdit4Hex("Normal Trajectory", cNormalKey, "#ff0000FF");
    });

    hack.setCustomWindowCocos([
        iterationsKey = hack.formatAdditionalSetting("iterations"),
        cJumpKey = hack.formatAdditionalSetting("jumpColor"),
        cNormalKey = hack.formatAdditionalSetting("normalColor"),
        cPlayerKey = hack.formatAdditionalSetting("playerColor"),
        cPlayerInnerKey = hack.formatAdditionalSetting("playerInnerColor")
    ](cocos2d::CCNode* popupNode){
        auto* popup = static_cast<HackSettingsPopup*>(popupNode);
        popup->addConfigIntInput("Iterations", iterationsKey, 10, 2000, 300);

        popup->addSeparator();

        popup->addConfigColor4Hex("Jump Trajectory", cJumpKey, "#00ff00FF");
        popup->addConfigColor4Hex("Normal Trajectory", cNormalKey, "#ff0000FF");
    });
}