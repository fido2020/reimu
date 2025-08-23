#pragma once

#include <vector>

enum AudioSampleFormat {
    Float32
};

class AudioDevice {
    int sample_rate() const;
    int num_channels() const;

    std::vector<AudioSampleFormat> supported_formats() const;
};
