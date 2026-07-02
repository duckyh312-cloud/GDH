#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include "../../core/gui.hpp"
#include "../../core/config.hpp"
#include "../../interface/imgui/widget_helper.hpp"
#include "../../core/utils.hpp"
#include "imgui.h"

using namespace geode::prelude;

GUI_HACK_CREATE("Level", "Show Hitboxes", "Visualises level hitboxes", true);

static cocos2d::CCDrawNode* createDrawNode(cocos2d::CCLayer* layer, cocos2d::CCNode* debugNode, const char* id) {
    auto* parent = debugNode ? debugNode->getParent() : layer;
    if (!parent) return nullptr;

    auto* drawNode = cocos2d::CCDrawNode::create();
    drawNode->setBlendFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
    drawNode->setZOrder(9999);
    drawNode->setID(id);
    drawNode->m_bUseArea = false;
    parent->addChild(drawNode);

    return drawNode;
}

static void drawRect(cocos2d::CCDrawNode* node, cocos2d::CCRect const& rect,
                     cocos2d::ccColor4F const& color, float borderWidth, cocos2d::ccColor4F const& borderColor) 
{
    cocos2d::CCPoint vertices[4] = {
        {rect.getMinX(), rect.getMinY()},
        {rect.getMinX(), rect.getMaxY()},
        {rect.getMaxX(), rect.getMaxY()},
        {rect.getMaxX(), rect.getMinY()}
    };
    node->drawPolygon(vertices, 4, color, borderWidth, borderColor);
}

static void drawOrientedOrRect(GJBaseGameLayer* layer, cocos2d::CCDrawNode* drawNode, GameObject* obj,
                               cocos2d::ccColor4F const& fill, float bw, cocos2d::ccColor4F const& border) 
{
    bool isEditor = layer->m_isEditor;

    if (auto* ob = isEditor ? obj->getOrientedBox() : obj->m_orientedBox) {
        drawNode->drawPolygon(ob->m_corners.data(), 4, fill, bw, border);
    } else {
        bool dirty = obj->m_isObjectRectDirty;
        bool boxOff = obj->m_boxOffsetCalculated;
        drawRect(drawNode, obj->getObjectRect(), fill, bw, border);
        obj->m_isObjectRectDirty = dirty;
        obj->m_boxOffsetCalculated = boxOff;
    }
}

