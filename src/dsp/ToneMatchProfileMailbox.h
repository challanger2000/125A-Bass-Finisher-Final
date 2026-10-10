#pragma once

#include "ToneMatchProfile.h"
#include "ZeroLatencyPartitionedFIR.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include <array>
#include <atomic>
#include <cstddef>

namespace HighGainGuitarFinisher::dsp {

class ToneMatchProfileMailbox {
public:
    bool push(
        const ToneMatchProfile& profile) noexcept {

        const auto write =
            writeIndex_.load(
                std::memory_order_relaxed);

        const auto next =
            increment(write);

        const auto read =
            readIndex_.load(
                std::memory_order_acquire);

        if (next == read)
            return false;

        auto& slot = (*slots_)[write];
        slot.profile = profile;
        // Producer-side FFT: before SPSC publication / outside process().
        if (slot.profile.valid && slot.profile.firValid) {
            bool anyEnergy = false;
            for (double& tap : slot.profile.firTaps) {
                tap = std::clamp(
                    std::isfinite(tap) ? tap : 0.0, -8.0, 8.0);
                anyEnergy = anyEnergy || std::abs(tap) > 1.0e-15;
            }
            if (anyEnergy)
                kernelPreparer_->prepareKernel(
                    slot.profile.firTaps, slot.kernel);
            else
                slot.profile.firValid = false;
        }

        writeIndex_.store(
            next,
            std::memory_order_release);

        return true;
    }

    bool popLatest(
        ToneMatchProfile& profile,
        ZeroLatencyPartitionedFIR::PreparedKernel* kernel = nullptr) noexcept {

        auto read =
            readIndex_.load(
                std::memory_order_relaxed);

        const auto write =
            writeIndex_.load(
                std::memory_order_acquire);

        if (read == write)
            return false;

        std::size_t latest = read;

        for (std::size_t i = 0;
             i < kSlotCount - 1 &&
             read != write;
             ++i) {

            latest = read;
            read = increment(read);
        }

        profile = (*slots_)[latest].profile;
        if (kernel && profile.valid && profile.firValid)
            *kernel = (*slots_)[latest].kernel;

        readIndex_.store(
            write,
            std::memory_order_release);

        return true;
    }

    void clearConsumerSide() noexcept {
        const auto write =
            writeIndex_.load(
                std::memory_order_acquire);

        readIndex_.store(
            write,
            std::memory_order_release);
    }

private:
    static constexpr std::size_t kSlotCount = 4;

    static constexpr std::size_t
    increment(std::size_t index) noexcept {
        return (index + 1u) %
            kSlotCount;
    }

    struct Slot {
        ToneMatchProfile profile {};
        ZeroLatencyPartitionedFIR::PreparedKernel kernel {};
    };

    std::unique_ptr<std::array<Slot, kSlotCount>> slots_ {
        std::make_unique<std::array<Slot, kSlotCount>>()};
    std::unique_ptr<ZeroLatencyPartitionedFIR> kernelPreparer_ {
        std::make_unique<ZeroLatencyPartitionedFIR>()};

    std::atomic<std::size_t>
        writeIndex_ {0u};

    std::atomic<std::size_t>
        readIndex_ {0u};
};

} // namespace HighGainGuitarFinisher::dsp
