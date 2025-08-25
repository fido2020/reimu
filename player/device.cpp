#include <condition_variable>
#include <pthread.h>
#include <reimu/core/logger.h>
#include <reimu/core/result.h>

#include <memory>
#include <string_view>
#include <vector>
#include <mutex>
#include <algorithm>

#include <miniaudio.h>
#include <string.h>

#include "device.h"
#include "reimu/core/optional.h"

static std::vector<std::shared_ptr<AudioDevice>> audio_devices;
static std::shared_ptr<AudioDevice> _default_audio_device;
static ma_context context;
static constexpr size_t audio_buffer_size = 0x100000;

struct AudioDevice::AudioDeviceImpl {
    ma_device_info info;
};

struct AudioContext::AudioContextImpl {
    ma_device device;
    std::array<uint8_t, audio_buffer_size> buffer;

    int frame_sz = 0;
    int max_frames_in_buffer = 0;

    uint32_t producer_head = 0;
    uint32_t consumer_head = 0;

    int num_channels = 0;
    ma_format sample_format = ma_format::ma_format_unknown;
    int sample_rate = 0;

    std::mutex queue_mutex;
    std::condition_variable producer_cv;
};

static reimu::Optional<AudioSampleFormat> convert_sample_fmt(ma_format f);
static ma_format convert_sample_fmt(AudioSampleFormat f);

static void audio_playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

template<>
std::shared_ptr<AudioDevice> AudioDevice::create<ma_device_info>(const ma_device_info &data) {
    auto impl = new AudioDeviceImpl;
    impl->info = data;

    auto dev = std::shared_ptr<AudioDevice>(new AudioDevice(impl));

    if (data.isDefault) {
        _default_audio_device = dev;
    }

    audio_devices.push_back(dev);

    ma_device_config cfg;

    return dev;
}

AudioDevice::AudioDevice(AudioDeviceImpl *data)
    : m_data(data) {
}

AudioDevice::~AudioDevice() {
    delete m_data;
}

std::string_view AudioDevice::name() const {
    return m_data->info.name;
}

bool AudioDevice::is_default() const {
    return m_data->info.isDefault;
}

std::vector<AudioFormat> AudioDevice::supported_formats() const {
    auto formats = std::vector<AudioFormat>{};
    
    auto cnt = m_data->info.nativeDataFormatCount;
    auto *fmt = m_data->info.nativeDataFormats;

    while (cnt--) {
        if (auto opt = convert_sample_fmt(fmt->format); opt.has_some()) {
            AudioFormat format;
            format.sample_format = opt.ensure();
            format.sample_rate = fmt->sampleRate;
            format.channels = fmt->channels;
            formats.push_back(format);
        } else {
            reimu::logger::warn("Unexpected audio format");
        }
        fmt++;
    }

    return formats;
}

reimu::Result<std::shared_ptr<AudioContext>, AudioError> AudioDevice::connect(const AudioFormat &fmt) const {
    auto impl = new AudioContext::AudioContextImpl;

    ma_device_config cfg = ma_device_config_init(ma_device_type::ma_device_type_playback);
    cfg.playback.format = ma_format::ma_format_f32; // Default to float32
    cfg.playback.channels = fmt.channels;
    cfg.playback.pDeviceID = &m_data->info.id;
    cfg.pUserData = impl;
    cfg.sampleRate = fmt.sample_rate;
    cfg.playback.format = convert_sample_fmt(fmt.sample_format);
    cfg.dataCallback = audio_playback_callback;

    ma_device &device = impl->device;

    ma_result result = ma_device_init(&context, &cfg, &device);
    
    if (result != MA_SUCCESS) {
        delete impl;

        reimu::logger::warn("Failed to initialize audio device: {}", ma_result_description(result));
        
        auto e = AudioError{ma_result_description(result)};
        return ERR(std::move(e));
    }

    impl->sample_rate = device.sampleRate;
    impl->num_channels = device.playback.channels;
    impl->sample_format = device.playback.format;

    impl->frame_sz = ma_get_bytes_per_frame(impl->sample_format, impl->num_channels);

    impl->device = device;
    impl->max_frames_in_buffer = audio_buffer_size / impl->frame_sz;

    auto ctx = std::shared_ptr<AudioContext>(new AudioContext(impl));

    return OK(ctx);
}

AudioContext::AudioContext(AudioContextImpl *data)
    : m_data(data) {}

AudioContext::~AudioContext() {
    if (m_data) {
        ma_device_stop(&m_data->device);
        ma_device_uninit(&m_data->device);
        delete m_data;
    }
}

AudioFormat AudioContext::get_sample_format() const {
    AudioFormat fmt;
    fmt.sample_format = convert_sample_fmt(m_data->sample_format).ensure();
    fmt.sample_rate = m_data->sample_rate;
    fmt.channels = m_data->num_channels;
    return fmt;
}

void AudioContext::start_playback() {
    std::lock_guard<std::mutex> lock(m_data->queue_mutex);

    if (ma_device_start(&m_data->device) != MA_SUCCESS) {
        reimu::logger::fatal("Failed to start audio device");
    }
}

void AudioContext::stop_playback() {
    std::lock_guard<std::mutex> lock(m_data->queue_mutex);

    if (ma_device_stop(&m_data->device) != MA_SUCCESS) {
        reimu::logger::fatal("Failed to stop audio device");
    }
}

