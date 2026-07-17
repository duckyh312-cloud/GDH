#include "audio_recorder.hpp"
#include "recorder.hpp"
#include <fstream>

void RecorderAudio::init() {
    auto fmod_engine = FMODAudioEngine::get();
    auto system = fmod_engine->m_system;

    system->getMasterChannelGroup(&m_masterGroup);

    FMOD_DSP_DESCRIPTION desc = {};
    strcpy(desc.name, "GDH DSP Recorder");
    desc.numinputbuffers = 1;
    desc.numoutputbuffers = 1;
    desc.read = [](FMOD_DSP_STATE* dspState, float* inBuffer, float* outBuffer, unsigned int length, int inChannels, int* outChannels) {
        auto& recorder = RecorderAudio::get();
        auto pl = PlayLayer::get();
        
        std::memcpy(outBuffer, inBuffer, length * (*outChannels) * sizeof(float));

        if (!recorder.is_recording || !pl || !pl->m_started) return FMOD_OK;

        int sampleRate = 0, channels = 0;
        FMODAudioEngine::get()->m_system->getSoftwareFormat(&sampleRate, nullptr, &channels);

        size_t current_size = 0;
        {
            std::lock_guard<std::mutex> lock(recorder.m_data_mutex);
            recorder.m_data.insert(recorder.m_data.end(), inBuffer, inBuffer + length * (*outChannels));
            current_size = recorder.m_data.size();
            
            recorder.m_current_audio_time = static_cast<double>(current_size) / (static_cast<double>(sampleRate) * static_cast<double>(*outChannels));
        }

        double recorded_audio_time = recorder.m_current_audio_time;

        if (recorded_audio_time >= recorder.target_audio_time) {
            if (recorder.m_masterGroup) {
                recorder.m_masterGroup->setPaused(true);
                recorder.m_master_paused = true;
            }
        }

        return FMOD_OK;
    };

    system->createDSP(&desc, &m_dsp);
}

void RecorderAudio::start() {
    if (is_recording) return;

    init();
    
    is_recording = true;
    m_master_paused = false;
    after_end_extra_time = 0;
    target_audio_time = 0.0;
    m_current_audio_time = 0.0;

    auto fmod_engine = FMODAudioEngine::get();
    old_volume_music = fmod_engine->getBackgroundMusicVolume();
    old_volume_sfx = fmod_engine->getEffectsVolume();

    fmod_engine->setBackgroundMusicVolume(volume_music);
    fmod_engine->setEffectsVolume(volume_sfx);

    if (m_masterGroup && m_dsp) {
        m_masterGroup->addDSP(0, m_dsp);
    }

    {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        m_data.clear();
        m_data.reserve(44100 * 2 * 10);
    }
}

void RecorderAudio::stop() {
    if (!is_recording) return;

    enabled = false;
    is_recording = false;

    if (m_masterGroup) {
        if (m_dsp) {
            m_masterGroup->removeDSP(m_dsp);
        }
        m_masterGroup->setPaused(false); 
    }
    m_master_paused = false;

    auto fmod_engine = FMODAudioEngine::get();
    fmod_engine->setBackgroundMusicVolume(old_volume_music);
    fmod_engine->setEffectsVolume(old_volume_sfx);

    save_to_wav(GDH::Recorder::get().folder / audio_name);
}

std::vector<float> RecorderAudio::get_data() {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    auto data = std::move(m_data);
    m_data.clear();
    return data;
}

void RecorderAudio::advance_target_time(double frame_dt) {
    if (!is_recording || !m_masterGroup) return;

    target_audio_time += frame_dt;

    if (m_master_paused) {
        m_masterGroup->setPaused(false);
        m_master_paused = false;
    }
}

void RecorderAudio::save_to_wav(const std::filesystem::path& filename) {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    if (m_data.empty()) return;

    int sampleRate = 0, channels = 0;
    FMODAudioEngine::get()->m_system->getSoftwareFormat(&sampleRate, nullptr, &channels);
    if (sampleRate == 0 || channels == 0) return;

    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) return;

    struct WavHeader {
        char chunkId[4] = {'R', 'I', 'F', 'F'};
        uint32_t chunkSize;
        char format[4] = {'W', 'A', 'V', 'E'};
        char subchunk1Id[4] = {'f', 'm', 't', ' '};
        uint32_t subchunk1Size = 16;
        uint16_t audioFormat = 3;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample = 32;
        char subchunk2Id[4] = {'d', 'a', 't', 'a'};
        uint32_t subchunk2Size;
    };

    WavHeader header;
    header.numChannels = static_cast<uint16_t>(channels);
    header.sampleRate = static_cast<uint32_t>(sampleRate);
    header.byteRate = header.sampleRate * header.numChannels * sizeof(float);
    header.blockAlign = header.numChannels * sizeof(float);
    header.subchunk2Size = static_cast<uint32_t>(m_data.size() * sizeof(float));
    header.chunkSize = 36 + header.subchunk2Size;

    outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
    outFile.write(reinterpret_cast<const char*>(m_data.data()), m_data.size() * sizeof(float));
}

double RecorderAudio::get_recorded_time() {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_current_audio_time;
}

std::string RecorderAudio::get_data_size() {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    size_t size_in_bytes = m_data.size() * sizeof(float);

    if (size_in_bytes < 1024) {
        return fmt::format("{} B", size_in_bytes);
    } else if (size_in_bytes < 1024 * 1024) {
        return fmt::format("{:.2f} KB", size_in_bytes / 1024.0);
    } else if (size_in_bytes < 1024 * 1024 * 1024) {
        return fmt::format("{:.2f} MB", size_in_bytes / (1024.0 * 1024.0));
    } else {
        return fmt::format("{:.2f} GB", size_in_bytes / (1024.0 * 1024.0 * 1024.0));
    }
}