#include "TestSupport.h"
#include "ToneMatchDSP.h"
#include "ZeroLatencyPartitionedFIR.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

using namespace HighGainGuitarFinisher::dsp;

namespace {

constexpr std::size_t kSamples = 2 * kToneMatchFirTapCount + 257u;

ToneMatchProfile makeProfile(double sign) {
    ToneMatchProfile p {};
    p.valid = true;
    p.firValid = true;
    p.firTaps[0] = sign * 0.7;
    for (std::size_t i = 1; i < p.firTaps.size(); ++i) {
        const double time = static_cast<double>(i);
        p.firTaps[i] = sign * (0.0010 * std::cos(0.023 * time)
            + 0.0007 * std::sin(0.007 * time))
            * std::exp(-time / 2400.0);
    }
    return p;
}

double input(std::size_t n, int channel) {
    const double t = static_cast<double>(n);
    // Stereo asymmetry; impulses expose off-by-one block/partition errors.
    const double impulse = n == (channel == 0 ? 0u : 137u) ? 0.75 : 0.0;
    return impulse +
        0.12 * std::sin((channel == 0 ? 0.091 : 0.127) * t) +
        0.04 * std::cos(0.0021 * t * t);
}

void testExactImpulseAndStream(double amount) {
    const auto profile = makeProfile(1.0);

    ToneMatchDSP match;
    match.prepare(48000.0);
    match.setProfile(profile);
    match.setAmount(amount);
    match.reset();

    std::array<std::vector<double>, 2> dry {};
    for (auto& channel : dry)
        channel.resize(kSamples);
    for (std::size_t n = 0; n < kSamples; ++n) {
        dry[0][n] = input(n, 0);
        dry[1][n] = input(n, 1);
    }

    double largestError = 0.0;
    for (std::size_t n = 0; n < kSamples; ++n) {
        double left = dry[0][n];
        double right = dry[1][n];
        match.processFrame(left, right);
        const double actual[2] {left, right};

        for (int c = 0; c < 2; ++c) {
            double direct = 0.0;
            if (amount > 0.0) {
                const std::size_t taps = std::min(n + 1u, kToneMatchFirTapCount);
                for (std::size_t k = 0; k < taps; ++k)
                    direct += profile.firTaps[k] * dry[c][n - k];
            } else {
                direct = dry[c][n];
            }

            const double expected =
                dry[c][n] + (direct - dry[c][n]) * amount;
            const double error = std::abs(actual[c] - expected);
            largestError = std::max(largestError, error);
            BF_REQUIRE(std::isfinite(actual[c]));
            BF_REQUIRE(error < 2.0e-10);
        }
    }

    std::cout << "partitioned FIR amount=" << amount
              << " maximum direct-convolution error=" << largestError << "\n";
}

void testResetAndProfileReplacement() {
    ZeroLatencyPartitionedFIR fir;
    auto positive = makeProfile(1.0);
    auto negative = makeProfile(-1.0);
    fir.setKernel(positive.firTaps);

    for (int i = 0; i < 267; ++i) {
        double left = std::sin(i * 0.17), right = std::cos(i * 0.12);
        fir.processFrame(left, right);
    }

    fir.setKernel(negative.firTaps);
    double left = 1.0, right = 0.0;
    fir.processFrame(left, right);
    // The new filter must start at sample zero, with no stale overlap tail.
    BF_REQUIRE(std::abs(left - negative.firTaps[0]) < 1.0e-10);
    BF_REQUIRE(std::abs(right) < 1.0e-10);

    fir.reset();
    left = 1.0;
    right = 0.0;
    fir.processFrame(left, right);
    BF_REQUIRE(std::abs(left - negative.firTaps[0]) < 1.0e-10);
    BF_REQUIRE(std::abs(right) < 1.0e-10);
}

} // namespace

int main() {
    testExactImpulseAndStream(0.0);
    testExactImpulseAndStream(0.25);
    testExactImpulseAndStream(1.0);
    testResetAndProfileReplacement();
    std::cout << "Bass Finisher V1.0.1 zero-latency convolution regression passed\n";
}
