#include <Geode/Enums.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include "../../core/gui.hpp"

GUI_HACK_CREATE("Cosmetic", "Legacy Pause Progress", "Shows full Normal and Practice Mode progress bars in the pause menu instead of active session only", false);

$execute {
    auto& gui = GDH::Gui::get();
    auto& hack = gui.getWindow("Cosmetic").findHackByName("Legacy Pause Progress");   
    hack.setGameVariableID(GameVar::LegacyPauseProgress);
}