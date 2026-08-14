#ifdef GEODE_IS_WINDOWS
#include <Geode/Geode.hpp>
#include "imgui.h"
#include <imgui_stdlib.h>
#include <imgui-cocos.hpp>
#include "../../../core/config.hpp"
#include "../../../core/gui.hpp"
#include "../../../core/replayEngine.hpp"
#include "../../../core/config.hpp"
#include "../../../core/utils.hpp"
#include "../../../core/recorder/recorder.hpp"
#include "../../../core/recorder/subprocess.hpp"
#include "../widgetH.hpp"
#include "../widget_helper.hpp"
#include "../layout.hpp"

std::string getFFmpegPath() {
    static std::string cachedPath = "";
    static bool checked = false;

    if (checked) return cachedPath;
    checked = true;

    static geode::Mod* loader = geode::Loader::get()->getLoadedMod("geode.loader");
    bool consoleEnabled = loader->getSettingValue<bool>("show-platform-console");
    GDH::Recorder::get().console = consoleEnabled;

    auto geodePath = geode::dirs::getGameDir() / "ffmpeg.exe";
    if (std::filesystem::exists(geodePath)) {
        cachedPath = geodePath.string();
        return cachedPath;
    }

    subprocess::Popen process("where ffmpeg");
    if (process.close(true) == 0) {
        cachedPath = "ffmpeg";
        return cachedPath;
    }

    cachedPath = "";
    return cachedPath;
}

void savePreset(const std::filesystem::path& path, const GDH::Recorder& recorder) {
    std::ofstream file(path);
    if (!file.is_open()) return;

    file << "width=" << recorder.width << "\n";
    file << "height=" << recorder.height << "\n";
    file << "fps=" << recorder.fps << "\n";
    file << "bitrate=" << recorder.bitrate << "\n";
    file << "codec=" << recorder.codec << "\n";
    file << "extra_args=" << recorder.extra_args << "\n";
    file << "vf_args=" << recorder.vf_args << "\n";
    file << "audio_args=" << recorder.audio_args << "\n";
    file << "record_audio=" << (recorder.record_audio ? 1 : 0) << "\n";
}

void loadPreset(const std::filesystem::path& path, GDH::Recorder& recorder) {
    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        if (key == "width") recorder.width = std::stoi(val);
        else if (key == "height") recorder.height = std::stoi(val);
        else if (key == "fps") recorder.fps = std::stoi(val);
        else if (key == "bitrate") recorder.bitrate = val;
        else if (key == "codec") recorder.codec = val;
        else if (key == "extra_args") recorder.extra_args = val;
        else if (key == "vf_args") recorder.vf_args = val;
        else if (key == "audio_args") recorder.audio_args = val;
        else if (key == "record_audio") recorder.record_audio = (val == "1");
    }
}

