#include "recorder.hpp"
#include "audio_recorder.hpp"

#ifdef GEODE_IS_WINDOWS
#include "subprocess.hpp"
#include "../windowsUtils.hpp"
#endif

using namespace GDH;

void Recorder::start() {
    if (is_recording) return;
    
    is_recording = true;

    #ifdef GEODE_IS_WINDOWS
    if (native_mode)
        GDH::WindowUtils::setWindowBorderless(true, width, height);
    #endif

    RecorderAudio::get().start();

    last_frame_time = extra_time = 0;
    after_end_extra_time = 0;

    frame_has_data = false;
    current_frame.resize(width * height * 3, 0);
    texture.init(width, height);

    auto view = cocos2d::CCEGLView::get();

    oldDesignResolution = view->getDesignResolutionSize();
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    newDesignResolution = cocos2d::CCSize(roundf(320.f * aspectRatio), 320.f);

    originalScreenScale = cocos2d::CCSize(view->m_fScaleX, view->m_fScaleY);
    newScreenScale = cocos2d::CCSize(static_cast<float>(width) / newDesignResolution.width, static_cast<float>(height) / newDesignResolution.height);

    #ifdef GEODE_IS_WINDOWS
    std::thread([&, command = command] {
        auto process = subprocess::Popen(command);
        while (is_recording || frame_has_data) {
            std::unique_lock<std::mutex> u_lock(lock);
            cv.wait(u_lock, [&] { return frame_has_data || !is_recording; });

            if (frame_has_data) {
                const auto frame = current_frame;
                frame_has_data = false;
                u_lock.unlock();
                cv.notify_one();
                process.m_stdin.write(frame.data(), frame.size());
            } else {
                u_lock.unlock();
            }
        }
        if (process.close()) {
            return;
        }
    }).detach();
    #endif
}

void Recorder::stop() {
    if (!is_recording) return;

    texture.cleanup();

    #ifdef GEODE_IS_WINDOWS
    if (native_mode)
        GDH::WindowUtils::setWindowBorderless(false);
    #endif

    needRevertOld = true;
    
    RecorderAudio::get().stop();

    {
        std::lock_guard<std::mutex> u_lock(lock);
        is_recording = false;
        enabled = false;
    }
    cv.notify_all();
}

void Recorder::capture_frame() {
    std::unique_lock<std::mutex> u_lock(lock);
    cv.wait(u_lock, [&] { return !frame_has_data; });
    u_lock.unlock();

    texture.capture(PlayLayer::get(), lock, cv, current_frame, frame_has_data);
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
            extra_time = time - frame_dt;
            last_frame_time = playLayer->m_gameState.m_levelTime;
            
            RecorderAudio::get().advance_target_time(frame_dt);
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

void Recorder::resize_resolution(geode::Function<void()> action) {
    auto view = cocos2d::CCEGLView::get();
    auto director = cocos2d::CCDirector::get();

    if (!is_recording || (PlayLayer::get() && PlayLayer::get()->m_isPaused)) {
        action();
        return;
    }

    cocos2d::CCSize oldFrameSize = view->getFrameSize();
    
    if (newDesignResolution.width != 0 && newDesignResolution.height != 0) {    
        cocos2d::CCDirector::get()->m_obWinSizeInPoints = newDesignResolution;        
        auto view = cocos2d::CCEGLView::get();
        view->setDesignResolutionSize(newDesignResolution.width, newDesignResolution.height, ResolutionPolicy::kResolutionExactFit);
        view->m_fScaleX = newScreenScale.width;
        view->m_fScaleY = newScreenScale.height;
    }

    action();

    if (oldDesignResolution.width != 0 && oldDesignResolution.height != 0) {
        cocos2d::CCDirector::get()->m_obWinSizeInPoints = oldDesignResolution;
        auto view = cocos2d::CCEGLView::get();
        view->setDesignResolutionSize(oldDesignResolution.width, oldDesignResolution.height, ResolutionPolicy::kResolutionExactFit);
        view->m_fScaleX = originalScreenScale.width;
        view->m_fScaleY = originalScreenScale.height;
    }
}