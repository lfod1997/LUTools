#include "cube.hpp"
#include "pathutils.hpp"
#include "thread_guard.hpp"

#include <iostream>
#include <thread>
#include <shared_mutex>
#include <vector>
#include <utility>

/// \brief LUTools the commandline tool, also serves as a demonstration of usage
int main(int argc, char** argv) {
    using namespace Lutools;
    using namespace LutoolsCli;
    using namespace Pathutils;

    if (argc < 2) {
        std::cout << "usage: " << getBaseName(argv[0]) << " {LUT | LUT_MAP} [-cube [RESOLUTION]] [INPUT [-OUTPUT]]..." << std::endl;
        return 0;
    }

    std::string lut_file = argv[1];
    Color* lut = nullptr;

    do {
        // We ultimately must have this
        const std::string raw_file = getExtensionNameRemoved(lut_file) + ".lut";

        try {
            // If lut file exists, load it
            if (isFileAvailable(raw_file)) {
                if (argc == 2) { break; }
                lut = loadCacheFromFile(raw_file); // If this throws, lut remains nullptr
            } else {
                lut = cacheLUTMap(lut_file, raw_file); // If this throws, lut remains nullptr
                std::cout << "generated: " << raw_file << std::endl;
            }

            // Determine whether a cube file is required
            if (argc == 2 || std::string { argv[2] } != "-cube") { break; }

            // Figure out the intended resolution
            int cube_res = 25;
            if (argc >= 4) {
                try {
                    cube_res = std::stoi(std::string { argv[3] });

                    // Eat the resolution arg because here it proves to be an integer
                    ++argv;
                    --argc;
                }
                catch (std::invalid_argument&) {}
            }

            // Eat the -cube arg
            ++argv;
            --argc;

            // Generate the cube file
            if (cube_res) {
                generateCube(lut, cube_res, getExtensionNameRemoved(lut_file) + ".cube");
                std::cout << "generated: cube file from LUT with resolution " << cube_res << std::endl;
            }
        }
        catch (std::exception& e) {
            delete[] lut;
            std::cerr << "error: " << e.what() << std::endl;
            return 1;
        }
    } while (false);

    // If this run is just to build a cache, here we're good to go
    if (argc == 2) { return 0; }

    // Eat the LUT-related 2 args, leaving only the input images
    ++++argv;
    ----argc;

    // Use the lowest compression level, I don't think people would rely on us to compress files :D
    stbi_write_png_compression_level = 5;

    // Initialize thread pool
    std::vector<ThreadGuard<std::thread>> workers {};
    workers.reserve(argc);
    std::mutex cout_mutex {}; // Force threads access stdout in order
    std::mutex cerr_mutex {}; // Force threads access stderr in order

    try {
        // Assign jobs
        for (int i = 0; i < argc; ++i) {
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
