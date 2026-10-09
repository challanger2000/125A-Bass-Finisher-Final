#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace HighGainGuitarFinisher {
class DemoGate {
public:
    void configure(double sampleRate, bool licensed) noexcept {
        licensed_ = licensed;
        sampleRate_ = std::isfinite(sampleRate) && sampleRate > 1.0 ? sampleRate : 44100.0;
        normalSamples_ = secondsToSamples(60.0);
        silenceSamples_ = secondsToSamples(3.0);
        fadeSamples_ = std::max<std::int64_t>(1, secondsToSamples(0.010));
        phaseSamples_ = 0;
    }
    template <class Sample>
    void process(Sample** outputs, int channels, int samples) noexcept {
        if (licensed_ || !outputs || channels <= 0 || samples <= 0) return;
        const std::int64_t cycleSamples = normalSamples_ + silenceSamples_;
        if (cycleSamples <= 0) return;
        for (int i = 0; i < samples; ++i) {
            const double g = gainForPhase(phaseSamples_);
            for (int ch = 0; ch < channels; ++ch)
                if (outputs[ch]) outputs[ch][i] = static_cast<Sample>(static_cast<double>(outputs[ch][i]) * g);
            if (++phaseSamples_ >= cycleSamples) phaseSamples_ = 0;
        }
    }
private:
    std::int64_t secondsToSamples(double seconds) const noexcept {
        return std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(sampleRate_ * seconds)));
    }
    double gainForPhase(std::int64_t phase) const noexcept {
        if (phase < normalSamples_) return 1.0;
        const std::int64_t inMute = phase - normalSamples_;
        if (inMute < fadeSamples_) return 1.0 - static_cast<double>(inMute + 1) / static_cast<double>(fadeSamples_);
        const std::int64_t fadeInStart = std::max<std::int64_t>(fadeSamples_, silenceSamples_ - fadeSamples_);
        if (inMute < fadeInStart) return 0.0;
        if (inMute < silenceSamples_) {
            return std::clamp(static_cast<double>(inMute - fadeInStart + 1) /
                              static_cast<double>(std::max<std::int64_t>(1, silenceSamples_ - fadeInStart)), 0.0, 1.0);
        }
        return 1.0;
    }
    bool licensed_ = false;
    double sampleRate_ = 44100.0;
    std::int64_t normalSamples_ = 0, silenceSamples_ = 0, fadeSamples_ = 1, phaseSamples_ = 0;
};
} // namespace HighGainGuitarFinisher
