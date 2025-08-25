#include "decoder.h"
#include "libavutil/channel_layout.h"
#include "reimu/core/file.h"
#include "reimu/core/logger.h"
#include <memory>

extern "C" {

#include <libavutil/error.h>
#include <libavcodec/codec.h>
#include <libavcodec/packet.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

}

#include <assert.h>

static constexpr int avio_buffer_size = 4096;
static constexpr int resample_buffer_size = 16384;

struct Decoder::DecoderPrivate {
    AVIOContext *avio_ctx = nullptr;
    AVCodecContext *avcodec_ctx = nullptr;
    uint8_t *avio_ctx_buffer = nullptr;
    SwrContext *audio_resampler = nullptr;

    ~DecoderPrivate() {
        if (audio_resampler) {
            swr_free(&audio_resampler);
        }

        if (avio_ctx) {
            avio_context_free(&avio_ctx);
        }

        if (avcodec_ctx) {
            avcodec_free_context(&avcodec_ctx);
        }

        if (avio_ctx_buffer) {
            av_freep(&avio_ctx_buffer);
        }
    }
};

Decoder::Decoder(AudioFormat fmt) {
    m_av_fmt_ctx = avformat_alloc_context();
    if (!m_av_fmt_ctx) {
        reimu::logger::fatal("Failed to allocate AVFormatContext");
    }

    m_data = new DecoderPrivate;
    m_av_fmt_ctx->pb = NULL;

    m_data->audio_resampler = swr_alloc();
    if (!m_data->audio_resampler) {
        reimu::logger::fatal("Failed to allocate SwrContext");
    }

    set_audio_output_fmt(fmt);
}

Decoder::~Decoder() {
    delete m_data;
    
    if (m_av_fmt_ctx) {
        avformat_free_context(m_av_fmt_ctx);
    }
}

void Decoder::set_audio_output_fmt(AudioFormat fmt) {
    m_audio_format = fmt;

    if (m_data->avcodec_ctx) {
        init_audio_resampler();
    }
}

reimu::Result<void, Decoder::DecoderError> Decoder::load(std::shared_ptr<reimu::File> file) {
    stop();
    
    const auto read_packet = [](void *opaque, uint8_t *buf, int buf_size) -> int {
        auto *decoder = static_cast<Decoder *>(opaque);
        if (!decoder || !decoder->m_file) {
            return AVERROR(EINVAL);
        }

        auto r = decoder->m_file->read(buf_size);
        if (r.is_err()) {
            return AVERROR(EIO);
        }

        auto data = r.move_val();
        memcpy(buf, data.data(), data.size());

        if (decoder->m_file->is_eof()) {
            return AVERROR_EOF;
        }

        return data.size();
    };

    const auto seek_packet = [](void *opaque, int64_t offset, int whence) -> int64_t {
        auto *decoder = static_cast<Decoder *>(opaque);
        if (!decoder || !decoder->m_file) {
            return AVERROR(EINVAL);
        }

        reimu::SeekMode m;
        switch(whence) {
            case SEEK_SET:
                m = reimu::SeekMode::Absolute;
                break;
            case SEEK_CUR:
                m = reimu::SeekMode::Relative;
                break;
            case SEEK_END:
                m = reimu::SeekMode::End;
                break;
            default:
                return AVERROR(EINVAL);
        }

        decoder->m_file->seek(offset, m);

        return decoder->m_file->offset();
    };

    m_file = std::move(file);

    m_data->avio_ctx_buffer = (uint8_t *)av_malloc(avio_buffer_size);
    m_data->avio_ctx = avio_alloc_context(
        m_data->avio_ctx_buffer, 
        avio_buffer_size, 
        0, 
        this, 
        read_packet, 
        nullptr, 
        seek_packet);
    m_av_fmt_ctx->pb = m_data->avio_ctx;

    if (int ret = avformat_open_input(&m_av_fmt_ctx, nullptr, nullptr, nullptr); ret < 0) {
        reimu::logger::warn("Failed to open input: {}", (ret));
        return ERR(DecoderError{});
    }

    if (avformat_find_stream_info(m_av_fmt_ctx, nullptr) < 0) {
        reimu::logger::warn("Failed to find stream info");
        return ERR(DecoderError{});
    }

    // Find first audio stream
    m_audio_stream_index = av_find_best_stream(m_av_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

    if (m_audio_stream_index < 0) {
        reimu::logger::warn("Failed to find audio stream");
        return ERR(DecoderError{});
    }

    auto codec = avcodec_find_decoder(m_av_fmt_ctx->streams[m_audio_stream_index]->codecpar->codec_id);

    m_data->avcodec_ctx = avcodec_alloc_context3(codec);
    if (!m_data->avcodec_ctx) {
        reimu::logger::fatal("Failed to allocate AVCodecContext");
    }

    if (avcodec_parameters_to_context(m_data->avcodec_ctx, m_av_fmt_ctx->streams[m_audio_stream_index]->codecpar) < 0) {
        reimu::logger::warn("Failed to copy codec parameters");
        return ERR(DecoderError{});
    }

    if (avcodec_open2(m_data->avcodec_ctx, codec, nullptr) < 0) {
        reimu::logger::warn("Failed to open codec");
        return ERR(DecoderError{});
    }

    init_audio_resampler();

    return OK();
}

void Decoder::start() {
    if (m_decoder_running.load()) {
        return;
    }
    
    if (m_decoder_thread.joinable()) {
        m_decoder_thread.join();
    }

    m_decoder_running.store(true);
    m_decoder_thread = std::thread([this]() {
        std::unique_ptr<uint8_t[]> sample_buf = std::make_unique<uint8_t[]>(resample_buffer_size);

        AVPacket *packet = av_packet_alloc();
        AVFrame *frame = av_frame_alloc();

        while (m_decoder_running.load()) {
            assert(m_av_fmt_ctx != nullptr);
            if (av_read_frame(m_av_fmt_ctx, packet) < 0) {
                reimu::logger::warn("Failed to read frame");
                break;
            }
            
            if (packet->stream_index != m_audio_stream_index) {
                reimu::logger::warn("stream idx: {} current {}", packet->stream_index, m_audio_stream_index);
                
                av_packet_unref(packet);
                continue;
            }

            if (avcodec_send_packet(m_data->avcodec_ctx, packet) < 0) {
                reimu::logger::fatal("Failed to send packet to decoder");
                break;
            }

            ssize_t ret = 0;
            while (ret >= 0) {
                // Decodes the audio
                ret = avcodec_receive_frame(m_data->avcodec_ctx, frame);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                    // Get the next packet and retry
                    reimu::logger::warn("retry");
                    break;
                } else if (ret) {
                    reimu::logger::warn("Could not decode frame: {}", ret);
                    break;
                }

                decode_frame(frame, sample_buf.get(), resample_buffer_size);

                av_frame_unref(frame);
            }

            av_packet_unref(packet);
        }

        m_decoder_running.store(false);

        av_packet_free(&packet);
        av_frame_free(&frame);
    });
}

