#include "DemoGate.h"
#include <cmath>
#include <iostream>
#include <vector>

int main() {
    using HighGainGuitarFinisher::DemoGate;
    constexpr int sampleRate = 1000;

    std::vector<float> left(70000, 1.0f);
    std::vector<float> right(70000, 1.0f);
    float* outputs[2]{left.data(), right.data()};

    DemoGate demo;
    demo.configure(sampleRate, false);
    demo.process(outputs, 2, static_cast<int>(left.size()));

    for (int i = 0; i < 60000; ++i) {
        if (left[i] != 1.0f || right[i] != 1.0f) return 1;
    }
    if (!(left[60000] < 1.0f && left[60000] > 0.0f)) return 2;
    if (left[60020] != 0.0f || right[60020] != 0.0f) return 3;
    if (!(left[62999] > 0.0f && left[62999] <= 1.0f)) return 4;
    if (left[63000] != 1.0f || right[63000] != 1.0f) return 5;

    std::fill(left.begin(), left.end(), 1.0f);
    std::fill(right.begin(), right.end(), 1.0f);

    DemoGate full;
    full.configure(sampleRate, true);
    full.process(outputs, 2, static_cast<int>(left.size()));
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i] != 1.0f || right[i] != 1.0f) return 6;
    }

    std::cout << "DemoGate PASS\n";
    return 0;
}
