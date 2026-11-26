#include <Geode/Enums.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include "../../core/gui.hpp"

GUI_HACK_CREATE("Creator", "Legacy Move Menu", "Uses the classic grid layout for move buttons instead of the d-pad layout", false);

$execute {
    auto& gui = GDH::Gui::get();
    auto& hack = gui.getWindow("Creator").findHackByName("Legacy Move Menu");   
    hack.setGameVariableID(GameVar::LegacyMoveMenu);
}