void updateHitboxes(GJBaseGameLayer* layer, cocos2d::CCDrawNode* drawNode) {
    if (!drawNode) return;
    drawNode->clear();

    auto& config = Config::get();
    if (!config.get<bool>("level.show_hitboxes", false)) return;
    if (config.get<bool>("level.show_hitboxes::show_on_death", false) && !layer->m_isEditor && !layer->m_playerDied) return;

    auto triggersColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::triggersColor", "#FF00E6FF"));
    auto otherColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::otherColor", "#00FF00FF"));
    auto passableColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::passableColor", "#00FFFFFF"));
    auto solidColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::solidColor", "#003FFFFF"));
    auto dangerColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::dangerColor", "#FF0000FF"));

    auto playerRotateColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::playerRotateColor", "#800000FF"));
    auto playerInnerColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::playerInnerColor", "#003FFFFF"));
    auto playerColor = GDH::Utils::hexToColor4F(config.get<std::string>("level.show_hitboxes::playerColor", "#FF0000FF"));

    auto size = config.get<float>("level.show_hitboxes::size", 0.5f);
    auto fillAlpha = config.get<float>("level.show_hitboxes::fill_alpha", 0.2f);

    auto getFillColor = [fillAlpha](cocos2d::ccColor4F color) -> cocos2d::ccColor4F {
        return { color.r, color.g, color.b, fillAlpha };
    };

    if (!layer->m_sections.empty()) {
        int rightBound = std::min(layer->m_rightSectionIndex, static_cast<int>(layer->m_sections.size()) - 1);
        for (int i = layer->m_leftSectionIndex; i <= rightBound; ++i) {
            auto* leftSection = layer->m_sections[i];
            if (!leftSection) continue;
            int topBound = std::min(layer->m_topSectionIndex, static_cast<int>(leftSection->size()) - 1);
            auto* sizeRow = layer->m_sectionSizes[i];
            for (int j = layer->m_bottomSectionIndex; j <= topBound; ++j) {
                auto* section = leftSection->at(j);
                if (!section) continue;
                int sectionSize = sizeRow->at(j);
                auto* data = section->data();
                for (int k = 0; k < sectionSize; ++k) {
                    auto* obj = data[k];
                    if (!obj) continue;

                    if (obj->m_objectType == GameObjectType::Decoration || !obj->m_isActivated || obj->m_isGroupDisabled)
                        continue;

                    switch (obj->m_objectType) {
                        default: {
                            if (obj == layer->m_player1 || obj == layer->m_player2) break;
                            bool isSpeedPortal = obj->m_objectID == 200 || obj->m_objectID == 201 || obj->m_objectID == 202 || obj->m_objectID == 203 || obj->m_objectID == 1334;
                            if (obj->m_objectType == GameObjectType::Modifier && !isSpeedPortal) {
                                if (static_cast<EffectGameObject*>(obj)->m_isTouchTriggered)
                                    drawOrientedOrRect(layer, drawNode, obj, getFillColor(triggersColor), size, triggersColor);
                                break;
                            }
                            drawOrientedOrRect(layer, drawNode, obj, getFillColor(otherColor), size, otherColor);
                            break;
                        }
                        case GameObjectType::Solid: {
                            bool p = obj->m_isPassable;
                            drawRect(drawNode, obj->getObjectRect(), getFillColor(p ? passableColor : solidColor), size, p ? passableColor : solidColor);
                            break;
                        }
                        case GameObjectType::Slope: {
                            auto rect = obj->getObjectRect();
                            cocos2d::CCPoint verts[3] = {
                                {rect.getMinX(), rect.getMinY()},
                                {rect.getMinX(), rect.getMaxY()},
                                {rect.getMaxX(), rect.getMinY()},
                            };

                            cocos2d::CCPoint const tr{rect.getMaxX(), rect.getMaxY()};
                            switch (obj->m_slopeDirection) {
                                case 0: case 7: verts[1] = tr; break;
                                case 1: case 5: verts[0] = tr; break;
                                case 3: case 6: verts[2] = tr; break;
                                default: break;
                            }
                            
                            bool const p = obj->m_isPassable;
                            drawNode->drawPolygon(verts, 3, getFillColor(p ? passableColor : solidColor), size, p ? passableColor : solidColor);
                            
                            break;
                        }
                        case GameObjectType::AnimatedHazard:
                        case GameObjectType::Hazard: {
                            if (obj == layer->m_anticheatSpike) break;
                            float const radius = std::max(obj->m_scaleX, obj->m_scaleY) * obj->m_objectRadius;

                            if (radius > 0)
                                drawNode->drawCircle(obj->getPosition(), radius, getFillColor(dangerColor), size, dangerColor, 16);
                            else
                                drawOrientedOrRect(layer, drawNode, obj, getFillColor(dangerColor), size, dangerColor);

                            break;
                        }
                        case GameObjectType::CollisionObject: break;
                    }
                }
            }
        }
    }

    auto drawPlayer = [&](PlayerObject* player) {
        if (!player) return;
        if (auto* ob = player->m_orientedBox)
            drawNode->drawPolygon(ob->m_corners.data(), 4, getFillColor(playerRotateColor), size, playerRotateColor);

        drawRect(drawNode, player->getObjectRect(), getFillColor(playerColor), size, playerColor);
        drawRect(drawNode, player->getObjectRect(0.3f, 0.3f), getFillColor(playerInnerColor), size, playerInnerColor);
    };

    drawPlayer(layer->m_player1);
    if (layer->m_gameState.m_isDualMode) drawPlayer(layer->m_player2);
}

