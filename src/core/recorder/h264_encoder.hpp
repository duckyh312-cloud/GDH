#pragma once
#include <Geode/Geode.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class H264Encoder {
public:
    H264Encoder(int width, int height, int fps, const std::string& bitrate_str)
        : width_(width), height_(height), fps_(fps), pts_(0), audio_pts_(0),
          enc_ctx_(nullptr), audio_enc_ctx_(nullptr), fmt_ctx_(nullptr), 
          stream_(nullptr), audio_stream_(nullptr), yuv_frame_(nullptr), 
          rgb_frame_(nullptr), audio_frame_(nullptr), pkt_(nullptr), sws_ctx_(nullptr),
          is_valid_(false)
    {
        const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
        if (!codec) { cleanup(); return; }

        enc_ctx_ = avcodec_alloc_context3(codec);
        if (!enc_ctx_) { cleanup(); return; }

        int64_t bitrate = 25000000; 
        if (!bitrate_str.empty()) {
            std::string str_copy = bitrate_str;
            char unit = str_copy.back();
            int64_t multiplier = 1;

            if (unit == 'M' || unit == 'm') {
                multiplier = 1000000ll;
                str_copy.pop_back();
            } else if (unit == 'K' || unit == 'k') {
                multiplier = 1000ll;
                str_copy.pop_back();
            }

            auto parsed_val = geode::utils::numFromString<int64_t>(str_copy);
            if (!parsed_val.isErr()) {
                bitrate = parsed_val.unwrap() * multiplier;
            }
        }

        enc_ctx_->bit_rate = bitrate;
        enc_ctx_->width = width_;
        enc_ctx_->height = height_;
        enc_ctx_->time_base = {1, fps_};
        enc_ctx_->framerate = {fps_, 1};
        enc_ctx_->gop_size = 10;
        enc_ctx_->max_b_frames = 0;
        enc_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
        enc_ctx_->colorspace = AVCOL_SPC_BT709;
        enc_ctx_->color_primaries = AVCOL_PRI_BT709;
        enc_ctx_->color_trc = AVCOL_TRC_BT709;
        enc_ctx_->color_range = AVCOL_RANGE_MPEG;

        av_opt_set(enc_ctx_->priv_data, "preset", "medium", 0);

        sws_ctx_ = sws_getContext(width_, height_, AV_PIX_FMT_RGB24,
                                  width_, height_, AV_PIX_FMT_YUV420P,
                                  SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws_ctx_) { cleanup(); return; }
        
        const int *inv_table = sws_getCoefficients(SWS_CS_ITU601);
        const int *table     = sws_getCoefficients(SWS_CS_ITU709);
        sws_setColorspaceDetails(sws_ctx_, inv_table, 1, table, 0, 0, 1 << 16, 1 << 16);

        if (avcodec_open2(enc_ctx_, codec, nullptr) < 0) {
            cleanup();
            return;
        }

        const AVCodec* audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (audio_codec) {
            audio_enc_ctx_ = avcodec_alloc_context3(audio_codec);
            if (audio_enc_ctx_) {
                int sampleRate = 0, channels = 0;
                FMODAudioEngine::sharedEngine()->m_system->getSoftwareFormat(&sampleRate, nullptr, &channels);
                
                if (sampleRate <= 0) sampleRate = 44100;

                audio_enc_ctx_->bit_rate = 320000;
                audio_enc_ctx_->sample_fmt = AV_SAMPLE_FMT_FLTP;
                audio_enc_ctx_->sample_rate = 44100;
                audio_enc_ctx_->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
                audio_enc_ctx_->time_base = {1, audio_enc_ctx_->sample_rate};
                if (avcodec_open2(audio_enc_ctx_, audio_codec, nullptr) < 0) {
                    avcodec_free_context(&audio_enc_ctx_);
                }
            }
        }

        pkt_ = av_packet_alloc();
        if (!pkt_) { cleanup(); return; }
        
        yuv_frame_ = av_frame_alloc();
        if (!yuv_frame_) { cleanup(); return; }
        yuv_frame_->format = enc_ctx_->pix_fmt;
        yuv_frame_->width = width_;
        yuv_frame_->height = height_;
        if (av_frame_get_buffer(yuv_frame_, 0) < 0) { cleanup(); return; }

        rgb_frame_ = av_frame_alloc();
        if (!rgb_frame_) { cleanup(); return; }
        rgb_frame_->format = AV_PIX_FMT_RGB24;
        rgb_frame_->width = width_;
        rgb_frame_->height = height_;
        if (av_frame_get_buffer(rgb_frame_, 0) < 0) { cleanup(); return; }

        is_valid_ = true;
    }

    ~H264Encoder() {
        stop();
        cleanup();
    }

    bool is_valid() const { return is_valid_; }

    bool start(const std::string& filename) {
        if (!is_valid_) return false;
        if (fmt_ctx_) stop();

        int ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, filename.c_str());
        if (ret < 0 || !fmt_ctx_) return false;

        stream_ = avformat_new_stream(fmt_ctx_, nullptr);
        if (!stream_) { stop(); return false; }
        avcodec_parameters_from_context(stream_->codecpar, enc_ctx_);
        stream_->time_base = enc_ctx_->time_base;

        if (audio_enc_ctx_) {
            audio_stream_ = avformat_new_stream(fmt_ctx_, nullptr);
            if (audio_stream_) {
                avcodec_parameters_from_context(audio_stream_->codecpar, audio_enc_ctx_);
                audio_stream_->time_base = audio_enc_ctx_->time_base;
            }
        }

        if (!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open(&fmt_ctx_->pb, filename.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) { stop(); return false; }
        }

        if (avformat_write_header(fmt_ctx_, nullptr) < 0) {
            stop();
            return false;
        }

        pts_ = 0;
        audio_pts_ = 0;
        return true;
    }

    void encode_frame(const std::vector<uint8_t>& rgb_data) {
        if (!is_valid_ || !fmt_ctx_ || rgb_frame_ == nullptr) return;

        av_image_fill_arrays(rgb_frame_->data, rgb_frame_->linesize, 
                             rgb_data.data(), AV_PIX_FMT_RGB24, width_, height_, 1);

        uint8_t* flipped_data[4] = { rgb_frame_->data[0] + rgb_frame_->linesize[0] * (height_ - 1), 0, 0, 0 };
        int flipped_linesize[4] = { -rgb_frame_->linesize[0], 0, 0, 0 };

        av_frame_make_writable(yuv_frame_);
        sws_scale(sws_ctx_, flipped_data, flipped_linesize, 0, height_, 
                  yuv_frame_->data, yuv_frame_->linesize);

        yuv_frame_->pts = pts_++;
        encode_internal(enc_ctx_, stream_, yuv_frame_);
    }

    void finalize_with_audio(std::vector<float> samples) {
        if (!is_valid_ || !fmt_ctx_ || !audio_enc_ctx_ || samples.empty()) return;

        int sample_rate = audio_enc_ctx_->sample_rate;
        int channels = audio_enc_ctx_->ch_layout.nb_channels;

        double video_duration = static_cast<double>(pts_) / fps_;
        size_t max_samples = static_cast<size_t>(video_duration * sample_rate);
        if (samples.size() / channels > max_samples) {
            samples.resize(max_samples * channels);
        }

        audio_frame_ = av_frame_alloc();
        if (!audio_frame_) return;

        audio_frame_->nb_samples = audio_enc_ctx_->frame_size;
        audio_frame_->format = audio_enc_ctx_->sample_fmt;
        audio_frame_->ch_layout = audio_enc_ctx_->ch_layout;
        if (av_frame_get_buffer(audio_frame_, 0) < 0) {
            av_frame_free(&audio_frame_);
            return;
        }

        size_t sample_idx = 0;
        int frame_size = audio_enc_ctx_->frame_size;

        while (sample_idx + frame_size * channels <= samples.size()) {
            av_frame_make_writable(audio_frame_);
            
            float* l_channel = reinterpret_cast<float*>(audio_frame_->data[0]);
            float* r_channel = reinterpret_cast<float*>(audio_frame_->data[1]);
            
            for (int i = 0; i < frame_size; ++i) {
                l_channel[i] = samples[sample_idx + i * channels];
                r_channel[i] = samples[sample_idx + i * channels + 1];
            }
            
            audio_frame_->pts = audio_pts_;
            audio_pts_ += frame_size;
            sample_idx += frame_size * channels;

            encode_internal(audio_enc_ctx_, audio_stream_, audio_frame_);
        }

        encode_internal(audio_enc_ctx_, audio_stream_, nullptr);
        av_frame_free(&audio_frame_);
    }

    void stop() {
        if (!fmt_ctx_) return;
        if (enc_ctx_ && stream_) encode_internal(enc_ctx_, stream_, nullptr);
        av_write_trailer(fmt_ctx_);

        if (!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&fmt_ctx_->pb);
        }
        avformat_free_context(fmt_ctx_);
        fmt_ctx_ = nullptr;
    }

