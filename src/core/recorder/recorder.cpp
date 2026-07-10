#include "recorder.hpp"
#include "subprocess.hpp"
#include "../windowsUtils.hpp"

using namespace GDH;

void Recorder::start() {
    is_recording = true;

    #ifdef GEODE_IS_WINDOWS
    if (native_mode)
        GDH::WindowUtils::setWindowBorderless(true, width, height);
    #endif

    last_frame_time = extra_time = 0;
    after_end_extra_time = 0;

    frame_has_data = false;
    current_frame.resize(width * height * 3, 0);
    texture.init(width, height);

    std::thread([&, command = command] {
        auto process = subprocess::Popen(command);
        while (is_recording || frame_has_data) {
            lock.lock();
            if (frame_has_data) {
                const auto frame = current_frame;
                frame_has_data = false;
                process.m_stdin.write(frame.data(), frame.size());
            }
            lock.unlock();
        }
        if (process.close()) {
            return;
        }
    }).detach();
}

void Recorder::stop() {
    texture.cleanup();

    if (native_mode)
        GDH::WindowUtils::setWindowBorderless(false);

    is_recording = false;
    enabled = false;
}

void Recorder::capture_frame() {
    while (frame_has_data) {}
    texture.capture(PlayLayer::get(), lock, current_frame, frame_has_data);
}

void Recorder::handle_recording(float dt) {
    auto playLayer = PlayLayer::get();
    if (!playLayer->m_hasCompletedLevel || after_end_extra_time < after_end_duration) {
        if (playLayer->m_hasCompletedLevel) {
            after_end_extra_time += dt;
        }

        double frame_dt = 1.0 / static_cast<double>(fps);
        double time = playLayer->m_gameState.m_levelTime + extra_time - last_frame_time;
        if (time >= frame_dt) {
            // playLayer->processActivatedAudioTriggers(playLayer->m_gameState.m_levelTime);
            
            extra_time = time - frame_dt;
            last_frame_time = playLayer->m_gameState.m_levelTime;
            capture_frame();
        }
    }
    else {
        stop();
    } 
}

void Recorder::compile_command() {
    command = fmt::format("ffmpeg.exe -y -f rawvideo -pix_fmt rgb24 -s {}x{} -r {} -i -", width, height, fps);

    if (!codec.empty()) {
        command += fmt::format(" -c:v {}", codec);
    }

    if (!bitrate.empty()) {
        command += fmt::format(" -b:v {}", bitrate);
    }

    if (!extra_args.empty()) {
        command += fmt::format(" {}", extra_args);
    }
    else {
        command += " -pix_fmt yuv420p";
    }
    
    if (!vf_args.empty())
        command += fmt::format(" -vf {}", vf_args);

    command += fmt::format(" -an \"{}\\{}\"", folder, videoName);
}