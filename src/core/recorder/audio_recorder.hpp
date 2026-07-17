#pragma once
#include <Geode/Geode.hpp>
#include <mutex>
#include <vector>
#include <string>
#include <filesystem>

class RecorderAudio {
public:
    static RecorderAudio& get() {
        static RecorderAudio instance;
        return instance;
    }

    RecorderAudio& operator=(const RecorderAudio&) = delete;
    RecorderAudio(const RecorderAudio&) = delete;

    std::string audio_name = "audio.wav";

    bool enabled = false;
    bool is_recording = false;

    float after_end_extra_time;
    float after_end_duration = 3.f;

    std::string extra_args;

    float volume_music = 1.f;
    float volume_sfx = 1.f;

    float old_volume_music;
    float old_volume_sfx;

    double target_audio_time = 0.0;

    void start();
    void stop();
    void advance_target_time(double frame_dt);

    std::vector<float> get_data();
    std::string get_data_size();
    double get_recorded_time();

private:
    RecorderAudio() = default;
    
    FMOD::DSP* m_dsp = nullptr;
    FMOD::ChannelGroup* m_masterGroup = nullptr;
    std::vector<float> m_data;
    std::mutex m_data_mutex;
    
    bool m_master_paused = false; 
    double m_current_audio_time = 0.0;

    void init();
    void save_to_wav(const std::filesystem::path& filename);
};