private:
    void cleanup() {
        if (enc_ctx_) avcodec_free_context(&enc_ctx_);
        if (audio_enc_ctx_) avcodec_free_context(&audio_enc_ctx_);
        if (yuv_frame_) av_frame_free(&yuv_frame_);
        if (rgb_frame_) av_frame_free(&rgb_frame_);
        if (pkt_) av_packet_free(&pkt_);
        if (sws_ctx_) sws_freeContext(sws_ctx_);
        is_valid_ = false;
    }

    void encode_internal(AVCodecContext* ctx, AVStream* st, AVFrame* frame) {
        if (!ctx || !st) return;
        int ret = avcodec_send_frame(ctx, frame);
        if (ret < 0) return;

        while (ret >= 0) {
            ret = avcodec_receive_packet(ctx, pkt_);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return;
            if (ret < 0) return;
            
            pkt_->stream_index = st->index;
            av_packet_rescale_ts(pkt_, ctx->time_base, st->time_base);
            av_interleaved_write_frame(fmt_ctx_, pkt_);
            av_packet_unref(pkt_);
        }
    }

    int width_, height_, fps_;
    int64_t pts_;
    int64_t audio_pts_;
    AVCodecContext *enc_ctx_, *audio_enc_ctx_;
    AVFormatContext* fmt_ctx_;
    AVStream *stream_, *audio_stream_;
    AVFrame *yuv_frame_, *rgb_frame_, *audio_frame_;
    AVPacket* pkt_;
    SwsContext* sws_ctx_;
    bool is_valid_;
};