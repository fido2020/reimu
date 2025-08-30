#pragma once

#include <reimu/core/result.h>
#include <reimu/core/error.h>
#include <reimu/core/optional.h>

#include <memory>
#include <string_view>
#include <vector>

enum class AudioSampleFormat {
    Unknown,
    Unsigned8,
    Signed16,
    Signed24,
    Signed32,
    Float32
};

struct AudioFormat {
    AudioSampleFormat sample_format;
    int sample_rate;
    int channels;

    uint32_t sample_size() const {
        switch (sample_format) {
            case AudioSampleFormat::Unsigned8: return 1;
            case AudioSampleFormat::Signed16:  return 2;
            case AudioSampleFormat::Signed24:  return 3;
            case AudioSampleFormat::Signed32:  return 4;
            case AudioSampleFormat::Float32:   return 4;
            default:
                return 0;
        }
    }
};

class AudioContext {
public:
    struct AudioContextImpl;

    AudioContext(AudioContextImpl *);
    ~AudioContext();

    void start_playback();
    void stop_playback();

    bool is_playing() const;

    void clear_queue();
    void queue_frames(const void *data, uint32_t num_frames);

    long get_current_timestamp(long reference_us) const;

    AudioFormat get_sample_format() const;

private:
    AudioContextImpl *m_data;
};

struct AudioError : reimu::ErrorBase {
    AudioError(std::string msg) : message(std::move(msg)) {}

    std::string as_string() const override {
        return message;
    }

private:
    std::string message;
};

class AudioDevice {
public:
    ~AudioDevice();

    struct AudioDeviceImpl;

    template<typename T>
    static std::shared_ptr<AudioDevice> create(const T &data);

    std::string_view name() const;
    bool is_default() const;
    std::vector<AudioFormat> supported_formats() const;

    reimu::Result<std::shared_ptr<AudioContext>, AudioError> connect(const AudioFormat &fmt) const;

private:
    AudioDevice(AudioDeviceImpl *data);

    AudioDeviceImpl *m_data;
};

void register_devices();
reimu::Optional<std::shared_ptr<AudioDevice>> default_audio_device();
