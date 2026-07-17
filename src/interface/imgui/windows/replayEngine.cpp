#ifdef GEODE_IS_WINDOWS
#include <Geode/Geode.hpp>
#include "imgui.h"
#include <imgui_stdlib.h>
#include <imgui-cocos.hpp>
#include "../../../core/gui.hpp"
#include "../../../core/replayEngine.hpp"
#include "../../../core/config.hpp"
#include "../../../core/recorder/recorder.hpp"
#include "../widgetH.hpp"
#include "../widget_helper.hpp"
#include "../layout.hpp"

void drawRecorderInterface() {
    auto& layout = GDH::Layout::Manager::get();
    auto& recorder = GDH::Recorder::get();
    if (ImGuiH::Checkbox("Record##Recorder", &recorder.enabled)) {
        if (recorder.enabled) {
            recorder.compile_command();
            recorder.start();
        }
        else {
            recorder.stop();
        }
    }

    
    ImGui::SameLine();
    
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##videoname", &recorder.videoName);
    
    ImGuiH::Checkbox("Native Mode", &recorder.native_mode);
    
    ImGui::Text("Resolution:");
    ImGui::Separator();

    ImGui::PushItemWidth(layout.multipleScale(45.f));
    if (ImGui::InputInt("##width", &recorder.width, 0) && recorder.lock_aspect_ratio) {
        float aspect_ratio = 16.0f / 9.0f;
        recorder.height = static_cast<int>(recorder.width / aspect_ratio);
    }
    ImGui::SameLine(0, 5);

    ImGui::Text("x");
    ImGui::SameLine(0, 5);

    ImGui::PushItemWidth(layout.multipleScale(45.f));
    if (ImGui::InputInt("##height", &recorder.height, 0) && recorder.lock_aspect_ratio) {
        float aspect_ratio = 16.0f / 9.0f;
        recorder.width = static_cast<int>(recorder.height * aspect_ratio);
    }
    ImGui::SameLine(0, 5);

    ImGui::Text("@");
    ImGui::SameLine(0, 5);

    ImGui::PushItemWidth(layout.multipleScale(35.f));
    ImGui::InputInt("##fps", &recorder.fps, 0);

    ImGui::SameLine(0, 5);

    ImGuiH::Checkbox("Lock Aspect Ratio (16:9)", &recorder.lock_aspect_ratio);

    ImGui::Spacing();

    ImGui::Text("Encoding Settings");
    ImGui::Separator(); 

    ImGui::PushItemWidth(layout.multipleScale(50.f));
    ImGui::InputText("Bitrate", &recorder.bitrate); 

    ImGui::SameLine();

    ImGui::PushItemWidth(layout.multipleScale(80.f));
    ImGui::InputText("Codec", &recorder.codec);

    ImGui::PushItemWidth(layout.multipleScale(450.f));
    ImGui::InputText("Extra Arguments", &recorder.extra_args);

    ImGui::PushItemWidth(layout.multipleScale(450.f));
    ImGui::InputText("VF Args", &recorder.vf_args);

    ImGui::Spacing();
    ImGui::Text("Presets");
    ImGui::Separator();

    if (ImGuiH::Button("HD")) { recorder.width = 1280; recorder.height = 720; recorder.fps = 60; recorder.bitrate = "25M"; }   
    ImGui::SameLine();  
    if (ImGuiH::Button("FULL HD")) { recorder.width = 1920; recorder.height = 1080; recorder.fps = 60; recorder.bitrate = "50M"; }   
    ImGui::SameLine();  
    if (ImGuiH::Button("2K")) { recorder.width = 2560; recorder.height = 1440; recorder.fps = 60; recorder.bitrate = "70M"; }   
    ImGui::SameLine();  
    if (ImGuiH::Button("4K")) { recorder.width = 3840; recorder.height = 2160; recorder.fps = 60; recorder.bitrate = "80M"; }   
    ImGui::SameLine();  
    if (ImGuiH::Button("8K")) { recorder.width = 7680; recorder.height = 4320; recorder.fps = 60; recorder.bitrate = "250M";}

    if (ImGuiH::Button("CPU x264")) {
        recorder.codec = "libx264";
        recorder.extra_args = "-pix_fmt yuv420p -preset ultrafast";
    } ImGui::SameLine();  

    if (ImGuiH::Button("CPU x265")) {
        recorder.codec = "libx265";
        recorder.extra_args = "-pix_fmt yuv420p -preset ultrafast";
    } ImGui::SameLine();

    if (ImGuiH::Button("CPU AV1 Lossless")) {
        recorder.codec = "libsvtav1";
        recorder.extra_args = "-crf 0 -pix_fmt yuv420p";
    }

    if (ImGuiH::Button("NVIDIA H264")) {
        recorder.codec = "h264_nvenc";
        recorder.extra_args = "-pix_fmt yuv420p -preset p7";
    } ImGui::SameLine();

    if (ImGuiH::Button("NVIDIA H265")) {
        recorder.codec = "hevc_nvenc";
        recorder.extra_args = "-pix_fmt yuv420p -preset p7";
    } ImGui::SameLine();
    
    if (ImGuiH::Button("NVIDIA AV1 (Only RTX40 Series)")) {
        recorder.codec = "av1_nvenc";
        recorder.extra_args = "-pix_fmt yuv420p -preset p7";
    }

    ImGui::Spacing();
    ImGui::Text("Folders");
    ImGui::Separator();

    if (ImGuiH::Button("Open Showcase Folder")) {
        geode::utils::file::openFolder(recorder.folder);
    }
    ImGui::Text("Showcase Folder: %s", recorder.folder.string().c_str());
}

