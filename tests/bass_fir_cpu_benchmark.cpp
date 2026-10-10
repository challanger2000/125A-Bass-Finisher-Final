#include "TestSupport.h"
#include "ToneMatchDSP.h"

#include <chrono>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>

using namespace HighGainGuitarFinisher::dsp;

int main() {
    constexpr double kSampleRate = 48000.0;
    constexpr std::size_t kFrames = 96000u; // two seconds of real stereo audio

    ToneMatchProfile profile {};
    profile.valid = true;
    profile.firValid = true;

    profile.firTaps[0] = 0.82;
    for (std::size_t i = 1;
         i < profile.firTaps.size();
         ++i) {

        const double decay =
            std::exp(
                -static_cast<double>(i) /
                620.0);

        profile.firTaps[i] =
            0.00018 *
            decay *
            std::cos(
                0.013 *
                static_cast<double>(i));
    }

    ToneMatchDSP match;
    match.prepare(kSampleRate);
    match.setProfile(profile);
    match.setAmount(1.0);
    match.reset();

    double checksum = 0.0;

    const auto start =
        std::chrono::steady_clock::now();

    for (std::size_t i = 0;
         i < kFrames;
         ++i) {

        const double t =
            static_cast<double>(i) /
            kSampleRate;

        double left =
            0.2 *
            std::sin(
                2.0 *
                3.14159265358979323846 *
                117.0 *
                t);

        double right =
            0.2 *
            std::sin(
                2.0 *
                3.14159265358979323846 *
                173.0 *
                t);

        match.processFrame(
            left,
            right);

        checksum +=
            left * 0.31 +
            right * 0.17;
    }

    const auto stop =
        std::chrono::steady_clock::now();

    // Run a functionally identical 4096-tap DIRECT reference on the same
    // machine in the same test. The old CI benchmark was informational only
    // and did not enforce an acceptable CPU load.
    std::array<double, 2 * kToneMatchFirTapCount> historyL {};
    std::array<double, 2 * kToneMatchFirTapCount> historyR {};
    std::size_t write = 0;
    double directChecksum = 0.0;
    const auto directStart = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < kFrames; ++i) {
        const double t = static_cast<double>(i) / kSampleRate;
        const double left = 0.2 *
            std::sin(2.0 * 3.14159265358979323846 * 117.0 * t);
        const double right = 0.2 *
            std::sin(2.0 * 3.14159265358979323846 * 173.0 * t);
        write = (write + kToneMatchFirTapCount - 1) % kToneMatchFirTapCount;
        historyL[write] = historyL[write + kToneMatchFirTapCount] = left;
        historyR[write] = historyR[write + kToneMatchFirTapCount] = right;
        double sumL = 0.0, sumR = 0.0;
        for (std::size_t tap = 0; tap < kToneMatchFirTapCount; ++tap) {
            sumL += profile.firTaps[tap] * historyL[write + tap];
            sumR += profile.firTaps[tap] * historyR[write + tap];
        }
        directChecksum += sumL * 0.31 + sumR * 0.17;
    }
    const auto directEnd = std::chrono::steady_clock::now();

    const double directSeconds = std::chrono::duration<double>(
        directEnd - directStart).count();

    const double elapsedSeconds =
        std::chrono::duration<double>(
            stop - start).count();

    const double audioSeconds =
        static_cast<double>(kFrames) /
        kSampleRate;

    const double corePercent =
        100.0 *
        elapsedSeconds /
        audioSeconds;

    BF_REQUIRE(elapsedSeconds > 0.0);
    BF_REQUIRE(std::isfinite(checksum));
    BF_REQUIRE(std::isfinite(directChecksum));
    BF_REQUIRE(std::abs(checksum - directChecksum) < 1.0e-6);

    // Paired same-runner speedup is materially more robust than a fixed
    // absolute 'percent of one core' on shared CI hardware. Shipping
    // accidentally with the O(4096) sample loop must FAIL the release gate.
    const double speedup = directSeconds / elapsedSeconds;
    BF_REQUIRE(std::isfinite(speedup) && speedup > 2.0);

    std::cout
        << "BASS MATCH FIR 4096 partitioned convolution: "
        << elapsedSeconds
        << " s CPU wall time for "
        << audioSeconds
        << " s stereo audio, equivalent="
        << corePercent
        << "% of one core, checksum="
        << checksum
        << ", original-direct=" << directSeconds << " s"
        << ", paired-speedup=" << speedup << "x"
        << "\n";

    return 0;
}
