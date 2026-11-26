#include <Geode/Enums.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include "../../core/gui.hpp"

GUI_HACK_CREATE("Cosmetic", "Auto Load Comments", "Loads comments automatically", false);

$execute {
    auto& gui = GDH::Gui::get();
    auto& hack = gui.getWindow("Cosmetic").findHackByName("Auto Load Comments");   
    hack.setGameVariableID(GameVar::AutoLoadComments);
}