class $modify(ShowHitboxesPlayLayer, PlayLayer) {
    struct Fields {
        cocos2d::CCDrawNode* m_drawNode = nullptr;
    };

    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();
        m_fields->m_drawNode = createDrawNode(this, m_debugDrawNode, "hitboxes"_spr);
    }

    void updateVisibility(float dt) override {
        PlayLayer::updateVisibility(dt);
        updateHitboxes(this, m_fields->m_drawNode);
    }
};

class $modify(ShowHitboxesLevelEditorLayer, LevelEditorLayer) {
    struct Fields {
        cocos2d::CCDrawNode* m_drawNode = nullptr;
    };

    bool init(GJGameLevel* level, bool noUI) {
        if (!LevelEditorLayer::init(level, noUI)) return false;
        m_fields->m_drawNode = createDrawNode(this, m_debugDrawNode, "hitboxes-editor"_spr);
        return true;
    }

    void updateVisibility(float dt) override {
        LevelEditorLayer::updateVisibility(dt);
        updateHitboxes(this, m_fields->m_drawNode);
    }
};

$execute {
    auto& config = Config::get();
    auto& gui = GDH::Gui::get();
    auto& hack = gui.getWindow("Level").findHackByName("Show Hitboxes");   

    hack.setCustomWindowImGui([
        onDeathKey = hack.formatAdditionalSetting("show_on_death"),
        sizeKey = hack.formatAdditionalSetting("size"),
        fillAlphaKey = hack.formatAdditionalSetting("fill_alpha"),

        cTriggersKey = hack.formatAdditionalSetting("triggersColor"),
        cOtherKey = hack.formatAdditionalSetting("otherColor"),
        cPassableKey = hack.formatAdditionalSetting("passableColor"),
        cSolidKey = hack.formatAdditionalSetting("solidColor"),
        cDangerKey = hack.formatAdditionalSetting("dangerColor"),
        cPlayerRotateKey = hack.formatAdditionalSetting("playerRotateColor"),
        cPlayerInnerKey = hack.formatAdditionalSetting("playerInnerColor"),
        cPlayerKey = hack.formatAdditionalSetting("playerColor")
    ]{
        ImGuiWidgetConfig::Checkbox("Show on Death", onDeathKey, false);
        ImGuiWidgetConfig::DragFloat("##Hitbox_Size", sizeKey, 0.05f, 0, 20.f, 0.5f, "Size: %.2f");
        ImGuiWidgetConfig::DragFloat("##Hitbox_FillAlpha", fillAlphaKey, 0.01f, 0, 1.f, 0.2f, "Fill Alpha: %.2f");

        ImGui::Separator();
        ImGui::Text("Hitbox Colors:");

        ImGuiWidgetConfig::ColorEdit4Hex("Triggers", cTriggersKey, "#FF00E6FF");
        ImGuiWidgetConfig::ColorEdit4Hex("Other Objects", cOtherKey, "#00FF00FF");
        ImGuiWidgetConfig::ColorEdit4Hex("Passable Blocks", cPassableKey, "#00FFFFFF");
        ImGuiWidgetConfig::ColorEdit4Hex("Solid (Blocks)", cSolidKey, "#003FFFFF");
        ImGuiWidgetConfig::ColorEdit4Hex("Danger (Spikes)", cDangerKey, "#FF0000FF");
        
        ImGui::Separator();
        ImGui::Text("Player:");
        ImGuiWidgetConfig::ColorEdit4Hex("Player Rotated Box", cPlayerRotateKey, "#800000FF");
        ImGuiWidgetConfig::ColorEdit4Hex("Player Inner Box", cPlayerInnerKey, "#003FFFFF");
        ImGuiWidgetConfig::ColorEdit4Hex("Player Box", cPlayerKey, "#FF0000FF");
    });
}