void drawRecorderInterface() {
    std::string ffmpegPath = getFFmpegPath();
    std::filesystem::path folder = Config::get().get<std::string>("recorder::path", (folderPath / "Showcases").string());

    if (ffmpegPath.empty()) {
        ImGui::TextColored(ImColor(255, 128, 128).Value, "FFmpeg not found!");
        ImGui::TextWrapped("Please place ffmpeg.exe in your Geometry Dash folder or add it to system PATH to use the recorder");
        
        if (ImGuiH::Button("Download")) {
            geode::utils::web::openLinkInBrowser("https://github.com/AnimMouse/ffmpeg-autobuild/releases/latest");
        } ImGui::SameLine();

        if (ImGuiH::Button("Open GD Folder")) {
            geode::utils::file::openFolder(geode::dirs::getGameDir());
        }
        return;
    }

    auto& layout = GDH::Layout::Manager::get();
    auto& recorder = GDH::Recorder::get();
    if (ImGuiH::Checkbox("Record##Recorder", &recorder.enabled)) {
        if (recorder.enabled) {
            if (GDH::Utils::isOnlyAsciiPath(folder)) {
                recorder.compile_command();
                recorder.start();
            }
            else {
                recorder.enabled = false;
                ImGuiH::AddPopup("Invalid path to the showcase folder. Please remove any Cyrillic characters");
            }
        }
        else {
            recorder.stop();
        }
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##videoname", &recorder.videoName);
    
    ImGuiH::Checkbox("Include Audio", &recorder.record_audio);
    ImGui::SameLine();

    static geode::Mod* loader = geode::Loader::get()->getLoadedMod("geode.loader");
    bool consoleEnabled = loader->getSettingValue<bool>("show-platform-console");
    if (ImGuiH::Checkbox("Show Console (Requires Restart)", &consoleEnabled)) {
        loader->setSettingValue("show-platform-console", consoleEnabled);
    }
    
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
    ImGui::PushItemWidth(layout.multipleScale(120.f));
    ImGui::InputText("Codec", &recorder.codec);

    ImGui::PushItemWidth(layout.multipleScale(450.f));
    ImGui::InputText("Extra Arguments", &recorder.extra_args);

    ImGui::PushItemWidth(layout.multipleScale(450.f));
    ImGui::InputText("VF Args", &recorder.vf_args);

    ImGui::PushItemWidth(layout.multipleScale(450.f));
    ImGui::InputText("Audio Merge Args", &recorder.audio_args);

    ImGui::Spacing();
    ImGui::Text("Built-in Presets");
    ImGui::Separator();

    if (ImGuiH::Button("HD")) { recorder.width = 1280; recorder.height = 720; recorder.fps = 60; recorder.bitrate = "25M"; }   
    ImGui::SameLine();  
    if (ImGuiH::Button("FULL HD")) { recorder.width = 1920; recorder.height = 1080; recorder.fps = 60; recorder.bitrate = "50M"; }   
    ImGui::SameLine();  
    if (ImGuiH::Button("2K")) { recorder.width = 2560; recorder.height = 1440; recorder.fps = 60; recorder.bitrate = "70M"; }   
    ImGui::SameLine();  
    if (ImGuiH::Button("4K")) { recorder.width = 3840; recorder.height = 2160; recorder.fps = 60; recorder.bitrate = "80M"; }   
    ImGui::SameLine();  
    if (ImGuiH::Button("8K")) { recorder.width = 7680; recorder.height = 4320; recorder.fps = 60; recorder.bitrate = "250M"; }

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

    if (ImGuiH::Button("AMD H264")) {
        recorder.codec = "h264_amf";
        recorder.extra_args = "-pix_fmt yuv420p -quality quality";
    } ImGui::SameLine();

    if (ImGuiH::Button("AMD H265")) {
        recorder.codec = "hevc_amf";
        recorder.extra_args = "-pix_fmt yuv420p -quality quality";
    } ImGui::SameLine();

    if (ImGuiH::Button("AMD AV1 (Only RX 7000 Series)")) {
        recorder.codec = "av1_amf";
        recorder.extra_args = "-pix_fmt yuv420p -quality quality";
    }

    ImGui::Spacing();
    ImGui::Text("Custom Presets");
    ImGui::Separator();

    static std::string customPresetName = "";
    static std::vector<std::filesystem::path> presetFiles;
    static bool presetsNeedRefresh = true;

    if (presetsNeedRefresh) {
        presetFiles.clear();
        if (std::filesystem::exists(folderPresetsPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(folderPresetsPath)) {
                if (entry.is_regular_file() && entry.path().extension() == ".preset") {
                    presetFiles.push_back(entry.path());
                }
            }
        }
        presetsNeedRefresh = false;
    }

    ImGui::SetNextItemWidth(layout.multipleScale(220.f));
    ImGui::InputTextWithHint("##custom_preset_name", "Enter preset name", &customPresetName);
    
    ImGui::SameLine();
    
    if (ImGuiH::Button("Save Preset") && !customPresetName.empty()) {
        std::filesystem::create_directories(folderPresetsPath);
        auto path = folderPresetsPath / (customPresetName + ".preset");
        savePreset(path, recorder);
        presetsNeedRefresh = true;
    }

    ImGui::SameLine();

    if (ImGuiH::Button("Load Preset") && !customPresetName.empty()) {
        auto path = folderPresetsPath / (customPresetName + ".preset");
        if (std::filesystem::exists(path)) {
            loadPreset(path, recorder);
        }
    }

    ImGui::SameLine();

    if (ImGuiH::Button("Delete Preset") && !customPresetName.empty()) {
        auto path = folderPresetsPath / (customPresetName + ".preset");
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
            presetsNeedRefresh = true;
        }
    }

    ImGui::Spacing();

    if (!presetFiles.empty()) {
        for (const auto& path : presetFiles) {
            std::string name = path.stem().string();

            if (ImGuiH::Button(name.c_str())) {
                customPresetName = name;
                loadPreset(path, recorder);
            }
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }

    ImGui::Spacing();
    ImGui::Text("Folders");
    ImGui::Separator();

    if (ImGuiH::Button("Open Showcase Folder")) {
        geode::utils::file::openFolder(folder);
    }
    ImGui::SameLine();
    if (ImGuiH::Button("Change Showcase Folder")) {
        geode::async::spawn(
            geode::utils::file::pick(
                geode::utils::file::PickMode::OpenFolder,
                {}
            ),
            [&recorder](geode::utils::file::PickResult res) {
                if (!res) return;
                auto pathOpt = std::move(res).unwrapOr(std::nullopt);
                if (!pathOpt.has_value()) return;

                auto folderPath = std::move(pathOpt).value();
                Config::get().set<std::string>("recorder::path", (folderPath).string());
            }
        );
    } ImGui::SameLine();

    if (ImGuiH::Button("Reset Showcase Folder")) {
        Config::get().set<std::string>("recorder::path", (folderPath / "Showcases").string());
    }

    if (ImGuiH::Button("Open Presets Folder")) {
        geode::utils::file::openFolder(folderPresetsPath);
    }

    ImGui::Text("Showcase Folder: %s", folder.string().c_str());
}

void drawMacroEditor() {
    static int selected_frame_type = 1;
    static int selected_index = -1;
    static int prev_frame_type = selected_frame_type;
    static int target_frame_input = 0;
    static bool request_scroll = false;
    const char* frame_types[] = { "Physic Frames", "Input Frames" };
    if (prev_frame_type != selected_frame_type) {
        selected_index = -1;
        prev_frame_type = selected_frame_type;
        request_scroll = true;
    }
    auto& engine = GDH::ReplayEngine::get();
    if (selected_frame_type == 0) {
        auto& frames = engine.get_physic_frames();
        if (frames.empty()) {
            selected_index = -1;
        } else if (selected_index < 0) {
            selected_index = 0;
        } else if (selected_index >= (int)frames.size()) {
            selected_index = (int)frames.size() - 1;
        }
        if (ImGui::BeginTable("PhysicTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
            ImVec2(ImGui::GetContentRegionAvail().x - 200, 0))) {
            ImGui::TableSetupColumn("Frame");
            ImGui::TableSetupColumn("Player");
            ImGui::TableSetupColumn("X");
            ImGui::TableSetupColumn("Y");
            ImGui::TableSetupColumn("Y Accel");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin((int)frames.size());
            if (request_scroll && selected_index >= 0 && selected_index < (int)frames.size())
                clipper.IncludeItemByIndex(selected_index);

            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    const auto& frame = frames[i];
                    ImGui::TableNextRow();
                    bool is_selected = (selected_index == i);
                    ImGui::TableSetColumnIndex(0);
                    std::string frame_str = std::to_string(frame.frame) + "##pf" + std::to_string(i);
                    if (ImGui::Selectable(frame_str.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                        selected_index = i;
                    }
                    if (is_selected && request_scroll) {
                        ImGui::SetScrollHereY(0.5f);
                        request_scroll = false;
                    }
                    ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(frame.isPlayer2 ? "P2" : "P1");
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%f", frame.x);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%f", frame.y);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%f", frame.y_accel);
                }
            }
            ImGui::EndTable();
        }
    } else {
        auto& frames = engine.get_input_frames();
        if (frames.empty()) {
            selected_index = -1;
        } else if (selected_index < 0) {
            selected_index = 0;
        } else if (selected_index >= (int)frames.size()) {
            selected_index = (int)frames.size() - 1;
        }
        if (ImGui::BeginTable("InputTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
            ImVec2(ImGui::GetContentRegionAvail().x - 200, 0))) {
            ImGui::TableSetupColumn("Frame");
            ImGui::TableSetupColumn("Player");
            ImGui::TableSetupColumn("Button");
            ImGui::TableSetupColumn("Action");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin((int)frames.size());
            if (request_scroll && selected_index >= 0 && selected_index < (int)frames.size())
                clipper.IncludeItemByIndex(selected_index);

            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    const auto& frame = frames[i];
                    ImGui::TableNextRow();
                    bool is_selected = (selected_index == i);
                    ImGui::TableSetColumnIndex(0);
                    std::string frame_str = std::to_string(frame.frame) + "##if" + std::to_string(i);
                    if (ImGui::Selectable(frame_str.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                        selected_index = i;
                    }
                    if (is_selected && request_scroll) {
                        ImGui::SetScrollHereY(0.5f);
                        request_scroll = false;
                    }
                    ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(frame.isPlayer2 ? "P2" : "P1");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(
                        frame.button == 1 ? "Jump" :
                        frame.button == 2 ? "Left" :
                        frame.button == 3 ? "Right" : "Unknown");
                    ImGui::TableSetColumnIndex(3);
                    ImGui::TextUnformatted(frame.down ? "Push" : "Release");
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::SameLine();
    if (ImGui::BeginChild("##EditorSettings", { 0, ImGui::GetContentRegionAvail().y })) {
        ImGui::Text("Frame Type:");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::Combo("##FrameType", &selected_frame_type, frame_types, IM_ARRAYSIZE(frame_types));
        ImGui::Separator();
        ImGui::Text("Go to Frame:");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
        ImGui::InputInt("##TargetFrame", &target_frame_input, 0, 0);
        ImGui::SameLine();
        if (ImGuiH::Button("Go##FindFrame", ImVec2(62.0f, 0))) {
            if (selected_frame_type == 0) {
                auto& frames = engine.get_physic_frames();
                for (size_t i = 0; i < frames.size(); i++) {
                    if (frames[i].frame >= static_cast<uint64_t>(target_frame_input)) {
                        selected_index = static_cast<int>(i);
                        request_scroll = true;
                        break;
                    }
                }
            } else {
                auto& frames = engine.get_input_frames();
                for (size_t i = 0; i < frames.size(); i++) {
                    if (frames[i].frame >= static_cast<uint64_t>(target_frame_input)) {
                        selected_index = static_cast<int>(i);
                        request_scroll = true;
                        break;
                    }
                }
            }
        }
        ImGui::Separator();
        ImGui::Text("Actions:");
        if (selected_frame_type == 0) {
            auto& frames = engine.get_physic_frames();
            bool hasSel = !frames.empty() && selected_index >= 0 && selected_index < (int)frames.size();
            if (ImGuiH::Button("Add Action", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                physic_data new_frame{};
                new_frame.x = 0.0f;
                new_frame.y = 0.0f;
                new_frame.y_accel = 0.0;
                if (hasSel) {
                    new_frame.frame = frames[selected_index].frame + 1;
                    new_frame.isPlayer2 = frames[selected_index].isPlayer2;
                    frames.insert(frames.begin() + selected_index + 1, new_frame);
                    selected_index++;
                } else {
                    new_frame.frame = 0;
                    new_frame.isPlayer2 = false;
                    frames.push_back(new_frame);
                    selected_index = (int)frames.size() - 1;
                }
                request_scroll = true;
            }
            if (hasSel) {
                if (ImGuiH::Button("Remove Action", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    frames.erase(frames.begin() + selected_index);
                    if (frames.empty()) selected_index = -1;
                    else if (selected_index >= (int)frames.size()) selected_index = (int)frames.size() - 1;
                    request_scroll = true;
                }
                ImGui::BeginDisabled(selected_index <= 0);
                if (ImGuiH::Button("Move Up", ImVec2((ImGui::GetContentRegionAvail().x / 2.0f) - 2.0f, 0))) {
                    std::swap(frames[selected_index], frames[selected_index - 1]);
                    selected_index--;
                    request_scroll = true;
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::BeginDisabled(selected_index >= (int)frames.size() - 1);
                if (ImGuiH::Button("Move Down", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    std::swap(frames[selected_index], frames[selected_index + 1]);
                    selected_index++;
                    request_scroll = true;
                }
                ImGui::EndDisabled();
                ImGui::Separator();
            }
            if (hasSel) {
                auto& selected_frame = frames[selected_index];
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                int frame_cast = static_cast<int>(selected_frame.frame);
                if (ImGuiH::DragInt("Frame##Drag", &frame_cast, 1.f, 0, INT_MAX, "Frame: %i"))
                    selected_frame.frame = static_cast<uint64_t>(frame_cast);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGuiH::DragFloat("X##Drag", &selected_frame.x, 0.001f, -FLT_MAX, FLT_MAX, "X Position: %f");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGuiH::DragFloat("Y##Drag", &selected_frame.y, 0.001f, -FLT_MAX, FLT_MAX, "Y Position: %f");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                float yAccel_cast = static_cast<float>(selected_frame.y_accel);
                if (ImGuiH::DragFloat("YAccel##Drag", &yAccel_cast, 0.001f, -FLT_MAX, FLT_MAX, "Y Accel: %f"))
                    selected_frame.y_accel = static_cast<double>(yAccel_cast);
                ImGuiH::Checkbox("Player 2", &selected_frame.isPlayer2);
            } else {
                ImGui::TextColored(ImColor(255, 128, 128).Value, "No physic frames");
            }
        } else {
            auto& frames = engine.get_input_frames();
            bool hasSel = !frames.empty() && selected_index >= 0 && selected_index < (int)frames.size();
            if (ImGuiH::Button("Add Action", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                input_data new_frame{};
                new_frame.button = 1;
                new_frame.down = false;
                if (hasSel) {
                    new_frame.frame = frames[selected_index].frame + 1;
                    new_frame.isPlayer2 = frames[selected_index].isPlayer2;
                    frames.insert(frames.begin() + selected_index + 1, new_frame);
                    selected_index++;
                } else {
                    new_frame.frame = 0;
                    new_frame.isPlayer2 = false;
                    frames.push_back(new_frame);
                    selected_index = (int)frames.size() - 1;
                }
                request_scroll = true;
            }
            if (hasSel) {
                if (ImGuiH::Button("Remove Action", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    frames.erase(frames.begin() + selected_index);
                    if (frames.empty()) selected_index = -1;
                    else if (selected_index >= (int)frames.size()) selected_index = (int)frames.size() - 1;
                    request_scroll = true;
                }
                ImGui::BeginDisabled(selected_index <= 0);
                if (ImGuiH::Button("Move Up", ImVec2((ImGui::GetContentRegionAvail().x / 2.0f) - 2.0f, 0))) {
                    std::swap(frames[selected_index], frames[selected_index - 1]);
                    selected_index--;
                    request_scroll = true;
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::BeginDisabled(selected_index >= (int)frames.size() - 1);
                if (ImGuiH::Button("Move Down", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    std::swap(frames[selected_index], frames[selected_index + 1]);
                    selected_index++;
                    request_scroll = true;
                }
                ImGui::EndDisabled();
                ImGui::Separator();
            }
            if (hasSel) {
                auto& selected_frame = frames[selected_index];
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                int frame_cast = static_cast<int>(selected_frame.frame);
                if (ImGuiH::DragInt("Frame##Drag", &frame_cast, 1, 0, INT_MAX, "Frame: %i"))
                    selected_frame.frame = static_cast<uint64_t>(frame_cast);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGuiH::DragInt("Button##Drag", &selected_frame.button, 1, 1, 3, "Button: %i");
                ImGuiH::Checkbox("Push", &selected_frame.down);
                ImGui::SameLine();
                ImGuiH::Checkbox("Player 2", &selected_frame.isPlayer2);
            } else {
                ImGui::TextColored(ImColor(255, 128, 128).Value, "No input frames");
            }
        }
        ImGui::EndChild();
    }
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
                    engine.setupHacks(true);
                    engine.mode = state::record;
                }
            }
        }
        ImGui::SameLine();
        
        if (ImGuiH::RadioButton("Play", engine.mode == state::play)) {
            engine.setupHacks(false);
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
                        if (GDH::Utils::isOnlyAsciiPath(entry)) 
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

        if (ImGui::BeginPopupModal("Record Overwrite Warning", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
            ImGuiH::GlowWindow(IM_COL32(71, 71, 131, 150), IM_COL32(255, 128, 128, 0), layout.multipleScale(500.f));
            ImGui::Text("You currently have actions stored in the buffer.\nStarting a new recording will clear your current unsaved active macro.\n\nAre you sure you want to proceed?");
            ImGuiH::SpaceSeparator();
            if (ImGuiH::Button("Yes, Overwrite", {ImGui::GetContentRegionAvail().x / 2, 0})) {
                engine.clear();
                engine.setupHacks(true);
                engine.mode = state::record;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGuiH::Button("Cancel", {ImGui::GetContentRegionAvail().x, 0})) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Save Overwrite Warning", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
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

        if (ImGui::BeginPopupModal("Load Overwrite Warning", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
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

        if (ImGui::BeginPopupModal("Clear Warning", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
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

        if (ImGui::BeginPopupModal("Select Replay", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
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
            ImGui::SetNextWindowSize({layout.multipleScale(800.f), layout.multipleScale(655.f)});
        }
        
        if (ImGui::BeginPopupModal("Pro Settings", &isProSettingsOpen)) {
            ImGuiH::GlowWindow(IM_COL32(120, 100, 240, 100), IM_COL32(120, 100, 240, 0), layout.multipleScale(250.f));

            if (ImGui::BeginTabBar("ProSettingsTabBar")) {
                if (ImGui::BeginTabItem("Settings")) {
                    ImGui::Spacing();

                    ImGuiWidgetConfig::Checkbox("Accuracy Fix", "engine::accuracy_fix", true);
                    ImGui::SameLine();
                    ImGuiWidgetConfig::Checkbox("Y Velocity Fix", "engine::velocity_fix", false);
                    ImGui::SameLine();
                    ImGuiWidgetConfig::Checkbox("Ignore Inputs on Playback", "engine::ignore_inputs", true);

                    ImGuiH::SpaceSeparator();
                    ImGui::Text("Replay Engine v3 to Replay Engine v4 Converter");
                    if (ImGui::TextLink("https://tobyadd.pages.dev/GDH/re3_to_re4")) {
                        geode::utils::web::openLinkInBrowser("https://tobyadd.pages.dev/GDH/re3_to_re4");
                    }
                    ImGuiH::SpaceSeparator();
                    if (ImGuiH::Button("Reset TPS Warning Popup")) {
                        config.set<bool>("invisible.tps_warning_dont_show_again", false);
                    }
                    
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Recorder")) {
                    ImGui::Spacing();
                    drawRecorderInterface();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Editor (Beta)")) {
                    ImGui::Spacing();
                    drawMacroEditor();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::EndPopup();
        }
    });
}
#endif