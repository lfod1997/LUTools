#include "image.hpp"
#include "pathutils.hpp"
#include "thread_guard.hpp"

#include <cmath>
#include <memory>
#include <iostream>
#include <fstream>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <unordered_map>
#include <utility>

static constexpr size_t LUT_RAW_DATA_SIZE = static_cast<size_t>(256) * 256 * 256;

inline std::pair<int, int> rgbToPosition(Color color, unsigned char axis, bool flip) noexcept {
    axis += 3;
    const unsigned char a = axis % 3;
    const unsigned char h = (axis + 1) % 3;
    const unsigned char v = (axis - 1) % 3;
    const unsigned char quot = color[a] / 16;
    const unsigned char rem = color[a] % 16;
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

inline void convertMap(const std::string& input_file, const std::string& output_file, int cube_samples = 0) {
    const auto map = std::make_shared<Image>(input_file);
    if (map->getWidth() != 4096 && map->getHeight() != 4096) {
        throw std::runtime_error { "LUT map size must be 4096 x 4096" };
    }
    const std::string axis_annot = getSecondaryExtensionName(input_file);

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
                    data[rgba.getHexRGB()] = map->at(rgbToPosition(rgba, axis, true));
                }
            }
        }

        // Write lut file
        std::ofstream fout;
        fout.open(output_file, std::ofstream::binary | std::ofstream::trunc);
        if (!fout.is_open()) {
            throw std::runtime_error { "unable to create LUT file" };
        }
        fout.write(
            reinterpret_cast<const char*>(data),
            static_cast<std::streamsize>(LUT_RAW_DATA_SIZE * sizeof(Color)));
        fout.close();

        // Just to make more out of this data array
        if (cube_samples != 0) {
            generateCube(data, cube_samples, getExtensionNameRemoved(output_file) + ".cube");
        }
    }
    catch (std::exception&) {
        delete[] data;
        throw;
    }

    delete[] data;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "usage: " << getBaseName(argv[0]) << " { LUT | LUT_MAP [-cube [SAMPLES]] } [INPUT [-OUTPUT]]..." << std::endl;
        return 0;
    }

    std::string lut_file = argv[1];

    do {
        // Continue on 3 occasions: 1. single extra arg; 2. explicitly wanted (for a cube file); 3. lut unavailable
        if (!(argc == 2 || argv[2][0] == '-' || getExtensionName(lut_file) != "lut")) { break; }

        // We ultimately must have this
        const std::string raw_file = getExtensionNameRemoved(lut_file) + ".lut";

        // If lut file exists and no intention of making cube file, skip
        const bool want_cube = argc != 2 && std::string { argv[2] } == "-cube";
        if (isFileAvailable(raw_file) && !want_cube) {
            lut_file = raw_file;
            break;
        }

        // Convert to LUT file & cube on demand
        try {
            int cube_samples = 0;
            if (want_cube) {
                if (getExtensionName(lut_file) == "lut") {
                    std::cerr << "error: a LUT map (image) must be given to convert to cube file, got LUT file" << std::endl;
                    return 1;
                }
                if (argc < 4) { cube_samples = 25; }
                else {
                    try {
                        cube_samples = std::stoi(std::string { argv[3] });

                        // Eat the samples arg
                        ++argv;
                        --argc;
                    }
                    catch (std::invalid_argument&) {
                        cube_samples = 25;
                    }
                }

                // Eat the -cube arg
                ++argv;
                --argc;
            }
            convertMap(lut_file, raw_file, cube_samples);
            std::cout << "generated: " << raw_file << std::endl;
            if (cube_samples) {
                std::cout << "generated: cube file from LUT with resolution " << cube_samples << std::endl;
            }
        }
        catch (std::exception& e) {
            std::cerr << "error: " << e.what() << std::endl;
            return 1;
        }

        // From now on we use the lut file
        lut_file = raw_file;
    } while (false);
    if (argc == 2) { return 0; }

    // Eat the very first arg
    ++argv;
    --argc;

    stbi_write_png_compression_level = 5;

    // Initialize thread pool
    std::vector<ThreadGuard<std::thread>> workers {};
    workers.reserve(argc - 2); // We need no more than this capacity
    std::mutex cout_mutex {};
    std::mutex cerr_mutex {};

    Color* lut = nullptr;

    try // Touching pile memory in this block
    {
        lut = new Color[LUT_RAW_DATA_SIZE] {};

        // Load filter into buffer
        std::ifstream fin;
        fin.open(lut_file, std::ifstream::binary);
        if (!fin.is_open()) {
            throw std::runtime_error {
                std::string { "unable to open LUT file \"" } + lut_file + "\""
            };
        }
        fin.read(reinterpret_cast<char*>(lut), LUT_RAW_DATA_SIZE * sizeof(Color));
        if (fin.fail()) {
            throw std::runtime_error { "invalid LUT file" };
        }
        fin.close();

        // Assign jobs
        for (int i = 1; i < argc; ++i) {
            // Determine filenames
            std::string input_file { argv[i] };
            std::string output_file =
                i < argc - 1 && argv[i + 1][0] == '-'
                    ? std::string { argv[++i] }.substr(1)
                    : getExtensionNameRemoved(input_file) + "_" + getBaseName(lut_file) + "." + getExtensionName(input_file);

            workers.emplace_back(
                [input_file = std::move(input_file), output_file = std::move(output_file), lut, &cout_mutex, &cerr_mutex] {
                    try {
                        // Load image
                        Image img { input_file };

                        // Color replacement
                        for (Color& px: img) {
                            Color mapped = lut[px.getHexRGB()];
                            mapped.a = px.a;
                            px = mapped;
                        }

                        // Write out
                        img.save(output_file);
                        std::lock_guard<std::mutex> lk { cout_mutex };
                        std::cout << "saved: " << output_file << std::endl;
                    }
                    catch (std::exception& e) {
                        // Encountering any exception, terminate thread
                        std::lock_guard<std::mutex> lk { cerr_mutex };
                        std::cerr << "error: " << e.what() << std::endl;
                    }
                });
        }
    }
    catch (std::exception& e) {
        std::lock_guard<std::mutex> lk { cerr_mutex };
        std::cerr << "error: " << e.what() << std::endl;
    }

    workers.clear(); // noexcept

    delete[] lut;

    return 0;
}
