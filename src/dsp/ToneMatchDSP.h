#pragma once

#include "Biquad.h"
#include "ToneMatchProfile.h"
#include "ZeroLatencyPartitionedFIR.h"

#include <array>
#include <memory>

namespace HighGainGuitarFinisher::dsp {

class ToneMatchDSP {
public:
    ToneMatchDSP()
        : fir_(std::make_unique<ZeroLatencyPartitionedFIR>()) {}

    void prepare(double sampleRate);
    void reset() noexcept;

    void setAmount(double normalized) noexcept;
    void setBassContext(
        double lowCutNormalized,
        double massNormalized) noexcept;
    void setProfile(const ToneMatchProfile& profile) noexcept;
    void setProfile(const ToneMatchProfile& profile,
                    const ZeroLatencyPartitionedFIR::PreparedKernel& kernel) noexcept;
    void clearProfile() noexcept;

    void processFrame(
        double& left,
        double& right) noexcept;

    double currentAmount() const noexcept {
        return amountTarget_;
    }

    const ToneMatchProfile& profile() const noexcept {
        return profile_;
    }

private:
    void sanitizeProfile(
        const ToneMatchProfile& source) noexcept;

    void updateCoefficients(
        double amount) noexcept;

    void resetFilters() noexcept;
    std::array<Biquad, 2> lowShelf_ {};
    std::array<
        std::array<Biquad, 2>,
        kToneMatchPeakCount> peaks_ {};
    std::array<Biquad, 2> highShelf_ {};

    std::unique_ptr<ZeroLatencyPartitionedFIR> fir_ {};

    ToneMatchProfile profile_ {};

    double sampleRate_ {44100.0};
    double amountTarget_ {0.0};
    double amountSmoothed_ {0.0};
    double lastCoefficientAmount_ {-1.0};
    double amountSmoothing_ {0.0};
    double lowCutContext_ {0.0};
    double massContext_ {0.0};

    int coefficientCountdown_ {0};
    bool prepared_ {false};
    bool bypassState_ {true};
};

} // namespace HighGainGuitarFinisher::dsp
