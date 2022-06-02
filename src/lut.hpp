// Created: 2022-06-02

#ifndef _LUT_HPP_
#define _LUT_HPP_

#include "image.hpp"

#include <cmath>
#include <vector>
#include <utility>

inline constexpr size_t LUT_RAW_DATA_SIZE = static_cast<size_t>(256) * 256 * 256;

inline std::pair<int, int> rgbToMapPosition(Color color, unsigned char axis, bool flip) noexcept {
    axis += 3;
    const unsigned char a = axis % 3;
    const unsigned char h = (axis + 1) % 3;
    const unsigned char v = (axis - 1) % 3;
    const unsigned char quot = color[a] >> 4;
    const unsigned char rem = color[a] & 15; // "& 15" is "% 16"
    return {
        (rem << 8) + (flip && (color[a] & 1) ? 255 - color[h] : color[h]), // "& 1" is "% 2"
        (quot << 8) + (flip && (quot & 1) ? 255 - color[v] : color[v])
    };
}

inline std::vector<int> sampleSpan(int begin, int end, int samples) {
    if (samples < 2) {
        throw std::runtime_error { "too few samples" };
    }
    const double step = (end - begin) * (1.0 / (samples - 1));

    std::vector<int> sample_points {};
    sample_points.reserve(samples);
    double v = begin;
    for (int i = 0; i < samples; ++i) {
        sample_points.push_back(static_cast<int>(std::round(v)));
        v += step;
    }
    return sample_points;
}

#endif // _LUT_HPP_
