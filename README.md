# LUTools
Simple commandline tool &amp; header-only library to create, batch-apply and convert 3D LUTs.

## Features

1. ***Rip*** color toning filters from any of your favorite apps with **zero or very little accuracy loss**, even if the app only supports saving as low-res lossy JPEGs
2. Make your own filters for once and **generate perfect 3D LUTs** from them
3. **Ultra-fast exportation to `.cube` format**, which can be used with many professional software such as Photoshop, Lightroom & Lightroom Mobile, Premiere, Final Cut Pro and DaVinci Resolve
4. Quickly apply 3D LUT filter to a batch of images with multi-thread processing, without having to open-apply-save each one of them manually in editors
5. Have the above facilities in your own C++ project by adding some headers

## Using the commandline tool

In this section I'd prefer to use a more intuitive language to explain the usage :D

### Rip or make filters

You'll need to use your filter to process a specially designed image, [the lutmap](img/lutmap4096.png):

- Simply apply the filter to the lutmap and save it as PNG format to get the zero-loss result
- If your app doesn't support PNG, you may still get a decent result! The lutmap was originally designed to rip beautiful filters from my social apps, which will shrink my image size and save as a lossy JPEG.
- For even better results with JPEG, you are still encouraged to manually chop the lutmap 2-by-2 or 4-by-4 (without losing pixels), enlarge each of them by 2 or 4, process them with the same filter, then put them back together seamlessly to get a complete lutmap. This way you'll get rid of some JPEG noise, then a 4px (or 8px, depending on how much you've enlarged the pieces) gaussian blur will also help smoothing out the remaining subtle noise. **Remember to resize back to 4096 by 4096**, because LUTools only accept this size for a 3D LUT. You can do all these procedures within Photoshop, Krita or GIMP.

The processed lutmap contains everything about the filter, it can be used to export `.cube` files, or just to apply to any other image.

### Apply filters to image

Batch processing images with a given filter is as simple as mouse selections and drags:

1. Select all the images you want to process by holding Ctrl and clicking

2. Add the processed lutmap to your selection

3. Click and hold your mouse on the lutmap that you've selected last, drag and drop onto the LUTools executable.

    > **Let me explain this:** By doing so, you are dragging all other images together to the LUTools, which is actually passing the paths of all these files as a list, into LUTools. And the fact that you're dragging your mouse cursor on the lutmap means passing its path by the first parameter.

4. A terminal ("black window") will appear, processing all images using the filter described by the lutmap. Results will be saved to the same folder.

### Export .cube files

Well, forgive me that this must be done via the terminal. But I promise it will be easy:

1. Open the "Run" dialog by pressing Win + R

2. Type `cmd` into the dialog and press enter, now you've opened the terminal

3. Type `cd /d` then a space, then the directory where you've put the LUTools executable, press enter to run

4. Type `LUTools -cube` then a space, then the path to your filter (the processed lutmap), run it

    > **Tips:** It will be much more convenient if you put the LUTools executable and the filter in the same folder -- you'll only need to type the filter's filename.

    And a `.cube` file will be generated in-place, ready to be used.

5. Alternatively, you may type `LUTools -cube 64` to have the file generated with a higher resolution of 64. This works for more complicated LUTs. The default resolution is 25 as you might have seen.

### FAQ

**What is that `.lut` file generated in the filter's directory?**

-- That is the filter's cache file. LUTools uses it to accelerate filter loading, so that next time you use the same filter, the loading will be much faster. It's totally safe to delete it, because it can be regenerated.

## Development

### Clone

Use this: `git clone --recurse-submodules git@github.com:lfod1997/LUTools.git`

### Build

It is a CMake Project with only one target so just config and make it.

### LUTools CLI

`LUTools  {LUT | LUT_MAP} [-cube [RESOLUTION]] [INPUT [-OUTPUT]]...`

Where LUT stands for the generated `.lut` file; LUT_MAP stands for any processed (or unprocessed) lutmap.

- Optionally, `-cube` may be used with or without a RESOLUTION specified. The generated `.cube` file will contain RESOLUTION ^ 3 samples. Default resolution is 25.
- Optionally, any number of INPUT images may be passed, they will be processed using the specified LUT. If no OUTPUT is specified for the INPUT, the output file will be put in the same directory, with a suffix `_` followed by the filter being used, and in the same image format as the INPUT.
- Each INPUT may have an OUTPUT after it to explicitly specify the output path. This syntax requires a `-` prefix, otherwise I can't tell the difference :D

### C++ library

`#include` the headers in the `src` directory, and have the functions in your project.

**Generally you'll just need these**:

- `Lutools::Color* Lutools::cacheLUTMap(const std::string& input_file, const std::string& output_file)` in `lut.hpp`
- `Lutools::Color* Lutools::loadCacheFromFile(const std::string& path)` in `lut.hpp`
- `void Lutools::generateCube(const Lutools::Color* data, int cube_res, const std::string& output_file)` in `cube.hpp`

All functions are carefully documented so I won't bother speaking here.

Namespace `Lutools`:

- `color.hpp` contains a simple RGBA class
- `image.hpp` contains a simple image wrapper that supports image loading and writing
- `lut.hpp` supports analyzing lutmaps and cache IO
- `cube.hpp` supports exporting `.cube` files

Namespace `Pathutils`: only `pathutils.hpp`, contains simple functions I used to process paths. If the file bothers you, just combine it into some of the other headers :D

Namespace `LutoolsCli`: only `thread_guard.hpp`, contains a simple wrapper of threads, used by the CLI.

### Lutmap disassembled

This image called lutmap is designed to get the colors mapped by any 3D LUT (obviously), with some tricks against JPEG compression:

1. The image has relatively low spatial frequencies, resulting in lower JPEG noise
2. Very high redundancy, actually enumerating the entire colorspace, so partial detail damage can be easily recovered (a gaussian blur, or more preferably surface blur, will do)
3. While being 4K, the image data block is extremely regular, resulting in small PNGs

---

### Credits

This project uses the amazing [stb image library by nothings](https://github.com/nothings/stb).
