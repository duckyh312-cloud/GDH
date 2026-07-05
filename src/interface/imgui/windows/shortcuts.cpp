#ifdef GEODE_IS_WINDOWS
#include "imgui.h"
#include <Geode/Geode.hpp>
#include "../../../core/gui.hpp"
#include "../layout.hpp"
#include "../../../core/shortcuts.hpp"
#include "../widgetH.hpp"
#include <imgui-cocos.hpp>
#include "../../cocos/hacks_layer.hpp"

$execute {
    auto& gui = GDH::Gui::get();
    auto& window = gui.getWindow("Shortcuts");

    window.setCustomWindowImGui([] {
        auto& layout = GDH::Layout::Manager::get();
        float width = ImGui::GetContentRegionAvail().x;
        
        if (ImGuiH::Button("Options", {width, 0.f}))
            GDH::Shortcuts::openOptions();

        if (ImGuiH::Button("Graphics Settings", {width, 0.f}))
            GDH::Shortcuts::openGraphicsSettings();

        if (ImGuiH::Button("Quests Page", {width, 0.f}))
            GDH::Shortcuts::openQuests();

        if (ImGuiH::Button("Open Mobile UI", {width, 0.f}))
            HacksLayer::isOpened() ? HacksLayer::get()->onClose(nullptr) : HacksLayer::get()->show();

        if (ImGuiH::Button("Reset Level", {width, 0.f})) 
            GDH::Shortcuts::resetLevel();

        if (ImGuiH::Button("Practice Mode", {width, 0.f}))
            GDH::Shortcuts::togglePracticeMode();

        if (ImGuiH::Button("Reset Volume", {width, 0.f}))
            GDH::Shortcuts::resetVolume();

        if (ImGuiH::Button("Uncomplete Level", {width, 0.f}))
            GDH::Shortcuts::uncompleteLevel();

        if (ImGuiH::Button("Resources", {width/2, 0.f})) {
            GDH::Shortcuts::openResourcesFolder();
        }
        ImGui::SameLine();
        if (ImGuiH::Button("AppData", {ImGui::GetContentRegionAvail().x, 0.f})) {
            GDH::Shortcuts::openAppDataFolder();
        }

        if (ImGuiH::Button("GDH AppData", {width, 0.f})) {
            GDH::Shortcuts::openGDHAppDataFolder();
        }

        if (ImGuiH::Button("Crash Game", {width, 0.f})) {
            ImGui::OpenPopup("Crash the Game");
        }

        if (ImGui::BeginPopupModal("Crash the Game")) {
            ImGuiH::GlowWindow(IM_COL32(71, 71, 131, 150), IM_COL32(255, 128, 128, 0), layout.multipleScale(500.f));
            ImGui::Text("Are you sure you want to crash Geometry Dash?");
            ImGui::Text("Unsaved data will be lost");

            if (ImGuiH::Button("Cancel", {ImGui::GetContentRegionAvail().x, 0})) {
                ImGui::CloseCurrentPopup();
            }

            if (ImGuiH::Button("OK", {ImGui::GetContentRegionAvail().x, 0})) {
                GDH::Shortcuts::triggerCrash();
            }

            ImGui::EndPopup();
        }

    });
}
#endif
