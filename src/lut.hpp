// Created: 2022-06-02

#ifndef _LUT_HPP_
#define _LUT_HPP_

#include "image.hpp"

#include <cmath>
#include <vector>
#include <utility>

namespace Lutools {

inline static constexpr size_t LUT_RAW_DATA_SIZE = static_cast<size_t>(256) * 256 * 256;

/// \brief Map a specific color to a unique 2D position, which maps to a pixel on the lutmap
/// \param color The color
/// \param axis Axis channel, whose value enumerates, tile by tile, throughout the entire lutmap: 0 for R, 1 for G, 2 for B
/// \param flip Whether or not the tile "flipping" (used to lower the spatial frequency and thereby reduce JPEG noise) are taken into consideration
/// \return a \c std::pair of two integers, x (horizontal pixel index) and y (vertical pixel index)
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

/// \brief Returns a given number of integer sample points evenly distributed through a given span
/// \param begin The begin point of the span
/// \param end The end point of the span
/// \param samples How many elements are desired in the return value
/// \return A \c std::vector , allocated \b on-stack, of \c samples integers, evenly distributed through the span [begin, end], in which \c begin and \c end are always included
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

/// \brief Analyzes a lutmap and cache the entire LUT, interpolation-free
/// \param input_file Path of the lutmap
/// \param output_file Path of the output (.lut format); writing is skipped if empty
/// \return An array of \c Color which stores the mapped value of all possible colors in the RGB colorspace; the mapped value can be accessed via index returned by \c Color::getHexRGB()
/// \remark Ensures a valid array of \c Color
[[nodiscard]] inline Color* cacheLUTMap(const std::string& input_file, const std::string& output_file) {
    const auto map = std::make_shared<Image>(input_file);
    if (map->getWidth() != 4096 && map->getHeight() != 4096) {
        throw std::runtime_error { "LUT map size must be 4096 x 4096" };
    }
    const std::string axis_annot = Pathutils::getSecondaryExtensionName(input_file);

    // Default axis is B (put most quantization loss on B -- the least noticeable light component for the eye)
    unsigned char axis = 2;
    if (axis_annot == "r") {
        axis = 0;
    } else if (axis_annot == "g") {
        axis = 1;
    }

    Color* data = nullptr;

    try // Touching pile memory in this block
    {
        data = new Color[LUT_RAW_DATA_SIZE] {};

        // Not a hot function, so we just do this ugly loop :D
        for (int r = 0; r < 256; ++r) {
            for (int g = 0; g < 256; ++g) {
                for (int b = 0; b < 256; ++b) {
                    Color rgba {
                        static_cast<unsigned char>(r),
                        static_cast<unsigned char>(g),
                        static_cast<unsigned char>(b),
                        255
                    };
                    data[rgba.getHexRGB()] = map->at(rgbToMapPosition(rgba, axis, true));
                }
            }
        }

        // Write lut file if output path is given
        if (!output_file.empty()) {
            std::ofstream fout;
            fout.open(output_file, std::ofstream::binary | std::ofstream::trunc);
            if (!fout.is_open()) {
                throw std::runtime_error { "unable to create LUT file" };
            }
            fout.write(
                reinterpret_cast<const char*>(data),
                static_cast<std::streamsize>(LUT_RAW_DATA_SIZE * sizeof(Color)));
            fout.close();
        }
    }
    catch (std::exception&) {
        delete[] data;
        throw;
    }
    return data;
}

/// \brief Loads a LUT cache into memory
/// \param path Path of the input (.lut format)
/// \return An array of \c Color which stores the mapped value of all possible colors in the RGB colorspace; the mapped value can be accessed via index returned by \c Color::getHexRGB()
/// \remark Ensures a valid array of \c Color
[[nodiscard]] inline Color* loadCacheFromFile(const std::string& path) {
    Color* data = nullptr;

    try // Touching pile memory in this block
    {
        data = new Color[LUT_RAW_DATA_SIZE] {};

        // Load filter into buffer
        std::ifstream fin;
        fin.open(path, std::ifstream::binary);
        if (!fin.is_open()) {
            throw std::runtime_error {
                std::string { "unable to open LUT file \"" } + path + "\""
            };
        }
        fin.read(reinterpret_cast<char*>(data), LUT_RAW_DATA_SIZE * sizeof(Color));
        if (fin.fail()) {
            throw std::runtime_error { "invalid LUT file" };
        }
        fin.close();
    }
    catch (std::exception&) {
        delete[] data;
        throw;
    }
    return data;
}
}

#endif // _LUT_HPP_
