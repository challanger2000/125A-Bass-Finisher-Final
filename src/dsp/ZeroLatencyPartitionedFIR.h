#pragma once

#include "ToneMatchProfile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <utility>

namespace HighGainGuitarFinisher::dsp {

// Zero-latency uniform partitioned FIR with a time-domain head.
//
// Every output sample uses the 128-tap direct head plus an FFT-computed
// tail from PREVIOUS input partitions. Partition j starts at lag (j+1)*128,
// so the FFT tail for the next output partition is ready before it begins.
// This preserves the exact 4096-tap transfer function at the original
// sample clock, with no lookahead and no allocations in processFrame().
//
// IMPORTANT: setKernel() is for non-audio profile setup, never the
// per-sample loop. Workspaces and FFT twiddles are fixed-size members.
class ZeroLatencyPartitionedFIR {
public:
    static constexpr std::size_t kPartitionSize = 128;
    static constexpr std::size_t kFftSize = 2 * kPartitionSize;
    static constexpr std::size_t kTailPartitions =
        kToneMatchFirTapCount / kPartitionSize - 1;

    using Buffer = std::array<std::complex<double>, kFftSize>;
    // Real-valued FIR and input allow Hermitian half-spectrum storage.
    using Spectrum = std::array<std::complex<double>, kPartitionSize + 1>;

    ZeroLatencyPartitionedFIR() noexcept {
        constexpr double pi = 3.141592653589793238462643383279502884;
        for (std::size_t k = 0; k < twiddles_.size(); ++k) {
            const double angle =
                -2.0 * pi * static_cast<double>(k) /
                static_cast<double>(kFftSize);
            twiddles_[k] = {std::cos(angle), std::sin(angle)};
        }
    }

    struct PreparedKernel {
        std::array<double, kPartitionSize> head {};
        std::array<Spectrum, kTailPartitions> spectra {};
    };

    // Call only on the producer / control thread, before publication into
    // the SPSC mailbox. No FFT work may be triggered by profile receipt
    // inside the realtime process callback.
    void prepareKernel(
        const std::array<double, kToneMatchFirTapCount>& taps,
        PreparedKernel& destination) const noexcept {

        std::copy_n(taps.begin(), kPartitionSize,
                    destination.head.begin());
        for (std::size_t p = 0; p < kTailPartitions; ++p) {
            Buffer spectrum {};
            for (std::size_t i = 0; i < kPartitionSize; ++i)
                spectrum[i] = {
                    taps[(p + 1) * kPartitionSize + i], 0.0
                };
            transform(spectrum, false);
            for (std::size_t bin = 0; bin <= kPartitionSize; ++bin)
                destination.spectra[p][bin] = spectrum[bin];
        }
    }

    // Bounded data copy only; does not call FFT, allocate or wait.
    void installKernel(const PreparedKernel& prepared) noexcept {
        head_ = prepared.head;
        kernelSpectra_ = prepared.spectra;
        reset();
    }

    // Fallback for setupProcessing / setState, never the audio process
    // callback. Live profile changes use prepared mailboxes instead.
    void setKernel(
        const std::array<double, kToneMatchFirTapCount>& taps) noexcept {

        PreparedKernel kernel {};
        prepareKernel(taps, kernel);
        installKernel(kernel);
    }

    void reset() noexcept {
        for (auto& channel : headHistory_)
            channel.fill(0.0);
        for (auto& channel : outputTail_)
            channel.fill(0.0);
        for (auto& channel : overlap_)
            channel.fill(0.0);
        // No need to clear the large FFT-history buffer. Unwritten entries
        // are excluded until validBlocks_ has grown after a reset.
        inputPosition_ = 0;
        spectralWrite_ = 0;
        validBlocks_ = 0;
    }