void Decoder::stop() {
    if (!m_decoder_running.load()) {
        return;
    }

    m_decoder_running.store(false);

    if (m_decoder_thread.joinable()) {
        m_decoder_thread.join();
    }
}

void Decoder::init_audio_resampler() {
    if (!m_data->avcodec_ctx) {
        reimu::logger::warn("Cannot init audio resampler without codec context");
        return;
    }

    auto *avcodec = m_data->avcodec_ctx;

    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_NONE;

    switch (m_audio_format.sample_format) {
        case AudioSampleFormat::Unsigned8:
            out_sample_fmt = AV_SAMPLE_FMT_U8;
            break;
        case AudioSampleFormat::Signed16:
            out_sample_fmt = AV_SAMPLE_FMT_S16;
            break;
        case AudioSampleFormat::Signed24:
            reimu::logger::fatal("24-bit audio not supported");
            break;
        case AudioSampleFormat::Signed32:
            out_sample_fmt = AV_SAMPLE_FMT_S32;
            break;
        case AudioSampleFormat::Float32:
            out_sample_fmt = AV_SAMPLE_FMT_FLT;
            break;
        default:
            reimu::logger::warn("Unsupported output sample format");
            return;
    }

    AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_MONO;
    if (m_audio_format.channels == 2) {
        out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    } else if (m_audio_format.channels != 1) {
        reimu::logger::warn("Unsupported number of channels: {}", m_audio_format.channels);
        return;
    }

    if (swr_alloc_set_opts2(&m_data->audio_resampler,
        &out_ch_layout,
        out_sample_fmt,
        m_audio_format.sample_rate,
        &avcodec->ch_layout,
        avcodec->sample_fmt,
        avcodec->sample_rate,
        0,
        nullptr) < 0) {
        reimu::logger::fatal("Failed to initialize audio resampler");
    }

    reimu::logger::debug("Initialized audio resampler: {} Hz, {}, {} channels -> {} Hz, {}, {} channels",
        avcodec->sample_rate,
        av_get_sample_fmt_name(avcodec->sample_fmt),
        avcodec->ch_layout.nb_channels,
        m_audio_format.sample_rate,
        av_get_sample_fmt_name(out_sample_fmt),
        out_ch_layout.nb_channels
    );

    if (swr_init(m_data->audio_resampler) < 0) {
        reimu::logger::fatal("Failed to initialize audio resampler");
    }
}

void Decoder::decode_frame(struct AVFrame *frame, const uint8_t *sample_buf, int sample_buf_size) {
    if (!m_data->audio_resampler) {
        reimu::logger::warn("Audio resampler not initialized");
        return;
    }

    float time_sec = (float)frame->pts * av_q2d(m_data->avcodec_ctx->time_base);
    float frame_len_sec = (float)frame->nb_samples / frame->sample_rate;
    reimu::logger::debug("timestamp: {}, len: {}", time_sec, frame_len_sec);

    int samples_to_write = av_rescale_rnd(
        swr_get_delay(m_data->audio_resampler, frame->sample_rate) + frame->nb_samples,
        m_audio_format.sample_rate,
        frame->sample_rate,
        AV_ROUND_UP);

    auto buffer_size_samples = sample_buf_size
        / m_audio_format.sample_size()
        / m_audio_format.channels;

    reimu::logger::debug("Resampling {} samples to {} samples", frame->nb_samples, samples_to_write);
    reimu::logger::debug("Input: {} Hz, {}, {} channels", frame->sample_rate, av_get_sample_fmt_name((AVSampleFormat)frame->format), frame->ch_layout.nb_channels);
    reimu::logger::debug("Output buffer holds {} samples", buffer_size_samples);

    int ret;
    if((ret = swr_convert(
        m_data->audio_resampler,
        (uint8_t* const*)&sample_buf,
        buffer_size_samples,
        (const uint8_t **)frame->extended_data,
        frame->nb_samples)) > 0) {

        // dump frame->extended_data
        for (int i = 0; i < ret * 4; i += 4) {
            int16_t v = *(int16_t*)(sample_buf + i);
            //if (v!=0)
            //reimu::logger::debug("{} ", v);
        }

        if (on_decoded_data) {
            on_decoded_data(
                (uint8_t*)sample_buf,
                ret
            );
        }
    }

    if (ret < 0) {
        reimu::logger::warn("Error while resampling audio: {}", ret);
    }
}
