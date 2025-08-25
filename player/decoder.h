#pragma once

#include <memory>
#include <new>
#include <reimu/core/file.h>

#include <functional>
#include <thread>

#include "device.h"
#include "reimu/core/error.h"

class DecoderStream {
    void read();
    void write();

    void seek();
    long pos() const;
};

class Decoder {
public:
    struct DecoderError : public reimu::ErrorBase {
        std::string as_string() const {
            return "DecoderError";
        }
    };

    struct DecoderPrivate;

    Decoder(AudioFormat fmt);
    ~Decoder();

    reimu::Result<void, DecoderError> load(std::shared_ptr<reimu::File> file);

    void start();
    void stop();

    void set_audio_output_fmt(AudioFormat fmt);

    std::function<void(const uint8_t *data, size_t samples_per_channel)> on_decoded_data;

private:
    void init_audio_resampler();
    void decode_frame(struct AVFrame *frame, const uint8_t *sample_buf, int sample_buf_size);

    AudioFormat m_audio_format;

    struct DecoderPrivate *m_data;
    struct AVFormatContext *m_av_fmt_ctx;

    int m_audio_stream_index = -1;

    std::shared_ptr<reimu::File> m_file;

    std::mutex m_decoder_mutex;
    std::thread m_decoder_thread;

    std::atomic_bool m_decoder_running = false;
};