    void processFrame(double& left, double& right) noexcept {
        double* outputs[2] {&left, &right};
        for (std::size_t c = 0; c < 2; ++c) {
            const double dry = *outputs[c];
            inputBlock_[c][inputPosition_] = dry;
            headHistory_[c][inputPosition_] = dry;

            double headSum = 0.0;
            std::size_t index = inputPosition_;
            for (std::size_t t = 0; t < kPartitionSize; ++t) {
                headSum += head_[t] * headHistory_[c][index];
                index = (index + kPartitionSize - 1) & (kPartitionSize - 1);
            }
            *outputs[c] = headSum + outputTail_[c][inputPosition_];
        }

        if (++inputPosition_ == kPartitionSize) {
            computeNextPartition();
            inputPosition_ = 0;
        }
    }

private:
    void transform(Buffer& data, bool inverse) const noexcept {
        // Iterative radix-2 FFT, power-of-two length fixed to 256.
        for (std::size_t i = 1, j = 0; i < kFftSize; ++i) {
            std::size_t bit = kFftSize >> 1;
            while (j & bit) {
                j ^= bit;
                bit >>= 1;
            }
            j ^= bit;
            if (i < j)
                std::swap(data[i], data[j]);
        }
        for (std::size_t len = 2; len <= kFftSize; len <<= 1) {
            const std::size_t half = len >> 1;
            const std::size_t stride = kFftSize / len;
            for (std::size_t base = 0; base < kFftSize; base += len) {
                for (std::size_t j = 0; j < half; ++j) {
                    const auto w = inverse
                        ? std::conj(twiddles_[j * stride])
                        : twiddles_[j * stride];
                    const auto u = data[base + j];
                    const auto v = data[base + j + half] * w;
                    data[base + j] = u + v;
                    data[base + j + half] = u - v;
                }
            }
        }
        if (inverse) {
            for (auto& value : data)
                value /= static_cast<double>(kFftSize);
        }
    }

    void computeNextPartition() noexcept {
        // Transform the just-completed stereo input partition.
        for (std::size_t c = 0; c < 2; ++c) {
            auto& fft = work_[c];
            fft.fill({0.0, 0.0});
            for (std::size_t i = 0; i < kPartitionSize; ++i)
                fft[i] = {inputBlock_[c][i], 0.0};
            transform(fft, false);
            auto& latest = inputSpectra_[c][spectralWrite_];
            for (std::size_t bin = 0; bin <= kPartitionSize; ++bin)
                latest[bin] = fft[bin];
        }

        validBlocks_ = std::min(validBlocks_ + 1, kTailPartitions);

        for (std::size_t c = 0; c < 2; ++c) {
            auto& sum = work_[c];
            sum.fill({0.0, 0.0});

            for (std::size_t p = 0; p < validBlocks_; ++p) {
                const std::size_t slot =
                    (spectralWrite_ + kTailPartitions - p) % kTailPartitions;
                const auto& input = inputSpectra_[c][slot];
                const auto& kernel = kernelSpectra_[p];
                for (std::size_t bin = 0; bin <= kPartitionSize; ++bin)
                    sum[bin] += input[bin] * kernel[bin];
            }

            // Restore the conjugate negative-frequency half for a real IFFT.
            for (std::size_t bin = kPartitionSize + 1; bin < kFftSize; ++bin)
                sum[bin] = std::conj(sum[kFftSize - bin]);
            transform(sum, true);
            for (std::size_t i = 0; i < kPartitionSize; ++i) {
                outputTail_[c][i] = sum[i].real() + overlap_[c][i];
                overlap_[c][i] = sum[i + kPartitionSize].real();
            }
        }

        spectralWrite_ = (spectralWrite_ + 1) % kTailPartitions;
    }

    std::array<double, kPartitionSize> head_ {};
    std::array<Spectrum, kTailPartitions> kernelSpectra_ {};
    std::array<std::array<Spectrum, kTailPartitions>, 2> inputSpectra_ {};
    std::array<Buffer, 2> work_ {};
    std::array<std::complex<double>, kFftSize / 2> twiddles_ {};

    std::array<std::array<double, kPartitionSize>, 2> inputBlock_ {};
    std::array<std::array<double, kPartitionSize>, 2> headHistory_ {};
    std::array<std::array<double, kPartitionSize>, 2> outputTail_ {};
    std::array<std::array<double, kPartitionSize>, 2> overlap_ {};

    std::size_t inputPosition_ {0};
    std::size_t spectralWrite_ {0};
    std::size_t validBlocks_ {0};
};

} // namespace HighGainGuitarFinisher::dsp