$execute {
    auto& gui = GDH::Gui::get();
    auto& config = Config::get();
    auto& layout = GDH::Layout::Manager::get();
    auto& window = gui.getWindow("Replay Engine");
    auto& engine = GDH::ReplayEngine::get();

    window.setCustomWindowImGui([&config, &gui, &layout, &engine] {
        using namespace GDH;

        static std::vector<std::filesystem::path> replay_list;
        static bool isProSettingsOpen = true;

        auto enableHackIfDisabled = [&](const char* windowName, const char* hackName) {
            auto& hack = gui.getWindow(windowName).findHackByName(hackName);
            if (!hack.getEnabled()) {
                hack.toggle();
            }
        };

        if (ImGuiH::RadioButton("Disable", engine.mode == state::disable)) {
            engine.mode = state::disable;
        }
        ImGui::SameLine();
                
        if (ImGuiH::RadioButton("Record", engine.mode == state::record)) {
            if (engine.mode != state::record) {
                if (engine.get_actions_size() > 0) {
                    ImGui::OpenPopup("Record Overwrite Warning");
                } else {
                    engine.clear();
                    enableHackIfDisabled("Level", "Practice Fix");
                    enableHackIfDisabled("Invisible", "TPS");
                    enableHackIfDisabled("Level", "Respawn Lag Fix");
                    engine.mode = state::record;
                }
            }
        }
        ImGui::SameLine();
        
        if (ImGuiH::RadioButton("Play", engine.mode == state::play)) {
            enableHackIfDisabled("Invisible", "TPS");
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
            std::filesystem::path savePath = folderMacroPath / (engine.replay_name + ".re4");
            if (std::filesystem::exists(savePath)) {
                ImGui::OpenPopup("Save Overwrite Warning");
            } else {
                ImGuiH::AddPopup(engine.save(engine.replay_name));
            }
        }
        ImGui::SameLine();
        
        if (ImGuiH::Button("Load", {ImGui::GetContentRegionAvail().x / 2, NULL})) {
            if (engine.get_actions_size() > 0) {
                ImGui::OpenPopup("Load Overwrite Warning");
            } else {
                ImGuiH::AddPopup(engine.load(engine.replay_name));
            }
        }
        ImGui::SameLine();
        
        if (ImGuiH::Button("Clear", {ImGui::GetContentRegionAvail().x, NULL})) {
            if (engine.get_actions_size() > 0) {
                ImGui::OpenPopup("Clear Warning");
            } else {
                engine.clear();
                ImGuiH::AddPopup("Replay has been cleared");
            }
        }

        ImGui::Text("Replay Size: %zu/%zu", engine.get_current_index(), engine.get_actions_size());
        ImGui::Text("Frame: %llu", engine.get_frame());

        ImGui::Separator();
        if (ImGui::Selectable("Pro Settings")) {
            isProSettingsOpen = true;
            ImGui::OpenPopup("Pro Settings");
        }

        if (ImGui::BeginPopupModal("Record Overwrite Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGuiH::GlowWindow(IM_COL32(71, 71, 131, 150), IM_COL32(255, 128, 128, 0), layout.multipleScale(500.f));
            ImGui::Text("You currently have actions stored in the buffer.\nStarting a new recording will clear your current unsaved active macro.\n\nAre you sure you want to proceed?");
            ImGuiH::SpaceSeparator();
            if (ImGuiH::Button("Yes, Overwrite", {ImGui::GetContentRegionAvail().x / 2, 0})) {
                engine.clear();
                enableHackIfDisabled("Level", "Practice Fix");
                enableHackIfDisabled("Invisible", "TPS");
                enableHackIfDisabled("Level", "Respawn Lag Fix");
                engine.mode = state::record;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGuiH::Button("Cancel", {ImGui::GetContentRegionAvail().x, 0})) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Save Overwrite Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGuiH::GlowWindow(IM_COL32(71, 71, 131, 150), IM_COL32(255, 128, 128, 0), layout.multipleScale(500.f));
            ImGui::Text("A macro named \"%s.re4\" already exists.\nAre you sure you want to overwrite it?", engine.replay_name.c_str());
            ImGuiH::SpaceSeparator();
            if (ImGuiH::Button("Yes, Overwrite", {ImGui::GetContentRegionAvail().x / 2, 0})) {
                ImGuiH::AddPopup(engine.save(engine.replay_name));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGuiH::Button("Cancel", {ImGui::GetContentRegionAvail().x, 0 })) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Load Overwrite Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGuiH::GlowWindow(IM_COL32(71, 71, 131, 150), IM_COL32(255, 128, 128, 0), layout.multipleScale(500.f));
            ImGui::Text("You currently have recorded actions actively loaded.\nLoading this macro will clear your unsaved working progress.\n\nAre you sure you want to proceed?");
            ImGuiH::SpaceSeparator();
            if (ImGuiH::Button("Yes, Load", {ImGui::GetContentRegionAvail().x / 2, 0})) {
                ImGuiH::AddPopup(engine.load(engine.replay_name));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGuiH::Button("Cancel", {ImGui::GetContentRegionAvail().x, 0})) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Clear Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGuiH::GlowWindow(IM_COL32(71, 71, 131, 150), IM_COL32(255, 128, 128, 0), layout.multipleScale(500.f));
            ImGui::Text("Are you sure you want to completely clear the current macro?\nThis action cannot be undone.");
            ImGuiH::SpaceSeparator();
            if (ImGuiH::Button("Yes, Clear", {ImGui::GetContentRegionAvail().x / 2, 0})) {
                engine.clear();
                ImGuiH::AddPopup("Replay has been cleared");
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGuiH::Button("Cancel", {ImGui::GetContentRegionAvail().x, 0})) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

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

        static bool onceResize = false;
        if (!onceResize && ImGui::IsPopupOpen("Pro Settings")) {
            onceResize = true;
            ImGui::SetNextWindowSize({layout.multipleScale(800.f), layout.multipleScale(540.f)});
        }
        
        if (ImGui::BeginPopupModal("Pro Settings", &isProSettingsOpen)) {
            ImGuiH::GlowWindow(IM_COL32(120, 100, 240, 100), IM_COL32(120, 100, 240, 0), layout.multipleScale(250.f));

            if (ImGui::BeginTabBar("ProSettingsTabBar")) {
                if (ImGui::BeginTabItem("Settings")) {
                    ImGui::Spacing();

                    ImGuiWidgetConfig::Checkbox("Accuracy Fix", "engine::accuracy_fix", true);
                    ImGui::SameLine();
                    ImGuiWidgetConfig::Checkbox("Y Velocity Fix", "engine::velocity_fix", false);
                    ImGuiH::SpaceSeparator();

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Recorder")) {
                    ImGui::Spacing();
                    drawRecorderInterface();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::EndPopup();
        }
    });
}
#endif