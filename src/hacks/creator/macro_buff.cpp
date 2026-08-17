#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../../core/gui.hpp"

GUI_HACK_CREATE("Creator", "Macro Buff", "Auto-places spikes on jump. Highly experimental. Works only on pure jumps! Pads and gravity shifts break the structure. Cube and Robot only", false);

class $modify(MacroBuffGJBaseGameLayer, GJBaseGameLayer) {
    static void onModify(auto& self) {
        auto& gui = GDH::Gui::get();
        auto& hack = gui.getWindow("Creator").findHackByName("Macro Buff");        
        
        hack.addHookPtr(self.getHook("GJBaseGameLayer::processCommands").unwrap());
    }

    struct Fields {
        bool cached = false;
        CCPoint spikeOffset;
        CCSize spikeSize;
    };

    void cacheSpike() {
        if (m_fields->cached) return;
        auto lel = LevelEditorLayer::get();
        if (!lel) return;

        auto temp = lel->createObject(8, CCPoint(0.f, 0.f), true);
        if (!temp) return;

        CCRect r = temp->getObjectRect();
        m_fields->spikeOffset = temp->getPosition() - r.origin;
        m_fields->spikeSize = r.size;

        lel->removeObject(temp, true);
        m_fields->cached = true;
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        auto lel = LevelEditorLayer::get();
        if (!lel || !m_player1) return;
        if (m_player1->m_isOnGround) return;

        cacheSpike();
        if (!m_fields->cached) return;

        CCRect playerRect = m_player1->getObjectRect();
        float vy = m_player1->m_yVelocity;

        auto spawnSpikeAt = [&](float originX, float originY) {
            CCPoint targetOrigin(originX, originY);
            CCPoint finalPos = targetOrigin + m_fields->spikeOffset;
            lel->createObject(8, finalPos, true);
        };

        if (vy >= 0.f) {
            // top left
            spawnSpikeAt(
                playerRect.origin.x - m_fields->spikeSize.width - 0.1f,
                playerRect.origin.y + playerRect.size.height + 0.1f
            );
        } else {
            // top right
            spawnSpikeAt(
                playerRect.origin.x + playerRect.size.width + 0.1f,
                playerRect.origin.y + playerRect.size.height + 0.1f
            );
        }

        if (std::abs(vy) >= 5.0f) {
            if (vy >= 0.f) {
                // bottom right
                spawnSpikeAt(
                     playerRect.origin.x + playerRect.size.width + 1.f,
                     playerRect.origin.y - m_fields->spikeSize.height - 1.f
                );
            } else {
                // bottom left
                spawnSpikeAt(
                    playerRect.origin.x - m_fields->spikeSize.width - 0.1f,
                    playerRect.origin.y - m_fields->spikeSize.height - 0.1f
                );
            }
        }
    }
};