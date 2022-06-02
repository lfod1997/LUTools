// Created: 2022-06-02

#ifndef _CUBE_HPP_
#define _CUBE_HPP_

#include "lut.hpp"
#include "pathutils.hpp"

#include <fstream>

inline void generateCube(const Color* data, int cube_samples, const std::string& output_file) {
    std::ofstream fout;
    fout.open(output_file, std::ofstream::trunc);
    if (!fout.is_open()) {
        throw std::runtime_error { "unable to create cube file" };
    }
    fout << "# Created with LUTools by Oasin Lyu\n# https://github.com/lfod1997\n\n";
    fout << "TITLE " << getBaseName(output_file) << '\n';
    fout << "LUT_3D_SIZE " << cube_samples << "\n\n";

    // 6-digit fixed precision
    fout.precision(6);
    fout << std::fixed;

    const auto sample_points = sampleSpan(0, 255, cube_samples);
    for (int b_index = 0; b_index < cube_samples; ++b_index) {
        for (int g_index = 0; g_index < cube_samples; ++g_index) {
            for (int r_index = 0; r_index < cube_samples; ++r_index) {
                Color rgba {
                    static_cast<unsigned char>(sample_points[r_index]),
                    static_cast<unsigned char>(sample_points[g_index]),
                    static_cast<unsigned char>(sample_points[b_index]),
                    255
                };

                rgba = data[rgba.getHexRGB()];
                constexpr double quantization_factor = 1. / 255.;
                fout << rgba.r * quantization_factor << ' '
                    << rgba.g * quantization_factor << ' '
                    << rgba.b * quantization_factor << '\n';
            }
        }
    }
}

#endif // _CUBE_HPP_
