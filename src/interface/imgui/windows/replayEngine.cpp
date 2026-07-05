#ifdef GEODE_IS_WINDOWS
#include <Geode/Geode.hpp>
#include "imgui.h"
#include <imgui_stdlib.h>
#include <imgui-cocos.hpp>
#include "../../../core/gui.hpp"
#include "../../../core/replayEngine.hpp"
#include "../../../core/config.hpp"
#include "../widgetH.hpp"
#include "../layout.hpp"

$execute {
    auto& gui = GDH::Gui::get();
    auto& config = Config::get();
    auto& layout = GDH::Layout::Manager::get();
    auto& window = gui.getWindow("Replay Engine");
    auto& engine = GDH::ReplayEngine::get();

    window.setCustomWindowImGui([&config, &gui, &layout, &engine] {
        using namespace GDH;

        static std::vector<std::filesystem::path> replay_list;

        if (ImGuiH::RadioButton("Disable", engine.mode == state::disable)) {
            engine.mode = state::disable;
        }
        ImGui::SameLine();
        
        
        if (ImGuiH::RadioButton("Record", engine.mode == state::record)) {
            if (engine.mode != state::record)
                engine.clear();

            auto& practiceFix = gui.getWindow("Level").findHackByName("Practice Fix");   
            if (!practiceFix.getEnabled()) practiceFix.toggle();
            
            auto& tps_bypass = gui.getWindow("Invisible").findHackByName("TPS");       
            if (!tps_bypass.getEnabled()) tps_bypass.toggle();

            auto& respawn_lag = gui.getWindow("Level").findHackByName("Respawn Lag Fix");       
            if (!respawn_lag.getEnabled()) respawn_lag.toggle();
            
            engine.mode = state::record;
        }
        ImGui::SameLine();
        
        if (ImGuiH::RadioButton("Play", engine.mode == state::play)) {
            engine.mode = state::play;
        }
        
        ImGui::Separator();
        
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - layout.multipleScale(35.f));
        ImGui::InputTextWithHint("##replay_name", "Enter the macro name", &engine.replay_name);
        ImGui::SameLine();

        if (ImGuiH::ArrowButton("##replay_select", ImGuiDir_Down)) {
            replay_list.clear();
            for (const auto& entry : std::filesystem::directory_iterator(folderMacroPath)) {
                if (entry.is_regular_file()) {
                    if (entry.path().extension() == ".re4") {
                        replay_list.push_back(entry);
                    }
                }
            }
            ImGui::OpenPopup("Select Replay");
        }
                
        if (ImGuiH::Button("Save", {ImGui::GetContentRegionAvail().x / 3, NULL})) {
            ImGuiH::AddPopup(engine.save(engine.replay_name));
        }
        ImGui::SameLine();
        
        if (ImGuiH::Button("Load", {ImGui::GetContentRegionAvail().x / 2, NULL})) {
            ImGuiH::AddPopup(engine.load(engine.replay_name));
        }
        ImGui::SameLine();
        
        if (ImGuiH::Button("Clear", {ImGui::GetContentRegionAvail().x, NULL})) {
            engine.clear();
            ImGuiH::AddPopup("Replay has been cleared");
        }

        ImGui::Text("Replay Size: %zu/%zu", engine.get_current_index(), engine.get_actions_size());
        ImGui::Text("Frame: %llu", engine.get_frame());

        ImGui::Separator();
        ImGui::Selectable("More Settings");

        if (ImGui::BeginPopupModal("Select Replay", 0, ImGuiWindowFlags_NoResize)) {
            ImGuiH::GlowWindow(IM_COL32(120, 100, 240, 100), IM_COL32(120, 100, 240, 0), layout.multipleScale(250.f));
            ImGui::BeginChild("Replay List", {layout.multipleScale(400.f), layout.multipleScale(500.f)});
            for (const auto& replay : replay_list)
            {
                std::string filename_str = replay.filename().string();
                
                if (ImGuiH::Button(filename_str.c_str(), {ImGui::GetContentRegionAvail().x, 0.0f}))
                {
                    engine.replay_name = replay.stem().string();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndChild();

            if (ImGuiH::Button("Open Folder", {ImGui::GetContentRegionAvail().x, NULL})) {
                geode::utils::file::openFolder(folderMacroPath);
            }
            
            if (ImGuiH::Button("Close", {ImGui::GetContentRegionAvail().x, NULL})) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    });
}
#endif