void AudioContext::queue_frames(const void *data, uint32_t num_frames) {
    if (num_frames > m_data->max_frames_in_buffer) {
        reimu::logger::warn("Trying to queue too many frames, truncating to max buffer size");
        num_frames = m_data->max_frames_in_buffer;
    }

    std::unique_lock<std::mutex> lock{m_data->queue_mutex};
    m_data->producer_cv.wait(lock, [this, num_frames]() {
        auto frames_in_buffer = (m_data->producer_head - m_data->consumer_head + m_data->max_frames_in_buffer) % m_data->max_frames_in_buffer;
        return frames_in_buffer + num_frames < m_data->max_frames_in_buffer;
    });
    
    while (num_frames > 0) {
        size_t frames_to_copy;
        if (m_data->producer_head < m_data->consumer_head) {
            frames_to_copy = std::min(num_frames, m_data->max_frames_in_buffer - m_data->producer_head);
        } else {
            frames_to_copy = std::min(num_frames, m_data->max_frames_in_buffer - (m_data->producer_head - m_data->consumer_head));
        }

        if (frames_to_copy == 0) {
            reimu::logger::debug("No space left in audio buffer, dropping frames");
            return;
        }

        memcpy(m_data->buffer.data() + m_data->producer_head * m_data->frame_sz, data, frames_to_copy * m_data->frame_sz);

        m_data->producer_head = (m_data->producer_head + frames_to_copy) % m_data->max_frames_in_buffer;
        data = static_cast<const uint8_t*>(data) + frames_to_copy * m_data->frame_sz;
        num_frames -= frames_to_copy;
    }
}

reimu::Optional<std::shared_ptr<AudioDevice>> default_audio_device() {
    return _default_audio_device;
}

void register_devices() {
    std::vector<ma_device_info> devices;

    auto device_cb = [](
        ma_context* pContext, ma_device_type deviceType, const ma_device_info* pInfo, void* pUserData
    ) -> ma_bool32 {
        reimu::logger::debug("Audio device: {}, is output? {}", pInfo->name, deviceType == ma_device_type::ma_device_type_playback);
        
        if (deviceType != ma_device_type::ma_device_type_playback) {
            return MA_TRUE; // Only interested in playback devices
        }


        reimu::logger::debug("Audio device {} supports {} formats", pInfo->name, pInfo->nativeDataFormatCount);

        ((typeof(&devices))pUserData)->push_back(*pInfo);
        
        return MA_TRUE;
    };

    ma_result result = ma_context_init(NULL, 0, NULL, &context);

    if (result != MA_SUCCESS) {
        reimu::logger::fatal("Failed to initialize miniaudio context: {}", ma_result_description(result));
        return;
    }

    ma_context_enumerate_devices(&context, device_cb, &devices);

    for (const auto &d : devices) {
        AudioDevice::create(d);
    }

    // ma_context_uninit(&context);
}

static reimu::Optional<AudioSampleFormat> convert_sample_fmt(ma_format f) {
    switch (f) {
        case ma_format::ma_format_u8:   return AudioSampleFormat::Unsigned8;
        case ma_format::ma_format_s16:  return AudioSampleFormat::Signed16;
        case ma_format::ma_format_s24:  return AudioSampleFormat::Signed24;
        case ma_format::ma_format_s32:  return AudioSampleFormat::Signed32;
        case ma_format::ma_format_f32:  return AudioSampleFormat::Float32;
        default:
            return OPT_NONE;
    }
}

static ma_format convert_sample_fmt(AudioSampleFormat f) {
    switch (f) {
        case AudioSampleFormat::Unsigned8: return ma_format::ma_format_u8;
        case AudioSampleFormat::Signed16:  return ma_format::ma_format_s16;
        case AudioSampleFormat::Signed24:  return ma_format::ma_format_s24;
        case AudioSampleFormat::Signed32:  return ma_format::ma_format_s32;
        case AudioSampleFormat::Float32:   return ma_format::ma_format_f32;
        default:
            return ma_format::ma_format_unknown;
    }
}

static void audio_playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    auto *impl = reinterpret_cast<AudioContext::AudioContextImpl *>(pDevice->pUserData);

    std::lock_guard<std::mutex> lock(impl->queue_mutex);

    auto frames_in_buffer = (impl->max_frames_in_buffer + impl->producer_head - impl->consumer_head) % impl->max_frames_in_buffer;
    
    // Lets not give the user a bad time if we don't have data to play :p
    memset(pOutput, 0, frameCount * impl->frame_sz);

    if (frames_in_buffer == 0) {
        impl->producer_cv.notify_all();
        return;
    }

    while (frames_in_buffer > 0 && frameCount > 0) {
        size_t frames_to_copy;
        if (impl->consumer_head < impl->producer_head) {
            frames_to_copy = std::min(frameCount, impl->producer_head - impl->consumer_head);
        } else {
            frames_to_copy = std::min(frameCount, impl->max_frames_in_buffer - impl->consumer_head);
        }

        memcpy(pOutput, impl->buffer.data() + impl->consumer_head * impl->frame_sz, frames_to_copy * impl->frame_sz);

        pOutput = static_cast<uint8_t*>(pOutput) + frames_to_copy * impl->frame_sz;
        impl->consumer_head = (impl->consumer_head + frames_to_copy) % impl->max_frames_in_buffer;
        
        frames_in_buffer -= frames_to_copy;
        frameCount -= frames_to_copy;
    }
    
    impl->producer_cv.notify_all();
}
