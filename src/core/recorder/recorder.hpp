#pragma once
#include <Geode/Geode.hpp>
#include "render_textute.hpp"
#include "../config.hpp"

namespace GDH {
    class Recorder {
    public:
        static Recorder& get() {
            static Recorder instance;
            return instance;
        }
    
        Recorder& operator=(const Recorder&) = delete;
        Recorder(const Recorder&) = delete;
    
        std::filesystem::path folder = folderPath / "Showcases";
        std::string videoName = "video.mp4";
        
        bool enabled = false;
        bool is_recording = false;
        bool lock_aspect_ratio = true;

        bool native_mode = false;
    
        int width = 1280;
        int height = 720;
        int fps = 60;
        
        std::string bitrate = "25M";
        std::string codec = "libx264";
        std::string extra_args = "-pix_fmt yuv420p -preset ultrafast";
        std::string vf_args = "vflip,colorspace=all=bt709:iall=bt470bg:fast=1";
        
        std::string command;
        void compile_command();
        
        float after_end_extra_time;
        float after_end_duration = 3.f;
        double last_frame_time, extra_time;
        
        RenderTexture texture;
        std::vector<uint8_t> current_frame;
        bool frame_has_data = false;

        std::mutex lock;

        void start();
        void stop();
        void capture_frame();

        void handle_recording(float dt);
    private:
        Recorder() = default;
    };
}